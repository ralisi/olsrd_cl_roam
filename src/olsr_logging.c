#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "olsr_cfg.h"
#include "olsr_logging.h"
#include "log.h"

static void (*log_handler[MAX_LOG_HANDLER])
    (enum log_severity, enum log_source, const char *, int, char *, int);
static int log_handler_count = 0;

/* keep this in the same order as the enums with the same name ! */
const char *LOG_SOURCE_NAMES[] = {
  "all",
  "routing",
  "scheduler"
};

const char *LOG_SEVERITY_NAMES[] = {
  "INFO",
  "WARN",
  "ERROR"
};

void olsr_log_addhandler(void (*handler)(enum log_severity, enum log_source, const char *, int,
    char *, int)) {

  assert (log_handler_count < MAX_LOG_HANDLER);
  log_handler[log_handler_count++] = handler;
}

void olsr_log_removehandler(void (*handler)(enum log_severity, enum log_source, const char *, int,
    char *, int)) {
  int i;
  for (i=0; i<log_handler_count;i++) {
    if (handler == log_handler[i]) {
      if (i < log_handler_count-1) {
        memmove(&log_handler[i], &log_handler[i+1], (log_handler_count-i-1) * sizeof(*log_handler));
      }
      log_handler_count--;
    }
  }
}

void olsr_log (enum log_severity severity, enum log_source source, const char *file, int line, const char *format, ...) {
  static char logbuffer[LOGBUFFER_SIZE];

  if (olsr_cnf->log_event[severity][source]) {
    va_list ap;
    int p1,p2, i;

    va_start(ap, format);

    p1 = snprintf(logbuffer, LOGBUFFER_SIZE, "%s %s (%d): ", LOG_SEVERITY_NAMES[severity], file, line);
    p2 = vsnprintf(&logbuffer[p1], LOGBUFFER_SIZE - p1, format, ap);

    assert (p1+p2 < LOGBUFFER_SIZE);

    for (i=0; i<log_handler_count; i++) {
      log_handler[i](severity, source, file, line, logbuffer, p1);
    }
    va_end(ap);
  }
}

void olsr_log_stderr (enum log_severity severity __attribute__((unused)), enum log_source source __attribute__((unused)),
    const char *file __attribute__((unused)), int line __attribute__((unused)),
    char *buffer, int prefixLength __attribute__((unused))) {
  fputs (buffer, stderr);
}

void olsr_log_file (enum log_severity severity __attribute__((unused)), enum log_source source __attribute__((unused)),
    const char *file __attribute__((unused)), int line __attribute__((unused)),
    char *buffer, int prefixLength __attribute__((unused))) {
  fputs (buffer, olsr_cnf->log_target_file);
}

void olsr_log_syslog (enum log_severity severity, enum log_source source __attribute__((unused)),
    const char *file __attribute__((unused)), int line __attribute__((unused)),
    char *buffer, int prefixLength __attribute__((unused))) {
  olsr_syslog(severity, "%s", buffer);
}
