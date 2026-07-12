#include "quill.h"

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static void check(int condition, const char *name) {
    fprintf(stderr, "%s: %s\n", name, condition ? "PASS" : "FAIL");
    if (!condition) ++failures;
}

int main(void) {
    check(quill_init() == 0, "initialize");
    check(quill_init() == 0, "idempotent initialize");
    if (failures) return failures;

    const int width = quill_width(), height = quill_height();
    const int stride = quill_stride();
    unsigned char *buffer = quill_buffer();
    check(width == 1620 && height == 2160, "known geometry");
    check(quill_format() == 4, "RGB32 format");
    check(stride >= width * 4 && buffer != NULL, "buffer and stride");

    check(quill_swap_ex(0, 0, 0, 10, 0, 0, QUILL_CONTENT_MONO) == 0,
          "empty rectangle rejected");
    check(quill_swap_ex(2000, 0, 10, 10, 0, 0, QUILL_CONTENT_MONO) == 0,
          "offscreen rectangle rejected");
    check(quill_swap_ex(INT_MAX, INT_MIN, INT_MAX, INT_MAX, 0, 0,
                        QUILL_CONTENT_MONO) == 0,
          "extreme rectangle rejected safely");
    check(quill_swap_ex(0, 0, 1, 1, 0, 0, 99) == 0,
          "invalid content type rejected");

    if (!buffer || stride < width * 4) return failures + 10;
    const int x = 8, y = 8, size = 8;
    unsigned char saved[size * size * 4];
    for (int row = 0; row < size; ++row)
        memcpy(saved + row * size * 4, buffer + (y + row) * stride + x * 4,
               size * 4);

    unsigned long last = 0;
    for (int iteration = 0; iteration < 200; ++iteration) {
        unsigned char value = (iteration & 1) ? 0xff : 0;
        for (int row = 0; row < size; ++row) {
            unsigned char *pixel = buffer + (y + row) * stride + x * 4;
            for (int column = 0; column < size; ++column) {
                pixel[column * 4] = value;
                pixel[column * 4 + 1] = value;
                pixel[column * 4 + 2] = value;
                pixel[column * 4 + 3] = 0xff;
            }
        }
        last = quill_swap_mono_fast(x, y, size, size);
        quill_process_events();
        if (!last) break;
    }
    check(last != 0, "200 partial-update stress swaps");

    /* Exercise clipping at all edges with an in-bounds one-pixel intersection. */
    check(quill_swap_mono_fast(-7, -7, 8, 8) != 0, "top-left clipping");
    check(quill_swap_mono_fast(width - 1, height - 1, 8, 8) != 0,
          "bottom-right clipping");

    /* Exercise every documented mode on a tiny region before restoring it. */
    check(quill_swap_ex(x, y, size, size, 1, 0, QUILL_CONTENT_COLOR) != 0,
          "color content mode 1 call");
    check(quill_swap_ex(x, y, size, size, 3, 0, QUILL_CONTENT_COLOR) != 0,
          "color mode 3 call");
    check(quill_swap_ex(x, y, size, size, 4, 0, QUILL_CONTENT_COLOR) != 0,
          "color mode 4 call");
    check(quill_swap_ex(x, y, size, size, 5, 0, QUILL_CONTENT_COLOR) != 0,
          "color mode 5 call");

    for (int row = 0; row < size; ++row)
        memcpy(buffer + (y + row) * stride + x * 4,
               saved + row * size * 4, size * 4);
    check(quill_swap_color_full(x, y, size, size) != 0,
          "complete color refresh call");
    quill_process_events();

    fprintf(stderr, "acceptance failures=%d\n", failures);
    return failures ? 1 : 0;
}
