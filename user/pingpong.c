#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int main(int argc, char* argv[]){
    int p[2];
    int pid;
    char buf_child[16] = {0};
    char buf_parent[16] = {0};

    buf_child[15] = '\0';
    buf_parent[0] = 'a';
    buf_parent[15] = '\0';
    
    pipe(p);
    pid = fork();
    if(pid == 0){
        read(p[0], buf_child, 1);
        close(p[0]);
        printf("%d: received ping\n", getpid());
        //printf("%s: received byte\n", buf_child);
        write(p[1], buf_child, 1);
        close(p[1]);
        //printf("%d: sending pong\n", getpid());
        exit(0);
    } else {
        write(p[1], buf_parent, 1);
        close(p[1]);
        //printf("%d: sending ping\n", getpid());
        wait(0);
        read(p[0], buf_parent+1, 1);
        close(p[0]);
        printf("%d: received pong\n", getpid());
        //printf("%s: received byte\n", buf_parent);
    }
    return 0;
}