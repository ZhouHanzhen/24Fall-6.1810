#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_mmap(void)
{
  int ret;
  uint64 addr, start, old;
  int len, prot, flags, fd, offset;
  argaddr(0, &addr);
  argint(1, &len);
  argint(2, &prot);
  argint(3, &flags);
  argint(4, &fd);
  argint(5, &offset);

  
  struct proc *p = myproc();
  struct vma *vma = vmaalloc();
  if(vma == 0){
    return -1;  // No available VMA structures
  }
  vma->addr = addr;
  vma->len = len;
  vma->prot = prot;
  vma->flags = flags;
  vma->fd = fd;
  vma->offset = offset;

  filedup(p->ofile[fd]);
  vma->fl = p->ofile[fd];

  old = p->unused;
  start = PGROUNDDOWN(p->unused - len);   // start address of the mapped region
  p->unused = start;
  vma->start = start;

  ret = addvma(vma);
  if(ret < 0){
    fileclose(p->ofile[fd]);
    vmafree(vma);
    p->unused = old;
    return -1;
  }
  return start;
}

uint64 
sys_munmap(void)
{
  uint64 addr;
  int len;
  argaddr(0, &addr);
  argint(1, &len);

  // checke the address range whether is in the mapped region
  struct proc* p = myproc();
  struct vma* v;
  for(int i = 0; i < NVMA; i++){
    v = p->mappedf[i];
    if(v == 0){
      continue;
    }
    if(v->ref == 0){
      continue;
    }
    if(addr >= v->start && (addr + len) <= (v->start + v->len)){
      // remove mmap mappings in the indicated address range
      uvmunmap(p->pagetable, addr, len / PGSIZE, 1);

      // update the vma structure 
      v->start = addr + len;
      v->len = v->len - len;
      p->unused = v->start;
      if(v->len == 0){  
        // if the whole mapped region of the vma structure is removed
        // free the vma structure
        fileclose(v->fl);
        vmafree(v);
        p->mappedf[i] = 0;
      }
    }
  }
  return -1;
}
