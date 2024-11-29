#ifndef CERROR_H
#define CERROR_H

typedef struct CError {
  int         code;
  char const* desc;
} CError;

#define CERROR_none ((CError){.code = 0, .desc = ""})
#define CERROR_memory_allocation                                               \
  ((CError){.code = 1, .desc = "c: Memory allocation/reallocation error"})
#define CERROR_str_exccedded_len                                               \
  ((CError){.code = 2, .desc = "c: String excedded the length"})
#define CERROR_no_such_source                                                  \
  ((CError){.code = 3, .desc = "c: Source doesn't exists"})
#define CERROR_build_function_not_found                                        \
  ((CError){.code = 4,                                                         \
            .desc = "c: build function `CError (*)(CBuild*)` not found"})
#define CERROR_invalid_string                                                  \
  ((CError){.code = 5,                                                         \
            .desc = "c: Invalid input string, must be zero terminated"})
#define CERROR_invalid_target_type                                             \
  ((CError){.code = 6, .desc = "c: Invalid target type"})
#define CERROR_wrong_options ((CError){.code = 7, .desc = "c: wrong option(s)"})
#define CERROR_failed_command ((CError){.code = 8, .desc = "c: command failed"})
#define CERROR_no_such_target ((CError){.code = 9, .desc = "c: no such target"})
#define CERROR_internal_error(m) ((CError){.code = 10, .desc = m})

/**********************************************************************/
/************************** Error management **************************/
/**********************************************************************/

// #define ON_ERR_BASE(
//     error_buf, error_code, error_msg, return_error, category, label
// )
//   if (error_code != 0)
//     {
//       return_error = c_generate_error (
//           &error_buf,
//           C_STRINGIFY (category),
//           error_code,
//           error_msg,
//           __FILE__,
//           __LINE__,
//           __func__,
//           C_VER
//       );
//       goto label;
//     }

// static inline CError
// c_generate_error (
//     CStr* error_buf,
//     char const category[],
//     int error_code,
//     char const error_msg[],
//     char const filename[],
//     size_t line,
//     char const function_name[],
//     char const version[]
// )
// {
//   /* Get the current time */
//   time_t t = time (NULL);
//   struct tm* current_time = localtime (&t);

//   c_str_format (
//       error_buf,
//       0,
//       C_STR_INV ("Category: %s\nCode: %zu\nDescription: %s\nFile: %s\n"
//                  "Line: %zu\nFunction: %s\nVersion: %s\nDate: %d-%02d-%02d"),
//       C_STRINGIFY (category),
//       error_code,
//       error_msg,
//       filename,
//       line,
//       function_name,
//       version,
//       current_time->tm_year + 1900,
//       current_time->tm_mon + 1,
//       current_time->tm_mday
//   );

//   if (strcmp (category, "fs") == 0)
//     {
//       return CERROR_fs (error_buf->data);
//     }
//   else if (strcmp (category, "arr") == 0)
//     {
//       return CERROR_arr (error_buf->data);
//     }
//   else if (strcmp (category, "str") == 0)
//     {
//       return CERROR_str (error_buf->data);
//     }
//   // else if (strcmp (category, "cbuild") == 0)
//   //   {
//   //     return CERROR_cbuild (error_buf->data);
//   //   }
//   else
//     {
//       return CERROR_none;
//     }
// }

// static inline bool c_on_error(bool has_err, int *err, int err_code)
// {
//     // assert(*err);
//     if (has_err)
//     {
//         *err = err_code;
//         return true;
//     }

//     return false;
// }

#endif // CERROR_H
