# Linux and Raspberry Pi

The BOINC client runs as its own user (`boinc`) without a desktop session, and Linux keeps USB devices closed to such a
user. A test program started from your own account finds the counter, the BOINC task says "No detector found". One command
fixes it for the counters Yarrow reads:

    curl -fsSL https://raw.githubusercontent.com/alplix/yarrow/main/linux/yarrow-linux-setup.sh | sudo sh

It writes `/etc/udev/rules.d/99-yarrow.rules` (the file is in [../linux/99-yarrow.rules](../linux/99-yarrow.rules); read it
first if you like) and reloads udev. The rule matches the USB ids of the counters, and for the ids shared with other
devices also the product name, so nothing else on the computer is touched. Remove the file to undo it.

Then unplug the counter and plug it in again and restart the BOINC client (`sudo systemctl restart boinc-client`). A
running task looks for a counter every 15 seconds; a task that has already gone to sleep ("postponed for 21600 seconds")
starts again after the restart.

| Counter | Device | What the rule does |
|---|---|---|
| Radioactive@Home (all boards) | `/dev/hidraw*` | opens it for every user |
| FNIRSI GC-03 with Yarrow firmware | `/dev/ttyACM*` | opens it for every user, ModemManager leaves it alone |
| GQ GMC | `/dev/ttyUSB*` or `/dev/ttyACM*` | not covered (the USB serial chips are shared with many devices): `sudo usermod -aG dialout boinc` and restart the client |

## If it still does not work

Look at the `stderr.txt` of the task (BOINC Manager, Tasks, Properties, or `slots/N/stderr.txt` in the BOINC data
directory). The monitor writes what it sees:

- `yhid: /dev/hidrawN: this is the counter the monitor looks for, but the BOINC client's user may not open it` means the
  rule is not active yet: check that `/etc/udev/rules.d/99-yarrow.rules` exists, run `udevadm info -a /dev/hidrawN | grep product`
  and unplug and re-plug the counter.
- `rah: a device with the Radioactive@Home id answered, but it does not behave like the counter` means the counter was
  opened but its answers are not those of a Radioactive@Home counter; post that line on the forum.
- Nothing about `yhid` at all and `0 hidraw device(s)`: the BOINC client cannot see USB devices. A client installed as a
  Snap or Flatpak is confined; use the distribution's `boinc-client` package.
- Compare with the test program of the project (`yarrow_rah_probe`, in the files linked from the forum): if that finds the
  counter and BOINC does not, it is this permission.
