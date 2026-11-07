#pragma once

/** Levels follow syslog(3): lower values are more severe.
 *  \note We are not using higher severity than ERROR
 *  \note We have no usecase for the NOTIFICATION severity so far
*/
enum FiorinoLogLevel {
    FIORINO_LOG_OFF = -1,
    FIORINO_LOG_ERROR = 3,
    FIORINO_LOG_WARNING = 4,
    FIORINO_LOG_INFO = 6,
    FIORINO_LOG_DEBUG = 7,
};

int fiorino_log_set_threshold(enum FiorinoLogLevel level);
enum FiorinoLogLevel fiorino_log_get_threshold(void);

/* Messages use the LOG_USER facility.
 * The calling program retains its syslog identity and mask.
 */
void fiorino_log_message(enum FiorinoLogLevel level, const char* format, ...);
void fiorino_log_errno(enum FiorinoLogLevel level, const char* action, const char* path);
void fiorino_log_message_at(enum FiorinoLogLevel level,
                            const char* source_file, int source_line,
                            const char* format, ...);
void fiorino_log_errno_at(enum FiorinoLogLevel level,
                          const char* source_file, int source_line,
                          const char* action, const char* path);

/* Add call-site location automatically; the logger prints it for DEBUG only.
 * The original function symbols remain available for binary compatibility.
 */
#define fiorino_log_message(level, ...) \
    fiorino_log_message_at((level), __FILE__, __LINE__, __VA_ARGS__)
#define fiorino_log_errno(level, action, path) \
    fiorino_log_errno_at((level), __FILE__, __LINE__, (action), (path))
