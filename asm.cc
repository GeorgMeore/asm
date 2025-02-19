#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/user.h>
#include <sys/mman.h>

typedef unsigned char u8;
typedef unsigned int  u32;
typedef unsigned long u64;
typedef signed   char s8;
typedef signed   int  s32;
typedef signed   long s64;

static_assert(sizeof(u8)  == 1);
static_assert(sizeof(s8)  == 1);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(s32) == 4);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(s64) == 8);

static void panic(const char *msg)
{
	fprintf(stderr, "panic: %s\n", msg);
	exit(1);
}

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

static Symbol *sym(const char *name)
{
	Symbol *s = (Symbol *)malloc(sizeof(*s));
	memset(s, 0, sizeof(*s));
	s->name = name;
	return s;
}

static void symfree(Symbol *s)
{
	free(s);
}

static void addref(Symbol *s, u32 addr)
{
	Ref *r = (Ref *)malloc(sizeof(*r));
	r->next = s->refs;
	r->addr = addr;
	s->refs = r;
}

struct Assembler {
	Symbol *symtab;
	char   *b;
	u32    cap;
	u32    ip;
};

Assembler *asmb(void)
{
	Assembler *a = (Assembler *)malloc(sizeof(*a));
	memset(a, 0, sizeof(*a));
	a->cap = PAGE_SIZE;
	a->b = (char *)malloc(a->cap);
	return a;
}

void asmfree(Assembler *a)
{
	while (a->symtab) {
		Symbol *s = a->symtab;
		a->symtab = s->next;
		symfree(s);
	}
	free(a->b);
	free(a);
}

static void patchbytes(Assembler *a, u32 addr, u64 v, u8 bytes)
{
	for (u32 i = 0; i < bytes; i++)
		a->b[addr+i] = (v & (0xFFLU<<(i*8)))>>(i*8);
}

static void pushbytes(Assembler *a, u64 v, u8 bytes)
{
	if (a->ip + bytes > a->cap) {
		a->cap += PAGE_SIZE;
		a->b = (char *)realloc(a->b, a->cap);
	}
	patchbytes(a, a->ip, v, bytes);
	a->ip += bytes;
}

static void pushbyte(Assembler *a, u8 b)
{
	pushbytes(a, b, 1);
}

static Symbol *getsym(Assembler *a, const char *name)
{
	Symbol *s = a->symtab;
	while (s && strcmp(name, s->name))
		s = s->next;
	return s;
}

static Symbol *addsym(Assembler *a, const char *name)
{
	Symbol *s = sym(name);
	s->next = a->symtab;
	a->symtab = s;
	return s;
}

void label(Assembler *a, const char *name)
{
	Symbol *s = getsym(a, name);
	if (!s)
		s = addsym(a, name);
	if (s->resolved)
		panic("Assembly error: label already defined");
	s->resolved = 1;
	s->addr = a->ip;
	while (s->refs) {
		Ref *r = s->refs;
		s->refs = r->next;
		patchbytes(a, r->addr, s->addr - (r->addr+4), 4);
		free(r);
	}
}

static void pushlabeloffset(Assembler *a, const char *name)
{
	Symbol *s = getsym(a, name);
	if (!s)
		s = addsym(a, name);
	if (s->resolved) {
		pushbytes(a, s->addr - (a->ip+4), 4);
	} else {
		addref(s, a->ip);
		pushbytes(a, 0, 4);
	}
}

enum RCode {
	A   = 0b0000,
	C   = 0b0001,
	D   = 0b0010,
	B   = 0b0011,
	SP  = 0b0100,
	BP  = 0b0101,
	R8  = 0b1000,
	R9  = 0b1001,
	R10 = 0b1010,
	R11 = 0b1011,
	R12 = 0b1100,
	R13 = 0b1101,
	R14 = 0b1110,
	R15 = 0b1111,
};

struct R {
	u8 code;
	u8 size;
};

static u8   size(R r) { return r.size; }
static u8   code(R r) { return r.code & 0b111; }
static bool isnew(R r) { return r.code & 0b1000; }

struct I {
	R  index;
	u8 scale;
};

I operator*(R r, u8 scale)
{
	if (scale < 1 || scale > 8 || scale&(scale - 1))
		panic("Invalid index/base expression: scale must be 1,2,4 or 8");
	if (r.code == SP)
		panic("Invalid index/base expression: cannot use RSP as an index");
	return {r, scale};
}

struct PTR {
	s32 offset;
	R   base;
	R   index;
	u8  scale;
};

static u8 offsetsize(PTR p)
{
	if (p.offset < -128 || p.offset > 127)
		return 4;
	if (p.offset || code(p.base) == 0b101)
		return 1;
	return 0;
}

static u8 size(PTR p) { return p.base.code ? p.base.size : p.index.size; }

