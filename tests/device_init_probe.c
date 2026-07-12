#include "quill.h"

#include <stdio.h>

int main(void) {
    fprintf(stderr, "probe: calling first init\n");
    int first = quill_init();
    fprintf(stderr, "probe: init_first=%d\n", first);
    if (first != 0) return first;

    fprintf(stderr, "probe: calling second init\n");
    int second = quill_init();
    fprintf(stderr, "probe: init_second=%d\n", second);
    fprintf(stderr, "width=%d\nheight=%d\nstride=%d\nformat=%d\nbuffer=%s\n",
           quill_width(), quill_height(), quill_stride(), quill_format(),
           quill_buffer() ? "non-null" : "null");
    fprintf(stderr, "probe: processing events\n");
    quill_process_events();
    fprintf(stderr, "probe: complete\n");
    return second;
}
