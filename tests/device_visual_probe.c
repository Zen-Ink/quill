/* Human-verified visual acceptance: orientation markers, RGB32 channel
 * order, and color-vs-grayscale mode behavior, all on one composite screen.
 * Expected on glass (portrait, cable at the bottom):
 *   - black bar along the TOP edge, black bar along the LEFT edge,
 *     black square near the BOTTOM-RIGHT corner (detects rotation/mirroring);
 *   - upper band, mode 4 color: bars red, green, blue, cyan, magenta,
 *     yellow, left to right;
 *   - lower band, same pixels but mode 1: collapses to grayscale.
 * Holds the composite, then exits so the wrapper restores xochitl.
 */
#include "quill.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static unsigned char *fb;
static int W, H, S;

static void fill(int x, int y, int w, int h,
                 unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > W) w = W - x;
    if (y + h > H) h = H - y;
    for (int row = 0; row < h; ++row) {
        unsigned char *p = fb + (size_t)(y + row) * S + (size_t)x * 4;
        for (int col = 0; col < w; ++col) {
            p[col * 4 + 0] = b;
            p[col * 4 + 1] = g;
            p[col * 4 + 2] = r;
            p[col * 4 + 3] = 0xff;
        }
    }
}

int main(void) {
    if (quill_init() != 0) {
        fprintf(stderr, "visual: init failed\n");
        return 1;
    }
    W = quill_width();
    H = quill_height();
    S = quill_stride();
    fb = quill_buffer();
    if (!fb || W <= 0 || H <= 0) return 1;

    for (int row = 0; row < H; ++row)
        memset(fb + (size_t)row * S, 0xff, (size_t)W * 4);
    fill(0, 0, W, 60, 0, 0, 0);
    fill(0, 0, 60, H, 0, 0, 0);
    fill(W - 260, H - 260, 200, 200, 0, 0, 0);
    quill_swap(0, 0, W, H, 3, 1);
    quill_process_events();
    sleep(4);

    static const struct { unsigned char r, g, b; } bars[6] = {
        {255, 0, 0},   {0, 255, 0},   {0, 0, 255},
        {0, 255, 255}, {255, 0, 255}, {255, 255, 0},
    };
    const int band_h = 600, bar_w = W / 6;
    const int color_y = 300, gray_y = 1100;

    for (int i = 0; i < 6; ++i)
        fill(i * bar_w, color_y, bar_w, band_h, bars[i].r, bars[i].g, bars[i].b);
    quill_swap_ex(0, color_y, W, band_h, 4, 0, QUILL_CONTENT_COLOR);
    quill_process_events();
    sleep(4);

    for (int i = 0; i < 6; ++i)
        fill(i * bar_w, gray_y, bar_w, band_h, bars[i].r, bars[i].g, bars[i].b);
    quill_swap_ex(0, gray_y, W, band_h, 1, 0, QUILL_CONTENT_COLOR);
    quill_process_events();

    fprintf(stderr, "visual: composite on glass; holding 120s\n");
    for (int i = 0; i < 120; ++i) {
        quill_process_events();
        sleep(1);
    }
    fprintf(stderr, "visual: done\n");
    return 0;
}
