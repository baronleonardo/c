#include "cprocess.h"

#include <stdlib.h>

#include "subprocess.h"

int cprocess_exec(const char *const command_line[], size_t commands_count)
{
    struct subprocess_s out_process = {0};

    printf("command:");
    for (size_t iii = 0; iii < commands_count; ++iii)
    {
        printf(" %s", command_line[iii]);
    }
    puts("");

    int status = subprocess_create(command_line, subprocess_option_search_user_path | subprocess_option_enable_async | subprocess_option_combined_stdout_stderr, &out_process);

    subprocess_join(&out_process, &status);

    if (status)
    {
        FILE *f = subprocess_stdout(&out_process);
        char buf[BUFSIZ] = {0};
        while (fgets(buf, BUFSIZ, f))
        {
            printf("%s", buf);
        }
    }

    subprocess_destroy(&out_process);

    return status;
}
