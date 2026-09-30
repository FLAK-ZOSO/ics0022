#pragma once

/* Levels follow syslog(3): lower values are more severe. */
enum FiorinoLogLevel {
    FIORINO_LOG_OFF = -1,
    FIORINO_LOG_ERROR = 3,
    FIORINO_LOG_WARNING = 4,
    FIORINO_LOG_INFO = 6,
    FIORINO_LOG_DEBUG = 7,
};

/* The process-wide Fiorino threshold defaults to WARNING. OFF silences it.
 * Returns -1 without changing the threshold for an invalid level.
 */
int fiorino_log_set_threshold(enum FiorinoLogLevel level);
enum FiorinoLogLevel fiorino_log_get_threshold(void);

/* Messages use the LOG_USER facility. On Fedora, journald receives them.
 * The calling program retains its syslog identity and mask.
 */
void fiorino_log_message(enum FiorinoLogLevel level, const char* format, ...);
void fiorino_log_errno(enum FiorinoLogLevel level, const char* action,
                       const char* path);
