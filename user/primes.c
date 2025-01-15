#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pd[2];
    pipe(pd);
    if (fork() == 0) {
        //child
        close(pd[1]); //close write port
        close(0);
        dup(pd[0]);
        close(pd[0]);
        int p;
        int n;
        int pd_pipe[2];
        int forked = 0;
        read(0,&p, sizeof(int));
        printf("prime %d\n", p);
        while (read(0,&n, sizeof(int)) != 0)
        {
            if(n % p != 0) {
                if (forked == 0) {
                    pipe(pd_pipe);
                    if (fork() == 0) {
                        close(pd_pipe[1]);
                        close(0);
                        dup(pd_pipe[0]);
                        close(pd_pipe[0]);
                        read(0,&p, sizeof(int));
                        printf("prime %d\n", p);
                    } else {
                        close(pd_pipe[0]);
                        forked = 1;
                        close(1);
                        dup(pd_pipe[1]);
                        close(pd_pipe[1]);
                        write(1,&n,sizeof(int));
                    }
                }
                else{
                    write(1,&n,sizeof(int));
                }
            }
        }
        close(1);
        wait((int *) 0);
        exit(0);    
    }
    else{
        //parent
        close(pd[0]);//close read port
        for (int i = 2; i <= 35; i += 1) {
            write(pd[1],&i,sizeof(int));
        }
        close(pd[1]);
        wait((int *) 0);
    }
    exit(0);
}