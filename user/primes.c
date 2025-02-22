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
        close(p[1]);
        primes(p[0]);
        exit(0);
    }else {
        close(p[0]);
        for(i = 2; i <= 280; i++) {
            write(p[1], &i, 4);
        } 
        close(p[1]);
        wait(0);
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
            while(read(p_0, &n, 4)){
                if( n % i ) {
                    write(p_next[1], &n, 4);
                }
            }
            close(p_0); // read from p[0] ends;
            close(p_next[1]); // write to p_next[1] ends;
            wait(0);
        }
    }
}