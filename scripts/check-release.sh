#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

fail() { echo "release check: $*" >&2; exit 1; }

[ -f LICENSE ] || fail "LICENSE is missing"
[ ! -e src/epfb.cpp ] || fail "prohibited historical implementation is present"
[ ! -e src/epframebuffer.h ] || fail "prohibited historical header is present"
if git ls-files 'vendor/*.so' | grep -q .; then
    fail "a proprietary vendor library is tracked"
fi

if [ -f build/libquill.so ]; then
    required='quill_init quill_width quill_height quill_stride quill_format quill_buffer quill_swap_ex quill_swap quill_swap_mono_fast quill_swap_mono_quality quill_swap_color quill_swap_color_full quill_process_events'
    exports=$(nm -D --defined-only build/libquill.so)
    for symbol in $required; do
        echo "$exports" | grep -Eq "[[:space:]]${symbol}$" || fail "missing export $symbol"
    done
fi

git diff --check
echo "release checks passed"
