#!/bin/sh
# Lets the BOINC client open the Geiger counters Yarrow reads (Radioactive@Home, FNIRSI GC-03 with the Yarrow firmware).
# Works on Debian, Ubuntu, Raspberry Pi OS, Mint, Fedora, Arch: anything with udev.
#   curl -fsSL https://raw.githubusercontent.com/alplix/yarrow/main/linux/yarrow-linux-setup.sh | sudo sh
# It writes /etc/udev/rules.d/99-yarrow.rules and reloads udev. Nothing else is changed; remove the file to undo it.
set -e
if [ "$(id -u)" != 0 ]; then
    echo "Run this as root:  sudo sh $0" >&2
    exit 1
fi
RULES=/etc/udev/rules.d/99-yarrow.rules
cat > "$RULES" <<'EOF'
# Yarrow: lets the BOINC client open the Geiger counters the monitor reads (https://yarrow.athena.org.tr/linux)
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="05df", ATTRS{product}=="*radioactiveathome*", MODE="0666"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="05df", ATTRS{product}=="*Radiation Detector KIT*", MODE="0666"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="f6fe", ATTRS{product}=="*radioactiveathome*", MODE="0666"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="0001", MODE="0666", ENV{ID_MM_DEVICE_IGNORE}="1"
EOF
udevadm control --reload-rules
udevadm trigger --subsystem-match=hidraw --subsystem-match=tty
echo "Done: $RULES is installed."
echo "Unplug the counter and plug it in again, then restart the BOINC client so a task that has gone to sleep starts looking again:"
echo "  sudo systemctl restart boinc-client"
