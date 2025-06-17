// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct spinlock locks[HTSIZE];
  struct buf heads[HTSIZE];
} bcache;

void
binit(void)
{
  struct buf *b;
  int i, j;

  initlock(&bcache.lock, "bcache");

  // Create HTSIZE buckets of linked list of buffers
  for(i = 0; i < HTSIZE; i++){
    initlock(&bcache.locks[i], "bcache_head");
    bcache.heads[i].next = &bcache.heads[i];
    bcache.heads[i].prev = &bcache.heads[i];
  }

  // hash the NBUF bufs to the HTSIZE head buckets
  i = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    j = i % HTSIZE;
    b->next = bcache.heads[j].next;
    b->prev = &bcache.heads[j];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[j].next->prev = b;
    bcache.heads[j].next = b;
    i++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int i;
  int j;

  i = blockno % HTSIZE;   // i is the hash bucket index for blockno
  // Is the block already cached?
  acquire(&bcache.locks[i]);
  for(b = bcache.heads[i].next; b != &bcache.heads[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.locks[i]); 

  // Not cached.
  // Recycle the unused buffer in hash table buckets.
  for(j = 0; j < HTSIZE; j++){
    acquire(&bcache.locks[j]);
    // Scan the j-th bucket for an unused buffer.
    for(b = bcache.heads[j].next; b != &bcache.heads[j]; b = b->next){
      if(b->refcnt == 0) {  // find a unused buffer
        // If found, either use it or move it to the i-th bucket.
        if(j == i){ // this j buckets is just the block should be hashed to 
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;

          release(&bcache.locks[j]);
          acquiresleep(&b->lock);
          return b;
        }else{    // b should be hashed to i bucket 
          // remove b frome its current j bucket
          b->next->prev = b->prev;
          b->prev->next = b->next;

          // set b
          acquire(&bcache.lock);
          release(&bcache.locks[j]);  // done with bucket j
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          

          // add b to i bucket
          acquire(&bcache.locks[i]);  // change to bucket i
          release(&bcache.lock);
          b->next = bcache.heads[i].next;
          b->prev = &bcache.heads[i];
          bcache.heads[i].next->prev = b;
          bcache.heads[i].next = b;

          release(&bcache.locks[i]);
          acquiresleep(&b->lock);
          return b;
        } //end of moving b to bucket head[i]
      } // end if(b->refcnt == 0)
    }
    // If not found, continue to the next bucket.
    release(&bcache.locks[j]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  int i;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  i = b->blockno % HTSIZE;
  acquire(&bcache.locks[i]);
  b->refcnt--;
  release(&bcache.locks[i]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


