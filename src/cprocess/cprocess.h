#ifndef CPROCESS_H
#define CPROCESS_H

#include <stdbool.h>
#include <stddef.h>

#include "cerror.h"

#include <str.h>

CError
cprocess_exec(char const* const command_line[],
              size_t commands_count,
              bool verbose,
              int* out_status,
              CStr* out_stdout_stderr);

#endif // CPROCESS_H
