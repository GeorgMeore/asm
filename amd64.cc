#include <stdio.h>
#include <stdlib.h>

#include "types.hh"
#include "asm.hh"
#include "amd64.hh"

static void panic(const char *msg)
{
	fprintf(stderr, "panic: Instruction error: %s\n", msg);
	exit(1);
}

enum RCode {
	A   = 0b0000,
	C   = 0b0001,
	D   = 0b0010,
	B   = 0b0011,
	SP  = 0b0100,
	BP  = 0b0101,
	SI  = 0b0110,
	DI  = 0b0111,
	R8  = 0b1000,
	R9  = 0b1001,
	R10 = 0b1010,
	R11 = 0b1011,
	R12 = 0b1100,
	R13 = 0b1101,
	R14 = 0b1110,
	R15 = 0b1111,
};

const R rax = {A,   64}, eax  = {A,   32}, ax   = {A,   16}, al   = {A,   8};
const R rcx = {C,   64}, ecx  = {C,   32}, cx   = {C,   16}, cl   = {C,   8};
const R rdx = {D,   64}, edx  = {D,   32}, dx   = {D,   16}, dl   = {D,   8};
const R rbx = {B,   64}, ebx  = {B,   32}, bx   = {B,   16}, bl   = {B,   8};
const R rsp = {SP,  64}, esp  = {SP,  32}, sp   = {SP,  16}, spl  = {SP,  8};
const R rbp = {BP,  64}, ebp  = {BP,  32}, bp   = {BP,  16}, bpl  = {BP,  8};
const R rsi = {SI,  64}, esi  = {SI,  32}, si   = {SI,  16}, sil  = {SI,  8};
const R rdi = {DI,  64}, edi  = {DI,  32}, di   = {DI,  16}, dil  = {DI,  8};
const R r8  = {R8,  64}, r8d  = {R8,  32}, r8w  = {R8,  16}, r8b  = {R8,  8};
const R r9  = {R9,  64}, r9d  = {R9,  32}, r9w  = {R9,  16}, r9b  = {R9,  8};
const R r10 = {R10, 64}, r10d = {R10, 32}, r10w = {R10, 16}, r10b = {R10, 8};
const R r11 = {R11, 64}, r11d = {R11, 32}, r11w = {R11, 16}, r11b = {R11, 8};
const R r12 = {R12, 64}, r12d = {R12, 32}, r12w = {R12, 16}, r12b = {R12, 8};
const R r13 = {R13, 64}, r13d = {R13, 32}, r13w = {R13, 16}, r13b = {R13, 8};
const R r14 = {R14, 64}, r14d = {R14, 32}, r14w = {R14, 16}, r14b = {R14, 8};
const R r15 = {R15, 64}, r15d = {R15, 32}, r15w = {R15, 16}, r15b = {R15, 8};

static u8   size(R r) { return r.size; }
static u8   code(R r) { return r.code & 0b111; }
static bool isnew(R r) { return r.code & 0b1000; }

I operator*(R r, u8 scale)
{
	if (scale < 1 || scale > 8 || scale&(scale - 1))
		panic("invalid index/base expression: scale must be 1,2,4 or 8");
	if (r.code == SP)
		panic("invalid index/base expression: cannot use RSP as an index");
	return {r, scale};
}

static u8 offsetsize(PTR p)
{
	if (p.offset < -128 || p.offset > 127)
		return 4;
	if (p.offset || code(p.base) == 0b101)
		return 1;
	return 0;
}

static u8 size(PTR p) { return p.base.code ? p.base.size : p.index.size; }

PTR ptr(R base, I i, s32 offset)
{
	if ((size(base) && size(base) < 32) || (size(i.index) && size(i.index) < 32))
		panic("invalid index/base expression: cannot use 8 or 16 bit registers");
	if (size(base) && size(i.index) && size(base) != size(i.index))
		panic("invalid index/base expression: size mismatch");
	return {offset, base, i.index, i.scale};
}

PTR ptr(R base, s32 offset) { return ptr(base, {}, offset); }
PTR ptr(I i, s32 offset) { return ptr({}, i, offset); }
PTR ptr(s32 offset) { return ptr({}, {}, offset); }

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
		default: panic("invalid offset size"); // fallthrough
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
		default: panic("invalid scale"); // fallthrough
		case 8: return Scale8;
	}
}

