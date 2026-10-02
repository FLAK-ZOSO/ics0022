#include <fiorino_log.h>

#include <stdatomic.h>
#include <errno.h>

static atomic_int threshold = FIORINO_LOG_WARNING;

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
