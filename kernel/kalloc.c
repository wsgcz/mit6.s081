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
  for(int i = 0; i < NCPU; i += 1) {
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  struct run *r;
  int cid;
  p = (char*)PGROUNDUP((uint64)pa_start);
  char *start = p;
  int nbytes = ((uint64)pa_end - (uint64)p) / NCPU;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    cid = (p-start) / nbytes;
    memset(p, 1, PGSIZE);
    r = (struct run*)p;
    r->next = kmem[cid].freelist;
    kmem[cid].freelist = r;
  }
}

// Free the page of physical memory pointed at by v,
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
  push_off();
  int cid = cpuid();
  pop_off();
  acquire(&kmem[cid].lock);
  r->next = kmem[cid].freelist;
  kmem[cid].freelist = r;
  release(&kmem[cid].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cid = cpuid();
  pop_off();
  acquire(&kmem[cid].lock);
  r = kmem[cid].freelist;
  if(r) {
    kmem[cid].freelist = r->next;
    release(&kmem[cid].lock);
  }
  else {
    release(&kmem[cid].lock);
    for (int i = 0; i < NCPU; i += 1) {
      if (i == cid)
        continue;
      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      if(r){
        kmem[i].freelist = r->next;
        release(&kmem[i].lock);
        break;
      }
      release(&kmem[i].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
