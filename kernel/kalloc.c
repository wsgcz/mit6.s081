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
} kmem;

struct {
  struct spinlock lock;
  int reference_count[FREEPAGES];
} kref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock, "kref");
  for (int i = 0; i < FREEPAGES; i += 1) {
    kref.reference_count[i] = 1;
  }
  freerange(end, (void*)PHYSTOP);
}

void kref_dec(uint64 pa) {
  acquire(&kref.lock);
  kref.reference_count[PA_REF(pa)] -= 1;
  release(&kref.lock);
}
void kref_inc(uint64 pa) {
  acquire(&kref.lock);
  kref.reference_count[PA_REF(pa)] += 1;
  release(&kref.lock);
}
void kref_setone(uint64 pa) {
  acquire(&kref.lock);
  kref.reference_count[PA_REF(pa)] = 1;
  release(&kref.lock);
}
int kref_get(uint64 pa) {
  int ref = 0;
  acquire(&kref.lock);
  ref = kref.reference_count[PA_REF(pa)];
  release(&kref.lock);
  return ref;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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
  // kref_dec((uint64)pa);
  // if (kref_get((uint64)pa) != 0) {
  //   return;
  // }
  acquire(&kref.lock);
  uint64 pn = PA_REF((uint64)pa);
  if (kref.reference_count[pn] < 1)
    panic("kfree ref");
  kref.reference_count[pn] -= 1;
  int tmp = kref.reference_count[pn];
  release(&kref.lock);
  if (tmp > 0) 
    return;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE);  // fill with junk
    kref_setone((uint64)r); //bug
  }
  return (void*)r;
}
