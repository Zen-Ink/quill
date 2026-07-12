#!/bin/sh
# On-device acceptance suite for the quill adapter. Deploy libquill.so, the
# device_* probes, scribble, and this script into one directory on the tablet,
# then launch it DETACHED from the SSH session — on this firmware an SSH
# session can be killed mid-run by autosleep or a systemd daemon-reload:
#
#   ssh rm 'nohup sh /home/root/quill-cleanroom-test/device-suite.sh \
#       > /home/root/quill-cleanroom-test/suite.log 2>&1 &'
#
# Always restores xochitl, whatever happens.
DIR="${1:-$(dirname "$0")}"
cd "$DIR" || exit 1

restore() {
    rm -f /tmp/epframebuffer.lock
    systemctl start xochitl
    echo quill-suite > /sys/power/wake_unlock 2>/dev/null
    echo "=== suite complete ==="
}
trap restore EXIT INT TERM

# Hold a wakelock: kernel autosleep is active and nothing else holds one
# while xochitl is down; a mid-run suspend resumes into a second xochitl
# instance fighting the probe for the panel.
echo quill-suite > /sys/power/wake_lock
systemctl stop xochitl
rm -f /tmp/epframebuffer.lock
sleep 1

export LD_LIBRARY_PATH="$DIR:/usr/lib/plugins/scenegraph"

echo "=== acceptance probe ==="
./device_acceptance_probe
echo "acceptance_code=$?"
rm -f /tmp/epframebuffer.lock

echo "=== term probe: SIGTERM 2s after init ==="
./device_term_probe &
pid=$!
sleep 2
kill -TERM "$pid"
wait "$pid"
echo "term_code=$?"
rm -f /tmp/epframebuffer.lock

echo "=== scribble: SIGTERM 4s after start ==="
./scribble &
pid=$!
sleep 4
kill -TERM "$pid"
wait "$pid"
echo "scribble_code=$?"
rm -f /tmp/epframebuffer.lock

echo "=== dynamic-linker binding evidence ==="
rm -f /tmp/quill-ldbind.*
LD_DEBUG=bindings LD_DEBUG_OUTPUT=/tmp/quill-ldbind ./device_init_probe >/dev/null 2>&1
echo "ldprobe_code=$?"
echo "--- C1 bindings resolved to libquill:"
grep -h "_ZN6QImageC1EPhiixNS_6FormatEPFvPvES2_" /tmp/quill-ldbind.* 2>/dev/null | grep libquill | head -n 4
echo "--- C2 bindings resolved to libquill:"
grep -h "_ZN6QImageC2EPhiixNS_6FormatEPFvPvES2_" /tmp/quill-ldbind.* 2>/dev/null | grep libquill | head -n 4
rm -f /tmp/quill-ldbind.*
