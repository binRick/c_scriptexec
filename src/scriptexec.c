#include "scriptexec.h"
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

#include "log.c"
#include "log.h"
#include "stringbuffer.h"
#include "stringfn.h"


// private functions
static struct ScriptExecResult _scriptexec_create_result();
static char *_scriptexec_read_and_delete_text_file(char *);

struct ScriptExecOptions scriptexec_create_options(){
  struct ScriptExecOptions options = {
    .runner = NULL, .working_directory = NULL, .exit_on_error = true, .print_commands = false
  };

  return(options);
}


int64_t ts64(void) {
  struct timeval tv;
  int            ret = gettimeofday(&tv, NULL);

  if (-1 == ret) {
    return(-1);
  }
  return((int64_t)((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000));
}


struct ScriptExecResult scriptexec_run(const char *script){
  struct ScriptExecOptions options = scriptexec_create_options();

  return(scriptexec_run_with_options(script, options));
}


struct ScriptExecResult scriptexec_run_with_options(const char *script, struct ScriptExecOptions options){
  struct ScriptExecResult result = _scriptexec_create_result();

  if (script == NULL) {
    result.code = -1;
    return(result);
  }

  struct StringBuffer *buffer = stringbuffer_new();

  // move to cwd
  char *cwd = getcwd(NULL, 0);

  stringbuffer_append_string(buffer, "cd ");
  stringbuffer_append_string(buffer, cwd);
  free(cwd);
  stringbuffer_append(buffer, '\n');

  // move to requested working directory
  if (options.working_directory != NULL) {
    stringbuffer_append_string(buffer, "cd ");
    stringbuffer_append_string(buffer, options.working_directory);
    stringbuffer_append(buffer, '\n');
  }

  if (options.exit_on_error) {
    stringbuffer_append_string(buffer, "set -e\n");
  }

  if (options.print_commands) {
    stringbuffer_append_string(buffer, "set -x\n");
  }

  stringbuffer_append_string(buffer, (char *)script);
  char *updated_script = stringbuffer_to_string(buffer);

  char template[] = "/tmp/scriptexec_XXXXXX";
  char *dir_name  = mkdtemp(template);

  result.start = ts64();

  if (dir_name == NULL) {
    stringbuffer_release(buffer);
    free(updated_script);

    result.code = -1;
    return(result);
  }

  stringbuffer_clear(buffer);
  stringbuffer_append_string(buffer, dir_name);
  stringbuffer_append_string(buffer, "/script");
  char *script_file = stringbuffer_to_string(buffer);

  // write script file
  bool written = fsio_write_text_file(script_file, updated_script);

  free(updated_script);
  if (!written) {
    rmdir(dir_name);

    stringbuffer_release(buffer);
    free(script_file);

    result.code = -1;
    return(result);
  }

  stringbuffer_append_string(buffer, ".out");
  char *out_file = stringbuffer_to_string(buffer);

  stringbuffer_clear(buffer);
  stringbuffer_append_string(buffer, script_file);
  stringbuffer_append_string(buffer, ".err");
  char *err_file = stringbuffer_to_string(buffer);

  // create command
  char *runner = options.runner;

  if (runner == NULL) {
    runner = "sh";
  }
  stringbuffer_clear(buffer);
  stringbuffer_append_string(buffer, runner);
  stringbuffer_append(buffer, ' ');
  stringbuffer_append_string(buffer, script_file);
  stringbuffer_append_string(buffer, " 2> ");
  stringbuffer_append_string(buffer, err_file);
  stringbuffer_append_string(buffer, " 1> ");
  stringbuffer_append_string(buffer, out_file);
  char *command = stringbuffer_to_string(buffer);

  stringbuffer_release(buffer);

  result.code = system(command);
  free(command);

  // delete files
  remove(script_file);
  rmdir(dir_name);

  // read out/err
  result.out = _scriptexec_read_and_delete_text_file(out_file);
  free(out_file);
  result.err = _scriptexec_read_and_delete_text_file(err_file);
  free(err_file);

  result.end = ts64();
  result.dur = result.end - result.start;
  log_debug("<%d> script_file: %s | out bytes: %d | exited %d | dur: %ld ms\n", getpid(), script_file, (int)strlen(result.out), result.code, result.dur);

  free(script_file);
  return(result);
} /* scriptexec_run_with_options */

static struct ScriptExecResult _scriptexec_create_result(){
  struct ScriptExecResult result = {
    .code  = 0,
    .outs  = { NULL },
    .errs  = { NULL },
    .out   = NULL,
    .err   = NULL,
    .dur   = -1,
    .start = -1,
    .end   = -1
  };

  return(result);
}


static char *_scriptexec_read_and_delete_text_file(char *file){
  char *text = fsio_read_text_file(file);

  remove(file);

  return(text);
}
