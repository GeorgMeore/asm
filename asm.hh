struct Ref {
	Ref *next;
	u32 pos;
	u32 sub, div;
	u8  len, off;
};

struct Symbol {
	Symbol     *next;
	u32        addr;
	const char *name;
	Ref        *refs;
	int        resolved;
};

enum AsmError {
	ErrDupLabel,
	ErrOverflow,
	ErrPatchParam,
};

struct Assembler {
	Arena  tmp;
	Symbol *syms;
	u8     *code;
	u32    ip;
	int    err;
};

void clear(Assembler &a);
void push_byte(Assembler &a, u8 b);
void push_bytes(Assembler &a, u64 v, u8 count);
void label(Assembler &a, const char *name);
void label_ref(Assembler &a, const char *name, u32 pos, u32 sub, u32 div, u8 len, u8 off);
