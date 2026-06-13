#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>

#include "types.hh"
#include "arena.hh"

static const u32 PageSize = 16*((u32)1 << 20);

static u32 free_size(Page *p)
{
	return p ? PageSize - ((u64)p->data - (u64)p) : 0;
}

void *alloc(Arena &a, u32 size, u16 align)
{
	assert (size + align <= PageSize - sizeof(Page));
	if (free_size(a.p) < size + align) {
		Page *p = (Page *)mmap(0, PageSize, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		assert(p != MAP_FAILED);
		p->next = a.p;
		p->data = (u8 *)p + sizeof(Page);
		a.p = p;
	}
	u64 d = (u64)a.p->data % align;
	if (d)
		a.p->data += align - d;
	void *ptr = a.p->data;
	a.p->data += size;
	return (void *)ptr;
}

void reset(Arena &a)
{
	for (Page *p = a.p; p; p = a.p) {
		a.p = p->next;
		munmap(p, PageSize);
	}
}
