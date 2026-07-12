#!/bin/bash
# Run a quill takeover app safely: stop xochitl, own the panel, and ALWAYS
# bring xochitl back — on app exit, crash, or this script dying.
# Usage: takeover.sh /home/root/quill/scribble [args...]

APP="${1:-/home/root/quill/scribble}"
if [ $# -gt 0 ]; then
    shift
fi

restore() {
    rm -f /tmp/epframebuffer.lock
    systemctl start xochitl
    echo quill-takeover > /sys/power/wake_unlock 2>/dev/null
}
trap restore EXIT INT TERM

# The kernel autosleeps whenever no wakelock is held, and xochitl normally
# holds one. While xochitl is down we must hold our own, or the device
# suspends mid-session and the resume path starts a second display engine
# on top of the app — observed to end in a segfault and a forced reboot.
# (Do NOT `systemctl mask` xochitl here: mask forces a daemon-reload, which
# on this firmware tears down the caller's own dropbear SSH session.)
echo quill-takeover > /sys/power/wake_lock 2>/dev/null

systemctl stop xochitl
# xochitl's userspace EPD lock can linger; a stale one blocks the engine.
rm -f /tmp/epframebuffer.lock
sleep 1

cd "$(dirname "$APP")"
set +e
LD_LIBRARY_PATH=/home/root/quill:/usr/lib/plugins/scenegraph "$APP" "$@"
status=$?
set -e
echo "takeover: app exited ($status), restoring xochitl"
exit "$status"
