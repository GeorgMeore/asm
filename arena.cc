#include <sys/mman.h>
#include <stdio.h>

#include "types.hh"
#include "arena.hh"

static const u32 PageSize = 16*((u32)1 << 20);

static u32 free(Page *p)
{
	return p ? (u64)p->data - (u64)p : 0;
}

void *alloc(Arena &a, u32 size, u16 align)
{
	if (size + align > PageSize - sizeof(Page))
		return 0;
	if (free(a.p) < size + align) {
		Page *p = (Page *)mmap(0, PageSize, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		if (p == MAP_FAILED)
			return 0;
		p->next = a.p;
		p->data = (u8 *)p + sizeof(Page);
		a.p = p;
	}
	u64 d = (u64)a.p->data % align;
	if (d)
		a.p->data += d;
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
