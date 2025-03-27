#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

// xargs 
// 关键点在于对xargv数组的处理和fork与exec的使用;
// 以及对readline函数的实现用来读取一行

   /* xargs
    1.需要从标准输入中读取每一行，并将每一行作为一个参数添加至命令的char *argv[]
    2.使用fork与exec 来invoke 命令 

    出现错误记录：
    1.使用strcpy(xargv[i-1], argv[i])是不可以这样复制字符串到xargv数组中的,因为xargv只是字符型指针数组，
    而不是存储了字符串的数组，所以需要为xargv数组中的每一个指针分配内存空间，然后将argv中的字符串复制到xargv中
    2.需要为xargv数组中的每一个指针分配内存空间，否则会出现段错误 
    3.在readline函数中，需要将读取到的行添加到xargv数组中
    4.xargv数组中的最后一个参数需要为0,否则exec函数会出现错误
    5.在main函数中，需要释放xargv数组中的每一个指针所分配的内存空间;xargv数组中参数为0的指针不需要释放
    */

int Readline(char *line);

int main(int argc, char *argv[]){
    int i;
    char* xargv[MAXARG] = {0};
    for(i = 0; i < MAXARG; i++){
        xargv[i] = (char*)malloc(512 * sizeof(char));
    }
    

    if(argc < 2){
        fprintf(2, "usage: run command...\n");
        exit(1);
    }
    // printf("in main\n");

    // 先将argv中已有参数复制到xargv中
    for(i = 1; i < argc; i++){
        strcpy(xargv[i-1], argv[i]);
        // printf("%s\n", xargv[i-1]);
    }
    // printf("copy argv\n");
    xargv[argc] = 0; // 最后一个参数为0

    // 从标准输入读取行,并将每一行附加至xargv中
    // 每读取一行，fork一个子进程，执行命令
    while(Readline(xargv[argc - 1])){
        // fork and exec
        if(fork() == 0) {
            exec(argv[1], xargv);
        }else{
            wait(0);
        }
    }

    for(i = 0; i < MAXARG; i++){
        if(xargv[i] != 0){
            free(xargv[i]);
        }
    }
    exit(0);
}

int Readline(char *line){ //读取一行
    char ch;
    int n;
    char* p = line;

    n = read(0, &ch, 1);
    while(n){
        if(ch != '\n'){
            *p++ = ch;
            n = read(0, &ch, 1);
        }else{
            *p = 0; //读取到换行符
            break;  //读取一行结束
        }
    }
    // printf("%s\n", line);
    return n;
}

