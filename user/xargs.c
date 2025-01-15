#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    char* new_argv[MAXARG];
    int new_argc = argc - 1;
    char param[512];
    int p = 0;
    for (int i = 1; i < argc; i += 1) {
        new_argv[i-1] = argv[i];
    }

    while(read(0,param + p,1) != 0) {
        if (param[p] == '\n') {
            param[p] = 0;
            new_argv[new_argc] = malloc(1+strlen(param));
            strcpy(new_argv[new_argc],param);
            new_argv[new_argc + 1] = 0;
            if (fork() == 0) {
                exec(new_argv[0],new_argv);
                exit(1);
            }
            else{
                wait((int *)0);
                new_argc = argc - 1;
                p = 0;
            }
        }
        else {
            if (param[p] == ' ') {
                param[p] = 0;
                new_argv[new_argc] = malloc(1+strlen(param));
                strcpy(new_argv[new_argc],param);
                new_argc++;
                p=0;
            }
            else{
                p++;
            }
        }
    }

    exit(0);
}