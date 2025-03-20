#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void primes(int);

int main(int argc, char *argv[]){
    int i;
    int p[2]; 
    pipe(p);

    if(fork() == 0) {
        close(p[1]); // close write end p[1] for child process
        primes(p[0]); // child process read from p[0] and write to p_next[1] and recursively call primes
        exit(0); // child process exit
    }else {
        close(p[0]); // close read end p[0] for parent process
        for(i = 2; i <= 280; i++) {
            write(p[1], &i, 4); // the first parent process write 2-280 to p[1] 
        } 
        close(p[1]); // parent process' write to p[1] ends; 
        wait(0); // wait for the child process to end
    }

    return 0;
}


void primes(int p_0) { 
    int i, n;
    int p_next[2];  
    pipe(p_next);
    
    if(read(p_0, &i, 4) == 0){
        close(p_0);
        close(p_next[0]);
        close(p_next[1]);
    }else{
        if(fork() == 0) {
            close(p_0);
            close(p_next[1]);
            primes(p_next[0]);
            exit(0);
        }else{
            printf("prime %d\n", i);
            
            close(p_next[0]);
            while(read(p_0, &n, 4)){      // read from p[0]
                if( n % i ) {               // 筛掉倍数
                    write(p_next[1], &n, 4);  // write to p_next[1]
                }
            }
            close(p_0); // read from p[0] ends;
            close(p_next[1]); // write to p_next[1] ends;
            wait(0);
        }
    }
}