#include "cprocess.h"
#include "helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#endif

#include <subprocess.h>

#include "defer.h"

CError
cprocess_exec (
    char const* const command_line[],
    size_t commands_count,
    bool verbose,
    int* out_status,
    CStr* out_stdout_stderr
)
{
  struct subprocess_s out_process = { 0 };
  CError err = CERROR_none;

  c_defer_init (3);

  if (verbose)
    {
      printf ("command:");
      for (size_t iii = 0; iii < commands_count && command_line[iii]; ++iii)
        {
          printf (" %s", command_line[iii]);
        }
      puts ("");
    }

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
  c_defer_err (true, NULL, NULL, subprocess_destroy (&out_process));
  if (out_status)
    {
      *out_status = status;
    }

  int join_status = subprocess_join (&out_process, &status);
  c_defer_check (join_status == 0, NULL, NULL, NULL);

  if (verbose)
    {
      printf ("Status: %d\n", status);
    }

  if (status != 0)
    {
#ifdef _WIN32
      DWORD process_error = GetLastError ();
      if (process_error > 0 && process_error < 500)
        {
          if (process_error == ERROR_FILE_NOT_FOUND)
            {
              if (out_stdout_stderr)
                {
                  c_str_error_t str_err = c_str_format (
                      out_stdout_stderr,
                      0,
                      C_STR_INV (
                          "Error(subprocess): '%s': No such file or directory"
                      ),
                      command_line[0]
                  );
                  c_defer_err (
                      str_err.code == 0,
                      NULL,
                      NULL,
                      err = CERROR_internal_error (str_err.desc)
                  );
                }
            }
          else
            {
              if (out_stdout_stderr)
                {
                  c_str_error_t str_err = c_str_format (
                      out_stdout_stderr,
                      0,
                      C_STR_INV ("Error(subprocess): code: %lu, io error"),
                      process_error
                  );
                  c_defer_err (
                      str_err.code == 0,
                      NULL,
                      NULL,
                      err = CERROR_internal_error (str_err.desc)
                  );
                }
            }

          c_defer_check (false, NULL, NULL, err = CERROR_failed_command);
        }
#endif
    }

  if (out_stdout_stderr)
    {
      out_stdout_stderr->len = subprocess_read_stdout (
          &out_process, out_stdout_stderr->data, out_stdout_stderr->capacity
      );
      if (verbose && out_stdout_stderr->len > 0)
        {
          puts (out_stdout_stderr->data);
        }
    }

  c_defer_deinit ();

  return err;
}