static u8 sib(Scale scale, u8 index, u8 base)
{
	return scale<<6 | index<<3 | base;
}

static void push_mod_sib_offset(Assembler &a, u8 reg, PTR p)
{
	if (!size(p.base)) {
		push(a, modrm(ModDisp0, reg, 0b100));
		if (size(p.index))
			push(a, sib(scale(p.scale), code(p.index), 0b101));
		else
			push(a, sib(Scale1, 0b100, 0b101));
		push(a, p.offset, 4);
		return;
	}
	u8 osz = offsetsize(p);
	if (!size(p.index) && code(p.base) != 0b100) {
		push(a, modrm(mod(osz), reg, code(p.base)));
	} else {
		push(a, modrm(mod(osz), reg, 0b100));
		if (size(p.index))
			push(a, sib(scale(p.scale), code(p.index), code(p.base)));
		else
			push(a, sib(Scale1, 0b100, code(p.base)));
	}
	push(a, p.offset, osz);
}

static void push_prefix(Assembler &a, R r, PTR p)
{
	if (size(p) == 32)
		push(a, 0x67);
	if (size(r) == 16)
		push(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	rex |= isnew(r)*REXR | isnew(p.index)*REXX | isnew(p.base)*REXB;
	if (rex)
		push(a, rex);
}

static void inst(Assembler &a, R r, PTR rm, u8 op)
{
	push_prefix(a, r, rm);
	push(a, op + (size(r) > 8));
	push_mod_sib_offset(a, code(r), rm);
}

static void push_prefix(Assembler &a, R r, R rm)
{
	if (size(r) == 16)
		push(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	rex |= isnew(r)*REXR | isnew(rm)*REXB;
	if (rex)
		push(a, rex);
}

static void inst(Assembler &a, R r, R rm, u8 op)
{
	if (size(r) != size(rm))
		panic("register-register operation size mismatch");
	push_prefix(a, r, rm);
	push(a, op + (size(r) > 8));
	push(a, modrm(ModDirect, code(r), code(rm)));
}

static void push_prefix(Assembler &a, R r)
{
	if (size(r) == 16)
		push(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	if (isnew(r))
		rex |= REXB;
	if (rex)
		push(a, rex);
}

static void inst(Assembler &a, R dst, u8 src, u8 op)
{
	push_prefix(a, dst);
	push(a, op + (size(dst) > 8));
	push(a, modrm(ModDirect, src, code(dst)));
}

void mov(Assembler &a, PTR dst, R src) { inst(a, src, dst, 0x88); }
void mov(Assembler &a, R dst, PTR src) { inst(a, dst, src, 0x8a); }
void mov(Assembler &a, R dst, R src) { inst(a, src, dst, 0x88); }

void mov(Assembler &a, R dst, u64 src)
{
	push_prefix(a, dst);
	push(a, size(dst) == 8 ? 0xb0 : 0xb8 + code(dst));
	push(a, src, size(dst)/8);
}

void mov(Assembler &a, R dst, void *src)
{
	if (dst.code != A)
		panic("mov: A register expected");
	push_prefix(a, dst);
	push(a, 0xa0 + (size(dst) > 8));
	push(a, (u64)src, 8);
}

void mov(Assembler &a, void *dst, R src)
{
	if (src.code != A)
		panic("mov: A register expected");
	push_prefix(a, src);
	push(a, 0xa2 + (size(src) > 8));
	push(a, (u64)dst, 8);
}

void cmov(Assembler &a, Cond c, R dst, R src)
{
	if (size(src) != size(dst))
		panic("register-register operation size mismatch");
	if (size(src) == 8)
		panic("cmov: cannot use 8-bit registers");
	push_prefix(a, dst, src);
	push(a, 0x0f);
	push(a, 0x40 + c);
	push(a, modrm(ModDirect, code(dst), code(src)));
}

void cmov(Assembler &a, Cond c, R dst, PTR src)
{
	if (size(src) == 8)
		panic("cmov: cannot use 8-bit registers");
	push_prefix(a, dst, src);
	push(a, 0x0f);
	push(a, 0x40 + c);
	push_mod_sib_offset(a, code(dst), src);
}

void xchg(Assembler &a, R dst, PTR src) { inst(a, dst, src, 0x86); }
void xchg(Assembler &a, R dst, R src) { inst(a, src, dst, 0x86); }

void lea(Assembler &a, R dst, PTR src)
{
	if (size(dst) == 8)
		panic("lea: cannot use 8 bit dst");
	inst(a, dst, src, 0x8c);
}

void inc(Assembler &a, R dst) { inst(a, dst, 0b000, 0xfe); }
void dec(Assembler &a, R dst) { inst(a, dst, 0b001, 0xfe); }

static void arith(Assembler &a, R dst, R src, u8 op) { inst(a, src, dst, (op << 3)); }

static void arith(Assembler &a, R dst, u32 src, u8 op)
{
	push_prefix(a, dst);
	if (dst.code == A) {
		push(a, 0x05);
	} else {
		push(a, 0x80 + (size(dst) > 8));
		push(a, modrm(ModDirect, op, code(dst)));
	}
	if (size(dst) == 64)
		push(a, src, 4);
	else
		push(a, src, size(dst)/8);
}

void add(Assembler &a, R dst, R src) { arith(a, dst, src, 0b000); }
void add(Assembler &a, R dst, u32 src) { arith(a, dst, src, 0b000); }
void or_(Assembler &a, R dst, R src) { arith(a, dst, src, 0b001); }
void or_(Assembler &a, R dst, u32 src) { arith(a, dst, src, 0b001); }
void and_(Assembler &a, R dst, R src) { arith(a, dst, src, 0b100); }
void and_(Assembler &a, R dst, u32 src) { arith(a, dst, src, 0b100); }
void sub(Assembler &a, R dst, R src) { arith(a, dst, src, 0b101); }
void sub(Assembler &a, R dst, u32 src) { arith(a, dst, src, 0b101); }
void xor_(Assembler &a, R dst, R src) { arith(a, dst, src, 0b110); }
void xor_(Assembler &a, R dst, u32 src) { arith(a, dst, src, 0b110); }
void cmp(Assembler &a, R dst, R src) { arith(a, dst, src, 0b111); }
void cmp(Assembler &a, R dst, u32 src) { arith(a, dst, src, 0b111); }

void jcc(Assembler &a, Cond c, const char *l)
{
	push(a, 0x0f);
	push(a, 0x80 + c);
	push_label_offset(a, l);
}

static void jump(Assembler &a, const char *dst, u8 op)
{
	push(a, op);
	push_label_offset(a, dst);
}

static void jump(Assembler &a, PTR dst, u8 op)
{
	if (size(dst) == 32)
		push(a, 0x67);
	u8 rex = isnew(dst.index)*REXX | isnew(dst.base)*REXB;
	if (rex)
		push(a, rex);
	push(a, 0xff);
	push_mod_sib_offset(a, op, dst);
}

static void jump(Assembler &a, R dst, u8 op)
{
	if (size(dst) != 64)
		panic("jump: not a 64 bit register");
	if (isnew(dst))
		push(a, REXB);
	push(a, 0xff);
	push(a, modrm(ModDirect, op, code(dst)));
}

void jmp(Assembler &a, const char *dst) { jump(a, dst, 0xe9); }
void jmp(Assembler &a, PTR dst) { jump(a, dst, 0b100); }
void jmp(Assembler &a, R dst) { jump(a, dst, 0b100); }
void call(Assembler &a, const char *dst) { jump(a, dst, 0xe8); }
void call(Assembler &a, PTR dst) { jump(a, dst, 0b010); }
void call(Assembler &a, R dst) { jump(a, dst, 0b010); }

void push(Assembler &a, R dst)
{
	if (size(dst) != 64)
		panic("push: not a 64 bit register");
	if (isnew(dst))
		push(a, REXB);
	push(a, 0x50 + code(dst));
}

void pop(Assembler &a, R dst)
{
	if (size(dst) != 64)
		panic("pop: not a 64 bit register");
	if (isnew(dst))
		push(a, REXB);
	push(a, 0x58 + code(dst));
}

void ret(Assembler &a) { push(a, 0xc3); }
