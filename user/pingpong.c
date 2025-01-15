#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pid;
    int pd[2];
    pipe(pd);
    pid = fork();
    if (pid == 0) {
        //child
        char buf[1];
        pid = getpid();
        read(pd[0],buf,1);
        printf("%d: received ping\n", pid);
        write(pd[1], buf, 1);
    } else{
        //parent
        pid = getpid();
        char ball[1] = {'1'};
        char ret[1];
        write(pd[1],ball,1);
        wait((int *) 0);
        read(pd[0], ret, 1);
        if (ret[0] == '1') {
            printf("%d: received pong\n", pid);
        }
    }
    exit(0);
}