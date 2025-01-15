#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define sizebuf 512

void find(char* path,char* name, char* filename);

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(2, "Usage: find dir filename\n");
        exit(1);
    }

    char buf[512];
    char* path = argv[1];
    char* filename = argv[2];
    strcpy(buf,path);

    find(buf,path,filename);

    exit(0);
}

void find(char* path, char* name ,char* filename){
    struct dirent de;
    struct stat st;
    char* p;
    int fd;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE:
        if (strcmp(name, filename) == 0) {
            printf("%s\n",path);
        }
        break;

    case T_DIR:
        p = path+strlen(path);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0 || strcmp(de.name,".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            find(path,de.name,filename);
        }
        break;
    }
    close(fd);
}