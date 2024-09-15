#ifndef CERROR_H
#define CERROR_H

#include <assert.h>
#include <stdbool.h>

typedef struct CError
{
    int code;
    char const *msg;
} CError;

#define CERROR_none ((CError){.code = 0, .msg = ""})
#define CERROR_memory_allocation ((CError){.code = 256, .msg = "Memory allocation/reallocation error"})
#define CERROR_str_exccedded_len ((CError){.code = 257, .msg = "String excedded the length"})
#define CERROR_out_is_null ((CError){.code = 258, .msg = "The out pointer is null"})
#define CERROR_no_such_source ((CError){.code = 259, .msg = "Source doesn't exists"})

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
