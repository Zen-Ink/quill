# quill

Takeover display host for the reMarkable Paper Pro: stops xochitl and drives
the e-ink panel directly through the vendor waveform engine
(`libqsgepaper`'s `EPFramebuffer`), with raw evdev input.

This is the lowest-latency third-party drawing path that exists on this device
short of reverse-engineering the FPGA transport frame format.

Quill supports two verified update classes on the Paper Pro:

- **fast mono dirty-rects** — `EPContentType::Mono`, mode `0`; best for live ink.
- **color dirty-rects** — `EPContentType::Color`, modes `3`/`4`/`5`; usable for
  static color UI, images, highlights, translucent overlays, and small partial
  region updates. Mode `1` was observed to collapse to grayscale even with
  `EPContentType::Color`.

Color is written through the captured RGB32 aux framebuffer (`B,G,R,0xFF`) and
pushed with `quill_swap_ex(..., QUILL_CONTENT_COLOR)` or the semantic wrappers
in `src/quill.h`. At runtime Quill supports both vendor QRect swap ABIs: the
OS 3.28+ signature without `EPContentType` is preferred, while the older
content-aware signature remains available as a fallback. On the current ABI,
the selected `EPScreenMode` carries the effective mono/color behavior.

- `src/vendor_probe.cpp`, `src/vendor_probe.h` — clean-room Qt/vendor boundary
  and controlled framebuffer discovery
- `src/quill.h`, `src/quill_c.cpp` — C ABI over the engine (init/buffer/swap)
  for C and Rust apps, including semantic mono/color swap wrappers
- `src/scribble.c` — C1 milestone: pen-to-glass latency demo (exit: pen
  side-button in hover, 5-finger tap, power button, or SIGTERM)
- `examples/vanishing_trail.c` — archived no-AI drawing experiment demonstrating live ink and delayed erasure
- `src/map_demo.c` — Marauder's-Map-style demo: full map render + tiny
  partial-update animated footsteps (`marauders_map.png` is its map;
  `medium-map.gif` / `deviant-map.gif` are recordings of it running)
- `src/image_demo.cpp` — render a PNG/JPEG/etc. image through Qt's QImage
  loader, scaled to the panel
- `src/color_probe.c` — full-screen RGB/CMY color path probe
- `src/color_mode_compare.c` — side-by-side mode comparison (`1`, `3`, `4`, `5`)
- `src/color_partial_probe.c` — small dirty-rect color add/erase/churn probe
- `src/color_blend_probe.c` — software alpha, color mixing, and stacked partial
  color updates
- `src/image_anim_demo.cpp`, `src/gif_demo.cpp` — partial-update animation
  experiments (sprites over a still image; GIF playback)
- `scripts/takeover.sh` — stop xochitl, run app, ALWAYS restore xochitl
- `build.sh` — cross-build against the SDK selected by `SDK_ENV` +
  `vendor/libqsgepaper.so` pulled from the device when absent (the SDK comes
  from reMarkable's developer program)

Exit the demos: power button, 5-finger tap, or SIGTERM.

## Build and verification

```sh
SDK_ENV=/path/to/environment-setup-cortexa53-crypto-remarkable-linux \
  ./build.sh                  # cross-build with the reMarkable SDK
SDK_ENV=/path/to/environment-setup-cortexa53-crypto-remarkable-linux \
QUILL_LIBRARY_ONLY=1 ./build.sh  # build only libquill.so for embedding
./scripts/test-host.sh        # clipping tests with ASan and UBSan
./scripts/check-release.sh    # ABI and redistribution checks
./scripts/device-suite.sh     # on-device acceptance suite (run ON the tablet,
                              # detached — see the header comment)
```

Takeover sessions hold a kernel wakelock (`/sys/power/wake_lock`): autosleep
is active on this firmware and a mid-session suspend resumes into a second
xochitl instance fighting the app for the panel. `scripts/takeover.sh`
handles this; see `docs/device-test-2026-07-12.md` for the incident record.

Framebuffer selection and display behavior must also be tested on the target
hardware. If initialization reports multiple valid candidates, validate them
on a development tablet and set `QUILL_AUX_BUFFER_INDEX` to the confirmed
writable auxiliary image. Never guess when selection is ambiguous.

Clean-room provenance is recorded in `CLEANROOM.md`. Quill's independently
authored source is available under the MIT license; `libqsgepaper.so` is
proprietary and is neither covered by that license nor redistributed here.

## C ABI policy

Use `quill_buffer()` to write pixels and one of these swap calls to push dirty
rectangles:

```c
quill_swap_mono_fast(x, y, w, h);      // Mono, mode 0, full=0: live ink
quill_swap_mono_quality(x, y, w, h);   // Mono, mode 3, full=0
quill_swap_color(x, y, w, h);          // Color, mode 4, full=0: normal color UI
quill_swap_color_full(x, y, w, h);     // Color, mode 4, full=1: cleanup/full reveal

quill_swap_ex(x, y, w, h, mode, full, QUILL_CONTENT_COLOR); // raw escape hatch
```

Dirty rectangles are clipped at the C ABI boundary. Keep `full=0` for local
work: the vendor backend may promote `CompleteRefresh` to a whole-panel update
even when the supplied rectangle is small. `quill_swap_mono_fast()`,
`quill_swap_mono_quality()`, and `quill_swap_color()` are partial by contract.

Empirical Paper Pro findings:

- `QUILL_CONTENT_COLOR + mode 3/4/5` renders color.
- `QUILL_CONTENT_COLOR + mode 1` renders grayscale/mono.
- `QUILL_CONTENT_MONO` renders grayscale/mono regardless of mode.
- Small partial color dirty-rects work; software-composited transparency and
  stacked color updates are viable. There is no hardware alpha channel — alpha
  means preblend in software, then write opaque RGB pixels.
