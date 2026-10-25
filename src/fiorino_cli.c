#include <fiorino_cli.h>
#include <fiorino_log.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <syslog.h>

static volatile sig_atomic_t signo;
static volatile sig_atomic_t interrupted;

void fiorino_cli_init_logging(const char* tag) {
    openlog(tag, LOG_PID, LOG_USER);
    fiorino_log_set_threshold(FIORINO_LOG_INFO);
    fprintf(stderr, "Read Fiorino logs with: journalctl -t %s\n", tag);
}

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
    int signalsPosixTerm[] = { SIGHUP, SIGINT, SIGPIPE, SIGTERM };
    for (size_t i = 0; i < sizeof signalsPosixTerm / sizeof signalsPosixTerm[0]; i++) {
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
