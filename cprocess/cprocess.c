#include "cprocess.h"

#include <assert.h>
#include <stdlib.h>

#include "subprocess.h"

int
cprocess_exec (char const* const command_line[], size_t commands_count)
{
  struct subprocess_s out_process = { 0 };

  printf ("command:");
  for (size_t iii = 0; iii < commands_count && command_line[iii]; ++iii)
    {
      printf (" %s", command_line[iii]);
    }
  puts ("");

  int status = subprocess_create (
      command_line,
      subprocess_option_inherit_environment |
          subprocess_option_search_user_path | subprocess_option_enable_async |
          subprocess_option_combined_stdout_stderr,
      &out_process
  );

  subprocess_join (&out_process, &status);

#ifndef _WIN32
  if (status)
    {
      char buf[BUFSIZ] = { 0 };
      while (subprocess_read_stdout (&out_process, buf, BUFSIZ))
        {
          printf ("%s", buf);
        }
    }
#endif

  subprocess_destroy (&out_process);

  return status;
}
