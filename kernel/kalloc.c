// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end, int i);
void kfree_in_init(void *pa, int id);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock[NCPU];
  struct run *freelist[NCPU]; // per-CPU freelist
} kmem;

void
kinit()
{
  int i, j;
 
  for(i = 0; i < NCPU; i++){
    initlock(&kmem.lock[i], "kmem");
  }
  
  freerange(end, (void*)(KERNBASE + MEMPERCPU), 0);
  for(j = 1; j < NCPU; j++){
    void* start = (void*)(KERNBASE + j * MEMPERCPU);
    void* stop = (void*)(KERNBASE + (j + 1) * MEMPERCPU);  
    freerange(start, stop, j);
  }
}

void
freerange(void *pa_start, void *pa_end, int i)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_in_init(p, i);
}

void
kfree_in_init(void *pa, int id)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  r->next = kmem.freelist[id];
  kmem.freelist[id] = r;
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int id;

  push_off();
  id = cpuid();
  

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock[id]);
  r->next = kmem.freelist[id];
  kmem.freelist[id] = r;
  release(&kmem.lock[id]);
  
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int id;

  push_off();
  id = cpuid();
  
  acquire(&kmem.lock[id]);
  r = kmem.freelist[id];
  if(r){
    kmem.freelist[id] = r->next;
    release(&kmem.lock[id]);
  }else{  // this cpu has no free pages
    release(&kmem.lock[id]);
    for(int i = 0; i < NCPU; i++){
      if(i == id){
        // skip this cpu
        continue;
      }
      acquire(&kmem.lock[i]);
      r = kmem.freelist[i];
      if(r){
        kmem.freelist[i] = r->next;
        release(&kmem.lock[i]);
        break; // found a free page in another cpu
      }
      release(&kmem.lock[i]);
    }
  }
  
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
