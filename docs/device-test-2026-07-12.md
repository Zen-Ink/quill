# Device test record — 2026-07-12

## Environment

- Device: reMarkable Paper Pro (`imx8mm-ferrari`, AArch64)
- reMarkable OS image: 3.27.3.0
- Codex Linux: 5.7.126 (scarthgap)
- Kernel: 6.12.49
- Qt Core: 6.8.2
- `libqsgepaper.so` SHA-256:
  `3800e9f3d01f40fa3f5cc96ca49bcd982471ce5b033320f8788707520c8ddd08`
- Build ID reported from the matching local device copy:
  `97e9f265840ce822a93a4ddb52f4192c80e343f3`

The vendor library on the tablet and the untracked local build copy had the
same SHA-256 digest.

## Results

### Initialization and shutdown

- First `quill_init()`: 0
- Second `quill_init()`: 0
- Width: 1620
- Height: 2160
- Stride: 6528 bytes
- Qt image format: 4 (`QImage::Format_RGB32`)
- Buffer: non-null
- Plausible candidates: one; no index override required
- Event processing returned normally
- Normal process shutdown: successful
- `xochitl` restoration: successful

An initial test exposed a shutdown crash caused by destroying the adapter-owned
`QCoreApplication` before the vendor singleton had shut down. The adapter now
keeps that application alive for process lifetime because the vendor ABI has no
public shutdown operation. The repeated test exited with status zero and the
vendor reported that its update and display threads stopped normally.

### Partial swap

A 24 by 24 RGB32 region at `(20, 20)` was preserved, filled with opaque black,
submitted through `quill_swap_mono_fast`, restored, and submitted through
`quill_swap_mono_quality`.

- Fast draw token: 2
- Quality restore token: 2
- Process exit: 0
- `xochitl` restoration: successful

This confirms the tested swap calling convention, writable buffer access,
stride addressing, two mono wrapper mappings, and self-restoring partial
updates on this firmware.

### Automated acceptance and stress probe

The automated probe passed all checks:

- empty, offscreen, extreme, and invalid-content requests returned zero;
- top-left and bottom-right intersecting rectangles produced update tokens;
- 200 consecutive partial mono updates completed;
- color-content calls in modes 1, 3, 4, and 5 produced update tokens;
- a complete color refresh call produced an update token;
- process shutdown and `xochitl` restoration completed normally.

These results establish call-path operation, not visual color fidelity. Visual
comparison is still required to establish channel order and the observed
color-versus-grayscale behavior of each mode.

### SIGTERM lifecycle

A dedicated probe initialized Quill, processed events, received SIGTERM, and
exited normally. Vendor update/display threads shut down, the process returned
zero, and `xochitl` was restored.

### `scribble` termination/reboot incident — root-caused and resolved

Journal forensics from the incident boot showed the crash was environmental,
not an adapter defect:

1. The tablet had entered kernel autosleep during an idle gap
   (`/sys/power/autosleep` is `mem`; the kernel suspends whenever no wakelock
   is held, and nothing holds one while `xochitl` is down).
2. The test's SSH connection woke it; `xochitl` was stopped and `scribble`
   took the panel.
3. Five seconds after the stop, the resume path auto-started a second
   `xochitl` instance, which initialized the display engine on top of the
   running takeover app.
4. That instance shut down ten seconds later, `scribble` segfaulted, and the
   OS layer invoked an explicit `reboot` (memfault reason: `UserReset`).

Mitigation: `scripts/takeover.sh` now holds `/sys/power/wake_lock` for the
whole session, which prevents the suspend and therefore the resume-triggered
second engine. `systemctl mask` was evaluated as an additional guard and
rejected: the daemon-reload it triggers tears down active dropbear
per-connection SSH sessions on this firmware.

Re-test under the hardened wrapper: `scribble` received SIGTERM four seconds
after start, printed its normal exit line, shut the vendor engine down
cleanly, and exited 0.

### Re-test with the polished adapter

After the post-review polish (atomic constructor-cache, diagnostics), the
suite was re-run detached from the SSH session. Library SHA-256
`e395af8e7ffb614dedf4370ef5fa2acc5ffa1f195da415b9049fd4f7d96f9929`, confirmed
identical on host and device. Results: all acceptance checks passed (exit 0),
SIGTERM probe exit 0, `scribble` SIGTERM exit 0, `xochitl` restored.

### Dynamic-linker binding evidence — captured

`LD_DEBUG=bindings` with `LD_DEBUG_OUTPUT` (run detached) recorded:

- `libqsgepaper.so`'s reference to
  `_ZN6QImageC1EPhiixNS_6FormatEPFvPvES2_` binds to `libquill.so` —
  the vendor engine's framebuffer constructions flow through the
  interposed observer;
- `libquill.so` in turn binds that symbol to `libQt6Gui.so.6` — the
  `dlsym(RTLD_NEXT)` forwarding path to the real constructor;
- no external binding of the `C2` entry point was observed on this Qt
  build; both entry points remain exported for safety.

The test binaries embed `DT_RUNPATH` (not `DT_RPATH`), so the
`LD_LIBRARY_PATH` used by the test wrappers takes precedence and the
clean-room build was definitively the library under test (also confirmed
by SHA-256 comparison).

### Visual acceptance — human-confirmed

`tests/device_visual_probe.c` painted one composite screen (white page with
orientation markers via a full quality refresh, then two identical
red/green/blue/cyan/magenta/yellow bar bands submitted as color content in
mode 4 and mode 1 respectively) and held it while the device owner inspected
the panel. Confirmed by the owner:

- orientation markers exactly as drawn — top-edge bar, left-edge bar,
  bottom-right square; no rotation, mirroring, channel offset, or row skew
  (validates stride-aware addressing and panel orientation);
- mode 4 band: six distinct colors in the drawn order red, green, blue,
  cyan, magenta, yellow — validates the RGB32 little-endian `B,G,R,0xFF`
  byte order end to end;
- mode 1 band (same pixel data): collapsed to grayscale — confirms the
  documented content/mode mapping.

## Still pending

- Longer soak testing beyond the repeated 200-update runs
- Additional firmware versions
