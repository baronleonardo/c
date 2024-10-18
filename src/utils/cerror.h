#ifndef CERROR_H
#define CERROR_H

#include <assert.h>
#include <stdbool.h>

typedef struct CError
{
  int code;
  char const* msg;
} CError;

#define CERROR_none ((CError){ .code = 0, .msg = "" })
#define CERROR_memory_allocation                                               \
  ((CError){ .code = 1, .msg = "c: Memory allocation/reallocation error" })
#define CERROR_str_exccedded_len                                               \
  ((CError){ .code = 2, .msg = "c: String excedded the length" })
#define CERROR_no_such_source                                                  \
  ((CError){ .code = 3, .msg = "c: Source doesn't exists" })
#define CERROR_build_function_not_found                                        \
  ((CError){ .code = 4,                                                        \
             .msg = "c: build function `CError (*)(CBuild*)` not found" })
#define CERROR_invalid_string                                                  \
  ((CError){ .code = 5,                                                        \
             .msg = "c: Invalid input string, must be zero terminated" })
#define CERROR_invalid_target_type                                             \
  ((CError){ .code = 6, .msg = "c: Invalid target type" })
#define CERROR_wrong_options                                                   \
  ((CError){ .code = 7, .msg = "c: wrong option(s)" })
#define CERROR_failed_command                                                  \
  ((CError){ .code = 8, .msg = "c: command failed" })

// #define CERROR_fs(m) ((CError){ .code = 260, .msg = m })
// #define CERROR_arr(m) ((CError){ .code = 261, .msg = m })
// #define CERROR_str(m) ((CError){ .code = 262, .msg = m })

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
