#include "quill.h"

#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t stopping;
static void stop(int signal_number) {
    (void)signal_number;
    stopping = 1;
}

int main(void) {
    signal(SIGTERM, stop);
    signal(SIGINT, stop);
    if (quill_init() != 0) return 1;
    while (!stopping) {
        quill_process_events();
        usleep(10000);
    }
    quill_process_events();
    return 0;
}
