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
	Symbol *symtab;
	u8     *b;
	u32    cap;
	u32    ip;
};

void clear(Assembler &a);
void pushbyte(Assembler &a, u8 b);
void pushbytes(Assembler &a, u64 v, u8 bytes);
void label(Assembler &a, const char *name);
void pushlabeloffset(Assembler &a, const char *name);
