#!/usr/bin/env python3
"""Command line client for the Yarrow serial protocol (needs: pip install pyserial).

    python tools/yarrow.py COM5 info
    python tools/yarrow.py COM5 sync                 set the device clock from this computer
    python tools/yarrow.py COM5 live                 stream measurements until Ctrl+C
    python tools/yarrow.py COM5 download log.csv     read the whole log and decode it
    python tools/yarrow.py COM5 cal                  show the calibration
    python tools/yarrow.py COM5 cal 18000 90 100 100  set sensitivity (x100), dead time us, E %, H %
    python tools/yarrow.py COM5 events
"""
import base64
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ylog_decode import decode  # noqa: E402


class Yarrow:
    def __init__(self, port):
        self.port = port

    def _line(self, timeout=3.0):
        end = time.time() + timeout
        buf = b""
        while time.time() < end:
            c = self.port.read(1)
            if c == b"\n":
                return buf.decode("ascii", "replace").rstrip("\r")
            if c:
                buf += c
        raise TimeoutError("no answer from the device")

    def command(self, text):
        self.port.write((text + "\n").encode("ascii"))
        first = self._line()
        while first.startswith("L "):  # a live line may arrive first
            first = self._line()
        if not first.startswith("OK"):
            raise RuntimeError(first)
        return first

    def lines_until_end(self):
        out = []
        while True:
            line = self._line()
            if line == "END":
                return out
            out.append(line)

    def info(self):
        return dict(kv.split("=") for kv in self.command("INFO").split()[1:])

    def sync_time(self):
        self.command(f"TIME {int(time.time())}")

    def calibration(self, values=None):
        if values:
            self.command("CAL " + " ".join(str(v) for v in values))
        return dict(kv.split("=") for kv in self.command("CAL").split()[1:])

    def events(self):
        n = int(self.command("EVENTS").split()[1])
        rows = [tuple(int(x) for x in line.split()[1:]) for line in self.lines_until_end()]
        assert len(rows) == n
        return rows

    def log_pages(self):
        info = dict(kv.split("=") for kv in self.command("LOGINFO").split()[1:])
        pages, size = int(info["pages"]), int(info["pagesize"])
        image = bytearray()
        for p in range(pages):
            head = self.command(f"LOGPAGE {p}").split()
            assert int(head[2]) == size
            data = base64.b64decode("".join(self.lines_until_end()))
            assert len(data) == size, f"page {p}: {len(data)} bytes"
            image += data
        return bytes(image), size

    def download(self, path):
        image, size = self.log_pages()
        n = 0
        with open(path, "w") as f:
            f.write("time,interval,time_valid,value\n")
            for t, iv, ok, v in decode(image, size):
                f.write(f"{t},{iv},{ok},{v}\n")
                n += 1
        return n


def main():
    import serial

    if len(sys.argv) < 3:
        print(__doc__)
        return
    y = Yarrow(serial.Serial(sys.argv[1], 115200, timeout=0.2))
    cmd, args = sys.argv[2], sys.argv[3:]
    if cmd == "info":
        print(y.info())
    elif cmd == "sync":
        y.sync_time()
        print("clock set")
    elif cmd == "cal":
        print(y.calibration([int(a) for a in args] if args else None))
    elif cmd == "events":
        for e in y.events():
            print(e)
    elif cmd == "download":
        print(y.download(args[0] if args else "yarrow-log.csv"), "samples written")
    elif cmd == "live":
        y.command("LIVE 1")
        try:
            while True:
                print(y._line(5))
        except KeyboardInterrupt:
            y.command("LIVE 0")
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
