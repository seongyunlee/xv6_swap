// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
void lru_pop2(struct page *p);
void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};
struct {
  struct spinlock lock;
  int *bitmap;
} swapTable;

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

struct spinlock lru_head_lock;
struct page pages[PHYSTOP/PGSIZE];
struct page *page_lru_head;
struct page *lru_clock_hand;
int num_free_pages;
int num_lru_pages;


// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}
void
swapinit(void){
  initlock(&swapTable.lock,"swaptable");
  initlock(&lru_head_lock,"lru head lock");
  swapTable.bitmap=(int*)kalloc();
  memset(swapTable.bitmap,0,PGSIZE);
}
void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r = (struct run*)0;

try_again:
  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(!r){
    if(kmem.use_lock)
      release(&kmem.lock);
    if(reclaim()>0){
     //cprintf("reclaim success\n");
      goto try_again;
    }
    else{
      cprintf("Out of memory\n");
      return 0;
    }
  }
  if(r)
    kmem.freelist = r->next;
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}
int allocSwapBlock(){
  acquire(&swapTable.lock);
  int *byte = swapTable.bitmap;
  for(int i=0;i<SWAPMAX/8;i++){
    if(*byte==0xFFFFFFFF){
      byte++;
      continue;
    }  
    for(int ind=0;ind<32;ind++){
      if(((1<<ind)&(*byte)) == 0){
        *byte = (*byte | 1<<ind);
        release(&swapTable.lock);
        return (i*32)+ind;
      }
    }
  }
  release(&swapTable.lock);
  return -1;
}

int reclaim(){
  //select victim
  //cprintf("reclaim! the lru length %d\n",num_lru_pages);
  if(!lru_clock_hand) lru_clock_hand = page_lru_head->prev;
  acquire(&lru_head_lock);
  //cprintf("acquire lru head lock\n");

  while(1){
    if(!lru_clock_hand){
      release(&lru_head_lock);
      return 0;
    }
    //clock algorithm;
    //if access bit 0 -> swap out
    //else change it to 0
    pte_t *pte=walkpgdir(lru_clock_hand->pgdir,lru_clock_hand->vaddr,0);
    if((int)pte == 0) continue;
    if((PTE_U & *pte)==0){
      lru_clock_hand=lru_clock_hand->prev;
      continue;
    }
    if((PTE_P & *pte)==0){
      release(&lru_head_lock);
      //cprintf("%x is not present\n",lru_clock_hand->vaddr);
      //panic("not present page");
      return -1;
      }
    if((*pte)&PTE_A){
      //cprintf("%x accses bit set\n",lru_clock_hand->vaddr);
      *pte = (~PTE_A) & (*pte);
    }
    else{
      int blknum = allocSwapBlock();
      if(blknum==-1){
        release(&lru_head_lock);
        return -1;
      }
      uint pa = PTE_ADDR(*pte);
      char *ptr = P2V(pa);
      lru_pop2(lru_clock_hand);
      release(&lru_head_lock);
      swapwrite(ptr,blknum);
      kfree((char*)P2V(pa));
      *pte = PTE_FLAGS(*pte) & ~PTE_P;
      *pte = *pte | (blknum<<12);
      lru_clock_hand=lru_clock_hand->prev;
      break;
    }
    lru_clock_hand=lru_clock_hand->prev;
  }
  return 1;
}
void lru_insert(char* va,pde_t *pgdir,int pa){
  int framenumber = pa/PGSIZE;
  struct page *p = &pages[framenumber];
  //cprintf("insert %d %x\n",pa,(int)p);

  p->vaddr=va;
  p->pgdir=pgdir;
  
  acquire(&lru_head_lock);
  if(!page_lru_head){
    page_lru_head=p;
    p->next=p;
    p->prev=p;
    num_lru_pages++;
  }
  else{
    p->next = page_lru_head;
    p->prev = page_lru_head->prev;
    page_lru_head->prev->next=p;
    page_lru_head->prev = p;
    page_lru_head=p;
    num_lru_pages++;
  }
  release(&lru_head_lock);
}
void lru_pop2(struct page *p){
  struct page *cur = page_lru_head;
  for(int i=0;i<num_lru_pages;i++){
    if(cur==p){
      page_lru_head=cur->prev;
      cur->prev->next=cur->next;
      cur->next->prev=cur->prev;
      num_lru_pages--;
      return;
    }
    cur=cur->next;
  }
}
void lru_pop(char* va,pde_t *pgdir,int pa){
  int framenumber = pa/PGSIZE;
  struct page *p = &pages[framenumber];
  //cprintf("pop %d %x\n",pa,(int)p);
  acquire(&lru_head_lock);
  struct page *cur = page_lru_head;
  for(int i=0;i<num_lru_pages;i++){
    if(cur==p){
      cur->prev->next=cur->next;
      cur->next->prev=cur->prev;
      page_lru_head=cur->prev;
      num_lru_pages--;
      release(&lru_head_lock);
      return;
    }
    cur=cur->next;
  }
  release(&lru_head_lock);
}