PTR ptr(R base, I i, s32 offset = 0)
{
	if ((size(base) && size(base) < 32) || (size(i.index) && size(i.index) < 32))
		panic("Invalid index/base expression: cannot use 8 or 16 bit registers");
	if (size(base) && size(i.index) && size(base) != size(i.index))
		panic("Invalid index/base expression: size mismatch");
	return {offset, base, i.index, i.scale};
}

PTR ptr(R base, s32 offset = 0) { return ptr(base, {}, offset); }
PTR ptr(I i, s32 offset = 0) { return ptr({}, i, offset); }

const R rax = {A,   64}, eax  = {A,   32}, ax   = {A,   16}, al   = {A,   8};
const R rcx = {C,   64}, ecx  = {C,   32}, cx   = {C,   16}, cl   = {C,   8};
const R rdx = {D,   64}, edx  = {D,   32}, dx   = {D,   16}, dl   = {D,   8};
const R rbx = {B,   64}, ebx  = {B,   32}, bx   = {B,   16}, bl   = {B,   8};
const R rsp = {SP,  64}, esp  = {SP,  32}, sp   = {SP,  16}, spl  = {SP,  8};
const R rbp = {BP,  64}, ebp  = {BP,  32}, bp   = {BP,  16}, bpl  = {BP,  8};
const R r8  = {R8,  64}, r8d  = {R8,  32}, r8w  = {R8,  16}, r8b  = {R8,  8};
const R r9  = {R9,  64}, r9d  = {R9,  32}, r9w  = {R9,  16}, r9b  = {R9,  8};
const R r10 = {R10, 64}, r10d = {R10, 32}, r10w = {R10, 16}, r10b = {R10, 8};
const R r11 = {R11, 64}, r11d = {R11, 32}, r11w = {R11, 16}, r11b = {R11, 8};
const R r12 = {R12, 64}, r12d = {R12, 32}, r12w = {R12, 16}, r12b = {R12, 8};
const R r13 = {R13, 64}, r13d = {R13, 32}, r13w = {R13, 16}, r13b = {R13, 8};
const R r14 = {R14, 64}, r14d = {R14, 32}, r14w = {R14, 16}, r14b = {R14, 8};
const R r15 = {R15, 64}, r15d = {R15, 32}, r15w = {R15, 16}, r15b = {R15, 8};

enum REX {
	REXW = 0b01001000,
	REXR = 0b01000100,
	REXX = 0b01000010,
	REXB = 0b01000001,
};

enum Mod {
	ModDisp0  = 0b00,
	ModDisp1  = 0b01,
	ModDisp4  = 0b10,
	ModDirect = 0b11,
};

static Mod mod(u8 offsetsize)
{
	switch (offsetsize) {
		case 0: return ModDisp0;
		case 1: return ModDisp1;
		default: // should never happen
		case 4: return ModDisp4;
	}
}

static u8 modrm(Mod mod, u8 reg, u8 rm) { return mod<<6 | reg<<3 | rm; }

enum Scale {
	Scale1 = 0b00,
	Scale2 = 0b01,
	Scale4 = 0b10,
	Scale8 = 0b11,
};

static Scale scale(u8 scale)
{
	switch (scale) {
		case 1: return Scale1;
		case 2: return Scale2;
		case 4: return Scale4;
		default: // should never happen
		case 8: return Scale8;
	}
}

static u8 sib(Scale scale, u8 index, u8 base) { return scale<<6 | index<<3 | base; }

