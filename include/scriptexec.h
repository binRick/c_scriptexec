#ifndef __SCRIPTEXEC_H__
#define __SCRIPTEXEC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "fsio.h"
#include "stringbuffer.h"
#include "stringfn.h"

struct ScriptExecOptions {
  char *runner;
  char *working_directory;
  bool exit_on_error;
  bool print_commands;
};

struct ScriptExecResult {
  int                    code;
  char                   *out, *err;
  int64_t                start, end, dur;
  struct StringFNStrings outs;
  struct StringFNStrings errs;
};

struct ScriptExecOptions scriptexec_create_options(void);

struct ScriptExecResult  scriptexec_run(const char * /* script */);
struct ScriptExecResult  scriptexec_run_with_options(const char * /* script */, struct ScriptExecOptions);

#endif

