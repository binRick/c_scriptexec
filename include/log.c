#ifndef LOG_LOADED
#define LOG_LOADED    1
#include "log.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define MAX_CALLBACKS    32

typedef struct {
  log_LogFn fn;
  void      *udata;
  int       level;
} Callback;

static struct {
  void       *udata;
  log_LockFn lock;
  int        level;
  bool       quiet;
  Callback   callbacks[MAX_CALLBACKS];
} L;


static const char *level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};


static void stdout_callback(log_Event *ev) {
  char buf[16];

  buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
  fprintf(
    ev->udata, "%s %s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    level_icons[ev->level], buf, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void file_callback(log_Event *ev) {
  char buf[64];

  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void lock(void) {
  if (L.lock) {
    L.lock(true, L.udata);
  }
}


static void unlock(void) {
  if (L.lock) {
    L.lock(false, L.udata);
  }
}


const char * log_level_string(int level) {
  return(level_strings[level]);
}


void log_set_lock(log_LockFn fn, void *udata) {
  L.lock  = fn;
  L.udata = udata;
}


const char * log_get_level_string(){
  return(log_level_string(log_get_level()));
}


int log_get_level(){
  return(L.level);
}


void log_set_level(int level) {
  L.level = level;
}


void log_set_quiet(bool enable) {
  L.quiet = enable;
}


int log_add_callback(log_LogFn fn, void *udata, int level) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (!L.callbacks[i].fn) {
      L.callbacks[i] = (Callback) { fn, udata, level };
      return(0);
    }
  }
  return(-1);
}


int log_add_fp(FILE *fp, int level) {
  return(log_add_callback(file_callback, fp, level));
}


static void init_event(log_Event *ev, void *udata) {
  if (!ev->time) {
    time_t t = time(NULL);
    ev->time = localtime(&t);
  }
  ev->udata = udata;
}


void log_log(int level, const char *file, int line, const char *fmt, ...) {
  log_Event ev = {
    .fmt   = fmt,
    .file  = file,
    .line  = line,
    .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    Callback *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}
#endif
