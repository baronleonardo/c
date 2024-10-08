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
  ((CError){ .code = 256, .msg = "Memory allocation/reallocation error" })
#define CERROR_str_exccedded_len                                               \
  ((CError){ .code = 257, .msg = "String excedded the length" })
#define CERROR_out_is_null                                                     \
  ((CError){ .code = 258, .msg = "The out pointer is null" })
#define CERROR_no_such_source                                                  \
  ((CError){ .code = 259, .msg = "Source doesn't exists" })
#define CERROR_build_function_not_found                                        \
  ((CError){ .code = 260,                                                      \
             .msg = "build function `CError (*)(CBuild*)` not found" })
#define CERROR_invalid_string                                                  \
  ((CError){ .code = 261,                                                      \
             .msg = "Invalid input string, must be zero terminated" })

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
