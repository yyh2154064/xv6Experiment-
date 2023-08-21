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


//首先是多内存池创建和锁管理
//该部分我们只需复用单一内存池，并在初始化时初始化所有锁即可
struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

//用cpuid来分配内存池
struct kmem kmem_sum[NCPU];

void
kinit()
{
  int i;
  //初始化所有锁
  for(i = 0; i < NCPU; ++i)
  {
    initlock(&(kmem_sum[i].lock), "kmem");
  }
  //这里不用修改，默认将内存分配给运行这一函数的cpu
  freerange(end, (void*)PHYSTOP);
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
//kfree本来是将内存块释放到单一内存块中，修改后我们需要放回该cpu对应的内存池

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
  int id = cpuid();
  //获取锁以保证内存池的使用安全
  acquire(&(kmem_sum[id].lock));
  r->next = kmem_sum[id].freelist;
  kmem_sum[id].freelist = r;
  release(&(kmem_sum[id].lock));
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
//内存块获取
void *
kalloc(void)
{
  struct run *r;

  push_off();

  int id = cpuid();

  acquire(&(kmem_sum[id].lock));
    
  r = kmem_sum[id].freelist;
    
  //先从自己的内存池中寻找可用的内存块
  if(r)
  {
    kmem_sum[id].freelist = r->next;
  }
  else	//没有空闲块则从其他内存池中寻找
  {
    int i;
    for(i = 0; i < NCPU; ++i)
    {
      if(i == id) continue;
	  
      //寻找前记得上锁
      acquire(&(kmem_sum[i].lock));

      r = kmem_sum[i].freelist;
      if(r)
      {
        kmem_sum[i].freelist = r->next;
        release(&(kmem_sum[i].lock));
        break;
      }

      release(&(kmem_sum[i].lock));

    }
  }
  release(&(kmem_sum[id].lock));
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

