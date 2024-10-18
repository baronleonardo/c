#ifndef CCMD_H
#define CCMD_H

#include "cerror.h"

#include <stddef.h>

typedef enum CSubCmd
{
  CSUB_CMD_init,
  CSUB_CMD_build,
  CSUB_CMD_run,
  CSUB_CMD_test,
  CSUB_CMD_doc,
  CSUB_CMD_fmt,
  CSUB_CMD_help,
  CSUB_CMD_version,
} CSubCmd;

typedef struct CCmd
{
  CSubCmd subcmd;
  size_t argc;
  char** argv;
} CCmd;

CError ccmd_create (int argc, char* argv[], CCmd* out_ccmd);

void ccmd_destroy (CCmd* self);

#endif // CCMD_H
