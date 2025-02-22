struct Ref {
	Ref *next;
	u32 addr;
};

struct Symbol {
	Symbol     *next;
	u32        addr;
	const char *name;
	Ref        *refs;
	int        resolved;
};

struct Assembler {
	Symbol *syms;
	u8     *b;
	u32    cap;
	u32    ip;
};

void clear(Assembler &a);
void push_byte(Assembler &a, u8 b);
void push_bytes(Assembler &a, u64 v, u8 count);
void label(Assembler &a, const char *name);
void push_label_offset(Assembler &a, const char *name);