static void ptrinst(Assembler *a, PTR p, R r, u8 op)
{
	if (size(p) == 32)
		pushbyte(a, 0x67);
	if (size(r) == 16)
		pushbyte(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	rex |= isnew(r)*REXR | isnew(p.index)*REXX | isnew(p.base)*REXB;
	if (rex)
		pushbyte(a, rex);
	pushbyte(a, op);
	if (!size(p.base)) {
		pushbyte(a, modrm(ModDisp0, code(r), 0b100));
		pushbyte(a, sib(scale(p.scale), code(p.index), 0b101));
		pushbytes(a, p.offset, 4);
		return;
	}
	u8 osz = offsetsize(p);
	if (!size(p.index) && code(p.base) != 0b100) {
		pushbyte(a, modrm(mod(osz), code(r), code(p.base)));
	} else {
		pushbyte(a, modrm(mod(osz), code(r), 0b100));
		if (size(p.index))
			pushbyte(a, sib(scale(p.scale), code(p.index), code(p.base)));
		else
			pushbyte(a, sib(Scale1, 0b100, code(p.base)));
	}
	pushbytes(a, p.offset, osz);
}

static void rrinst(Assembler *a, R r1, R r2, u8 op)
{
	if (size(r1) != size(r2))
		panic("Invalid mov: register size mismatch");
	if (size(r1) == 16)
		pushbyte(a, 0x66);
	u8 rex = 0;
	if (size(r1) == 64)
		rex |= REXW;
	rex |= isnew(r1)*REXR | isnew(r2)*REXB;
	if (rex)
		pushbyte(a, rex);
	pushbyte(a, op);
	pushbyte(a, modrm(ModDirect, code(r1), code(r2)));
}

void mov(Assembler *a, PTR dst, R src) { ptrinst(a, dst, src, 0x88 + (size(src) > 8)); }
void mov(Assembler *a, R dst, PTR src) { ptrinst(a, src, dst, 0x8a + (size(dst) > 8)); }
void mov(Assembler *a, R dst, R src) { rrinst(a, src, dst, 0x88 + (size(src) > 8)); }

void mov(Assembler *a, void *dst, R src)
{
	if (code(src) != A)
		panic("Invalid mov: expected the A register");
	pushbyte(a, 0xA3);
	pushbytes(a, (u64)dst, 8);
}

void mov(Assembler *a, R dst, u64 src)
{
	if (size(dst) == 16)
		pushbyte(a, 0x66);
	u8 rex = 0;
	if (size(dst) == 64)
		rex |= REXW;
	if (isnew(dst))
		rex |= REXB;
	if (rex)
		pushbyte(a, rex);
	pushbyte(a, size(dst) == 8 ? 0xB0 : 0xB8 + code(dst));
	pushbytes(a, src, size(dst)/8);
}

void lea(Assembler *a, R dst, PTR src)
{
	if (size(dst) == 8)
		panic("Invalid lea: cannot use 8 bit dst");
	ptrinst(a, src, dst, 0x8d);
}

void add(Assembler *a, R dst, R src) { rrinst(a, src, dst, 0x0 + (size(src) > 8)); }
void ori(Assembler *a, R dst, R src) { rrinst(a, src, dst, 0x8 + (size(src) > 8)); }
void andi(Assembler *a, R dst, R src) { rrinst(a, src, dst, 0x20 + (size(src) > 8)); }

enum Cond {
	AE = 0x3,          // above or equal (CF=0)
	Z = 0x4, EQ = 0x4, // equal (ZF=1)
	AB = 0x7,          // above (CF=0 and ZF=0)
};

void jcc(Assembler *a, Cond c, const char *l)
{
	pushbyte(a, 0x0F);
	pushbyte(a, 0x80 + c);
	pushlabeloffset(a, l);
}

void jmp(Assembler *a, R r)
{
	if (size(r) != 64)
		panic("Invalid jump: not a 64 bit register");
	if (isnew(r))
		pushbyte(a, REXB);
	pushbyte(a, 0xFF);
	pushbyte(a, modrm(ModDirect, 0b100, code(r)));
}

void push(Assembler *a, R r)
{
	if (size(r) != 64)
		panic("Invalid push: not a 64 bit register");
	if (isnew(r))
		pushbyte(a, REXB);
	pushbyte(a, 0x50 + code(r));
}

void pop(Assembler *a, R r)
{
	if (size(r) != 64)
		panic("Invalid pop: not a 64 bit register");
	if (isnew(r))
		pushbyte(a, REXB);
	pushbyte(a, 0x58 + code(r));
}

void ret(Assembler *a) { pushbyte(a, 0xc3); }

// TODO: cmov, call

void test(Assembler *a)
{
	mov(a, r8, 123);
	mov(a, r8d, 123);
	mov(a, r8w, 123);
	mov(a, r8b, 123);
	mov(a, ptr(r12), rax);
	mov(a, ptr(r12), r8w);
	mov(a, ptr(rsp, rbp*2, -1), rax);
	mov(a, rax, ptr(rsp, rbp*2, -1));
	mov(a, ax, ptr(esp, ebp*2, -1));
	mov(a, al, ptr(esp, ebp*2, -1));
	mov(a, ptr(r8, r9*2, 5), r10b);
	mov(a, rbx, r8);
	mov(a, bx, ax);
	mov(a, bl, al);
	mov(a, eax, ecx);
	mov(a, r8b, al);
	push(a, rax);
	push(a, r8);
	pop(a, rax);
	lea(a, rax, ptr(rsp, rbp*2, -1));
	jmp(a, rax);
	jmp(a, r12);
label(a, "foo");
	jcc(a, AB, "foo");
	jcc(a, EQ, "foo");
	add(a, rax, rbx);
	add(a, eax, ecx);
	add(a, al, r12b);
	add(a, r13b, r8b);
	ori(a, eax, ecx);
	ori(a, al, r12b);
	ori(a, r13b, r8b);
	andi(a, eax, ecx);
	andi(a, al, r12b);
	andi(a, r13b, r8b);
}

int main(void)
{
	Assembler *a = asmb();
	test(a);
	for (u32 i = 0; i < a->ip; i++)
		putchar(a->b[i]);
	asmfree(a);
	return 0;
}
