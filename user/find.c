#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

    // find 
    // 递归遍历文件树，在每一个文件夹中的一项与目标文件名比较是否相同
    // 需要传入的参数为3，程序名称，其余一个是文件夹名称，一个是目标文件名称
    /* 需要注意的点： 
    ** 1.递归遍历文件树是否需要考虑基础base情况避免无限递归
    ** 2.如何获取文件名
    ** 3.如何获取文件类型
    ** 4.不能递归遍历 . 和 .. 文件路径
    ** 5.函数头在前面的，与.h文件中的函数头的格式不一样
    */
void find(char* path, char* filename);
char* getname(char *path);

int main(int argc, char *argv[]){
    if(argc < 3){
        fprintf(2, "usage: find files in a directory...\n");
        exit(1);
    }
    
    find(argv[1], argv[2]);
    exit(0);
}

char*
getname(char *path)
{
    char *p;
    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    return p;
}

void find(char* path, char* filename){
    //打开文件路径，查看文件类型，判断是否为文件夹类型
    //若为文件夹类型，遍历文件夹中的每一项entry
    //若entry为文件夹类型，则继续递归遍历
    //若不是则比较entry文件名与目标名称是否相等
    //若相等，则print 出文件路径。若不相等则不做处理

    int fd;
    struct stat st;
    struct dirent de;
    char buf[512];
    char *p;

    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type) {
    case T_FILE:
    case T_DEVICE:
        if(strcmp(getname(path), filename) == 0){
            printf("%s\n", path);
        }
        break;
    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
            printf("find: path too long\n");
            break;
        }

        // 为文件夹下的每一项建立路径名称，先建立路径头
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0){
                continue;
            }
            
            // Don't recurse into "." and "..".
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
                continue;
            }
            
            memmove(p, de.name, DIRSIZ); 
            p[DIRSIZ] = 0; // 此时buf 保存着de的路径    //新建立的字符串需要0结尾

            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }

            switch(st.type){
            case T_FILE:
            case T_DEVICE:
                if(strcmp(de.name, filename) == 0){
                    printf("%s\n", buf);
                }
                break;
            case T_DIR:
                find(buf, filename); //递归寻找
                break;
            }
        }
        break;
    }
    close(fd);

}