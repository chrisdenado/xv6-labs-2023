// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  for (int i=0; i<NCPU; ++i)
    initlock(&kmem[i].lock, "kmem"+(i+'0'));
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  uint64 len = ((uint64)pa_end - (uint64)p) / PGSIZE / NCPU;
  for (uint64 i=0; i<NCPU; ++i) {
    uint64 offset = (uint64)p + i*len*PGSIZE;
    uint64 end = (uint64)p + (i+1)*len*PGSIZE;
    for (uint64 pa=offset; pa<(uint64)pa_end && pa<end; pa+=PGSIZE) {
      memset((void*)pa, 1, PGSIZE);
      struct run* r = (struct run*)pa;
      r->next = kmem[i].freelist;
      kmem[i].freelist = r;
    }
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  int cpu_id = safe_cpuid();
  acquire(&kmem[cpu_id].lock);
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  release(&kmem[cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int cpu_id = safe_cpuid();
  acquire(&kmem[cpu_id].lock);
  r = kmem[cpu_id].freelist;
  if(r) {
    kmem[cpu_id].freelist = r->next;
    release(&kmem[cpu_id].lock);
  }
  else { // steal
    release(&kmem[cpu_id].lock); // first release lock
    for (int i=cpu_id+1; i<cpu_id+NCPU+1; ++i) {
      acquire(&kmem[i%NCPU].lock);
      if (kmem[i%NCPU].freelist) {
        r = kmem[i%NCPU].freelist;
        kmem[i%NCPU].freelist = r->next;
      }
      release(&kmem[i%NCPU].lock);
      if (r)
        break;
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
