#include <fiorino_log.h>

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

static int messages;
static int last_priority;
static char last_message[1024];

/* Capture syslog calls without requiring a running journal in the test. */
void syslog(int priority, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_message, sizeof last_message, format, args);
    va_end(args);
    last_priority = priority;
    ++messages;
}
int main(void) {
    /* Default threshold suppresses debug while retaining warning messages. */
    assert(fiorino_log_get_threshold() == FIORINO_LOG_WARNING);
    fiorino_log_message(FIORINO_LOG_DEBUG, "hidden");
    assert(messages == 0);

    fiorino_log_message(FIORINO_LOG_WARNING, "file %s", "name");
    assert(messages == 1);
    assert(last_priority == (LOG_USER | LOG_WARNING));
    assert(strcmp(last_message, "fiorino: file name") == 0);

    /* Lowering the threshold exposes debug messages. */
    assert(fiorino_log_set_threshold(FIORINO_LOG_DEBUG) == 0);
    fiorino_log_message(FIORINO_LOG_DEBUG, "visible");
    assert(messages == 2);
    assert(last_priority == (LOG_USER | LOG_DEBUG));

    /* Errno logging adds context and preserves the caller's errno value. */
    errno = ENOENT;
    fiorino_log_errno(FIORINO_LOG_ERROR, "open", "missing");
    assert(messages == 3);
    assert(strstr(last_message, "open 'missing':") != NULL);
    assert(errno == ENOENT);

    /* Threshold filtering, invalid thresholds, and the fully-off setting. */
    assert(fiorino_log_set_threshold(FIORINO_LOG_ERROR) == 0);
    errno = ELOOP;
    fiorino_log_errno(FIORINO_LOG_WARNING, "open", "symlink");
    assert(messages == 3);
    assert(errno == ELOOP);
    fiorino_log_errno(FIORINO_LOG_ERROR, "write", "vault");
    assert(messages == 4);
    assert(last_priority == (LOG_USER | LOG_ERR));

    assert(fiorino_log_set_threshold((enum FiorinoLogLevel)5) == -1);
    assert(fiorino_log_get_threshold() == FIORINO_LOG_ERROR);
    assert(fiorino_log_set_threshold(FIORINO_LOG_OFF) == 0);
    fiorino_log_message(FIORINO_LOG_ERROR, "hidden too");
    assert(messages == 4);
    return 0;
}
