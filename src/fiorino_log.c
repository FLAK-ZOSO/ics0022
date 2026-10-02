#include <fiorino_log.h>

#include <stdatomic.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <syslog.h>

static atomic_int threshold = FIORINO_LOG_WARNING;

/** \brief Assesses whether a logging level is valid
 *  \param level chosen logging level
 *  \note FIORINO_LOG_OFF is treated as invalid
 *  \return whether the logging level is valid
 *  \retval 0 when the level is invalid
 *  \retval 1 when the level is valid
 */
static int valid_level(enum FiorinoLogLevel level) {
    return level == FIORINO_LOG_ERROR || level == FIORINO_LOG_WARNING ||
           level == FIORINO_LOG_INFO || level == FIORINO_LOG_DEBUG;
}

int fiorino_log_set_threshold(enum FiorinoLogLevel level) {
    if (level != FIORINO_LOG_OFF && !valid_level(level)) return -1;
    atomic_store(&threshold, level);
    return 0;
}

enum FiorinoLogLevel fiorino_log_get_threshold(void) {
    return atomic_load(&threshold);
}


void fiorino_log_message(enum FiorinoLogLevel level, const char* format, ...) {
    int saved_errno = errno;
    int limit = atomic_load(&threshold);
    if (!valid_level(level) || limit == FIORINO_LOG_OFF || level > limit)
        return;

    char stack_message[512];
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    int length = vsnprintf(stack_message, sizeof stack_message, format, args);
    va_end(args);
    if (length < 0) {
        va_end(copy);
        errno = saved_errno;
        return;
    }

    char* message = stack_message;
    if ((size_t)length >= sizeof stack_message) {
        message = malloc((size_t)length + 1);
        if (message != NULL)
            vsnprintf(message, (size_t)length + 1, format, copy);
        else
            message = stack_message;
    }
    va_end(copy);

    syslog(LOG_USER | level, "fiorino: %s", message);
    if (message != stack_message) free(message);
    errno = saved_errno;
}

/** \brief Logs with information retrieved from the errno
 *  \note The errno value is preserved by the function
 */
void fiorino_log_errno(enum FiorinoLogLevel level, const char* action, const char* path) {
    int error = errno;
    char description[128];
    if (strerror_r(error, description, sizeof description) != 0)
        snprintf(description, sizeof description, "errno %d", error);
    fiorino_log_message(level, "%s '%s': %s", action, path, description);
    errno = error;
}
