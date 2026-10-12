#include <fiorino_cli.h>
#include <signal.h>
#include <stddef.h>

static volatile sig_atomic_t signo;
static volatile sig_atomic_t interrupted;

static void handler(int signo_) {
    interrupted = 1;
    signo = signo_;
}

int fiorino_cli_install_signal_handler(void (*callback)(int)) {
    if (callback == NULL) {
        callback = &handler;
    }
    struct sigaction saction = {0};
    saction.sa_handler = callback;
    /// \todo We may want to use SA_SIGINFO later among .sa_flags
    sigemptyset(&saction.sa_mask);
    // https://man7.org/linux/man-pages/man7/signal.7.html
    int signalsPosixTerm[] = { SIGALRM, SIGHUP, SIGINT, SIGPIPE, SIGTERM };
    for (int i = 0; i < sizeof signalsPosixTerm; i++) {
        // https://pubs.opengroup.org/onlinepubs/009695399/functions/sigaction.html
        if (sigaction(signalsPosixTerm[i], &saction, NULL) == -1)
            return -1;
    }
    return 0;
}

int fiorino_cli_raised(void) {
    return interrupted != 0;
}
int fiorino_cli_signo(void) {
    return (int)signo;
}
