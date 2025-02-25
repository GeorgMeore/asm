struct Page {
	Page *next;
	u8   *data;
};

struct Arena {
	Page *p;
};

void *alloc(Arena &a, u32 size, u16 align = 8);
void reset(Arena &a);
