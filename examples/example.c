#include "scriptexec.h"
#include "stringfn.h"
#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


void dd(){
  struct StringFNStrings strings_struct = stringfn_split("first line\nsecond line\n3rd", '\n');

  printf("Strings Count: %d\n%s and %s and %s\n", strings_struct.count, strings_struct.strings[0], strings_struct.strings[1], strings_struct.strings[2]);
  stringfn_release_strings_struct(strings_struct);

  strings_struct = stringfn_split_lines("first line\nsecond line\n3rd");
  printf("Strings Count: %d\n%s and %s and %s\n", strings_struct.count, strings_struct.strings[0], strings_struct.strings[1], strings_struct.strings[2]);
  stringfn_release_strings_struct(strings_struct);
}

#include "stringfn.c"


int main(){
//dd();
  struct ScriptExecResult  result;
  struct ScriptExecOptions options = scriptexec_create_options();

  options.runner            = "sh";  // default is 'sh'
  options.working_directory = "/";   // default to current working directory
  options.exit_on_error     = true;  // default true, will break script execution on any error
  options.print_commands    = false; // default false, if true will print every command before invocation
  result                    = scriptexec_run_with_options("echo -n ok", options);
  result                    = scriptexec_run_with_options("last|tail\necho 1a\necho 1aaEEEEEEE \necho 2\necho 3\necho 4\necho 555\n\necho ok\n\n", options);
  if ((int)strlen(result.out) > 0) {
    result.outs = stringfn_split_lines_and_trim(stringfn_trim(result.out));
  }
  if ((int)strlen(result.err) > 0) {
    result.errs = stringfn_split_lines_and_trim(stringfn_trim(result.err));
  }
  printf("Code: %d\nOutput:\n%s\nError:\n%s\nDur:%ldms\nstarted:%ld|ended:%ld\n", result.code, result.out, result.err, result.dur, result.start, result.end);
  printf("   out len: %d\n", (int)strlen(result.out));
  printf("   outs qty: %d\n", result.outs.count);
  printf("   err len: %d\n", (int)strlen(result.err));
  printf("   errs qty: %d\n", result.errs.count);

  free(result.out);
  free(result.err);
}
