#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  // your code here.  you should write the secret to fd 2 using write
  // (e.g., write(2, secret, 8)

  // 出现的问题：内存页的前8个字节是存储页链表指针的地方，这里是无法存储其他内容的，所以当读取end的内容时读取不到内容
  // 无法匹配"my very very very secret pw is:"，需要从end+8处读取字符，才能找到secret

  // 遍历分配的内存页，通过"my very very very secret pw is:"对应找到secret所在内存页的地址end
  // secret的地址则为end + 32
  // 将secret保存至字符数组中
  // 将secret写入fd 2

  char match[7] = "secret";
  char word[7];
  word[6] = '\0';

  char *end = sbrk(PGSIZE*32);
  char *limit = end + PGSIZE*31;
  for(int i = 0; i < 31; i++) {
    printf("%d ", i);
    memcpy(word, end+18, 6);
    if(strcmp(word, match) == 0) {
      write(2, end+32, 8);
      break;
    }
    end += PGSIZE;
  }

  if(end == limit) {
    memcpy(word, end+18, 6);
    if(strcmp(word, match) == 0) {
      write(2, end+32, 8);
    }
  }

  exit(0);
}
