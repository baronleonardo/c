#include "cprocess.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#endif

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

#ifdef _WIN32
  SetLastError (0);
#endif
  int status = subprocess_create (
      command_line,
      subprocess_option_inherit_environment |
          subprocess_option_search_user_path | subprocess_option_no_window |
          subprocess_option_enable_async |
          subprocess_option_combined_stdout_stderr,
      &out_process
  );

  subprocess_join (&out_process, &status);

  // on error
  printf ("Status: %d\n", status);
  if (status)
    {
#ifdef _WIN32
      DWORD process_error = GetLastError ();
      if (process_error > 0 && process_error < 500)
        {
          if (process_error == ERROR_FILE_NOT_FOUND)
            {
              fprintf (
                  stderr,
                  "Error(subprocess): '%s': No such file or directory\n",
                  command_line[0]
              );
            }
          else
            {
              fprintf (
                  stderr,
                  "Error(subprocess): code: %lu, io error\n",
                  process_error
              );
            }
          goto End;
        }
#endif
      char buf[BUFSIZ] = { 0 };
      while (subprocess_read_stdout (&out_process, buf, BUFSIZ))
        {
          printf ("%s", buf);
        }
      puts ("");
    }
#ifdef _WIN32
End:
#endif
  subprocess_destroy (&out_process);
  return status;
}
