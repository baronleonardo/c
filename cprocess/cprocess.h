#ifndef CPROCESS_H
#define CPROCESS_H

#include <stddef.h>

int cprocess_exec(const char *const command_line[], size_t commands_count);

#endif // CPROCESS_H
