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
} bcache;

#define bucket_num 5
struct {
  struct spinlock lock;
  struct buf head;
} buckets[bucket_num];

void
binit(void)
{
  initlock(&bcache.lock, "bcache");
  for (int i=0; i<bucket_num; ++i) {
    initlock(&buckets[i].lock, "bcache"+(i+'0'));
    buckets[i].head.prev = &buckets[i].head;
    buckets[i].head.next = &buckets[i].head;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int b_id = blockno % bucket_num;
  acquire(&buckets[b_id].lock);
  for (struct buf* b = buckets[b_id].head.next; b != &buckets[b_id].head; b = b->next) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&buckets[b_id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.lock);
  for (int i=0; i<NBUF; ++i) {
    struct buf *b = &bcache.buf[i];
    if(b->refcnt == 0) {
      b->refcnt = 1;
      b->blockno = blockno;
      b->dev = dev;
      b->valid = 0;
      release(&bcache.lock);
      // add b to bucket list
      b->prev = buckets[b_id].head.next->prev;
      buckets[b_id].head.next->prev = b;
      b->next = buckets[b_id].head.next;
      buckets[b_id].head.next = b;
      release(&buckets[b_id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock);
  release(&buckets[b_id].lock);
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int b_id = b->blockno % bucket_num;
  acquire(&buckets[b_id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
  }
  release(&buckets[b_id].lock);
}

void
bpin(struct buf *b) {
  // acquire(&bcache.lock);
  int b_id = b->blockno % bucket_num;
  acquire(&buckets[b_id].lock);
  b->refcnt++;
  release(&buckets[b_id].lock);
  // release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  // acquire(&bcache.lock);
  int b_id = b->blockno % bucket_num;
  acquire(&buckets[b_id].lock);
  b->refcnt--;
  release(&buckets[b_id].lock);
  // release(&bcache.lock);
}


