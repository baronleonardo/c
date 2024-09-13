#ifndef CCMD_H
#define CCMD_H

typedef enum CSubCmd {
    CSUB_CMD_init,
    CSUB_CMD_build,
    CSUB_CMD_run,
    CSUB_CMD_test,
    CSUB_CMD_doc,
    CSUB_CMD_fmt,
    CSUB_CMD_help,
    CSUB_CMD_version,
} CSubCmd;

typedef struct CCmd {
    CSubCmd* subcmd;
} CCmd;

CCmd ccmd_create(int argc, char* argv[]);

void ccmd_destroy(CCmd* self);

#endif // CCMD_H
