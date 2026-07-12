#include "quill.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    int result = quill_init();
    if (result) {
        fprintf(stderr, "swap probe: initialization failed: %d\n", result);
        return result;
    }

    const int x = 20, y = 20, width = 24, height = 24;
    const int stride = quill_stride();
    unsigned char *buffer = quill_buffer();
    if (!buffer || quill_format() != 4 || stride < quill_width() * 4) return 20;

    const size_t row_bytes = (size_t)width * 4;
    unsigned char *saved = malloc(row_bytes * height);
    if (!saved) return 21;
    for (int row = 0; row < height; ++row) {
        unsigned char *target = buffer + (size_t)(y + row) * stride + (size_t)x * 4;
        memcpy(saved + (size_t)row * row_bytes, target, row_bytes);
        for (int column = 0; column < width; ++column) {
            target[column * 4 + 0] = 0;
            target[column * 4 + 1] = 0;
            target[column * 4 + 2] = 0;
            target[column * 4 + 3] = 0xff;
        }
    }

    unsigned long draw_token = quill_swap_mono_fast(x, y, width, height);
    quill_process_events();
    usleep(300000);

    for (int row = 0; row < height; ++row) {
        unsigned char *target = buffer + (size_t)(y + row) * stride + (size_t)x * 4;
        memcpy(target, saved + (size_t)row * row_bytes, row_bytes);
    }
    unsigned long restore_token = quill_swap_mono_quality(x, y, width, height);
    quill_process_events();
    free(saved);

    fprintf(stderr, "swap probe: draw_token=%lu restore_token=%lu\n",
            draw_token, restore_token);
    return (draw_token && restore_token) ? 0 : 22;
}
