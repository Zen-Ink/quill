#include "clip_rect.h"

#include <cassert>
#include <climits>

using quill_detail::ClippedRect;
using quill_detail::clip_rect;

static void expect(int x, int y, int w, int h, ClippedRect wanted) {
    ClippedRect actual{};
    assert(clip_rect(x, y, w, h, 1620, 2160, &actual));
    assert(actual.x == wanted.x && actual.y == wanted.y);
    assert(actual.width == wanted.width && actual.height == wanted.height);
}

int main() {
    expect(-10, -20, 30, 40, {0, 0, 20, 20});
    expect(1600, 2140, 100, 100, {1600, 2140, 20, 20});
    expect(0, 0, 1620, 2160, {0, 0, 1620, 2160});

    ClippedRect output{};
    assert(!clip_rect(2000, 0, 10, 10, 1620, 2160, &output));
    assert(!clip_rect(0, 0, 0, 10, 1620, 2160, &output));
    assert(!clip_rect(INT_MAX, 0, INT_MAX, 10, 1620, 2160, &output));
    assert(!clip_rect(INT_MAX, INT_MIN, INT_MAX, INT_MAX,
                      1620, 2160, &output));
    assert(!clip_rect(0, 0, 1, 1, 1620, 2160, nullptr));
}
