#include <assert.h>

#include "types.hh"
#include "arena.hh"
#include "asm.hh"
#include "amd64.hh"

namespace amd64 {

const Reg rax = {0,  64}, eax  = {0,  32}, ax   = {0,  16}, al   = {0,  8};
const Reg rcx = {1,  64}, ecx  = {1,  32}, cx   = {1,  16}, cl   = {1,  8};
const Reg rdx = {2,  64}, edx  = {2,  32}, dx   = {2,  16}, dl   = {2,  8};
const Reg rbx = {3,  64}, ebx  = {3,  32}, bx   = {3,  16}, bl   = {3,  8};
const Reg rsp = {4,  64}, esp  = {4,  32}, sp   = {4,  16}, spl  = {4,  8};
const Reg rbp = {5,  64}, ebp  = {5,  32}, bp   = {5,  16}, bpl  = {5,  8};
const Reg rsi = {6,  64}, esi  = {6,  32}, si   = {6,  16}, sil  = {6,  8};
const Reg rdi = {7,  64}, edi  = {7,  32}, di   = {7,  16}, dil  = {7,  8};
const Reg r8  = {8,  64}, r8d  = {8,  32}, r8w  = {8,  16}, r8b  = {8,  8};
const Reg r9  = {9,  64}, r9d  = {9,  32}, r9w  = {9,  16}, r9b  = {9,  8};
const Reg r10 = {10, 64}, r10d = {10, 32}, r10w = {10, 16}, r10b = {10, 8};
const Reg r11 = {11, 64}, r11d = {11, 32}, r11w = {11, 16}, r11b = {11, 8};
const Reg r12 = {12, 64}, r12d = {12, 32}, r12w = {12, 16}, r12b = {12, 8};
const Reg r13 = {13, 64}, r13d = {13, 32}, r13w = {13, 16}, r13b = {13, 8};
const Reg r14 = {14, 64}, r14d = {14, 32}, r14w = {14, 16}, r14b = {14, 8};
const Reg r15 = {15, 64}, r15d = {15, 32}, r15w = {15, 16}, r15b = {15, 8};

static u8   size(Reg r) { return r.size; }
static u8   code(Reg r) { return r.code & 0b111; }
static bool isnew(Reg r) { return r.code & 0b1000; }

I operator*(Reg r, u8 scale) { return {r, scale}; }

static u8 offsetsize(Ptr p)
{
	if (p.offset < -128 || p.offset > 127)
		return 4;
	if (p.offset || code(p.base) == 0b101)
		return 1;
	return 0;
}

static u8 size(Ptr p) { return p.base.code ? p.base.size : p.index.size; }

Ptr ptr(Reg base, I i, s32 offset) { return {offset, base, i.index, i.scale}; }
Ptr ptr(Reg base, s32 offset) { return ptr(base, {}, offset); }
Ptr ptr(I i, s32 offset) { return ptr({}, i, offset); }
Ptr ptr(s32 offset) { return ptr({}, {}, offset); }

static int ptr_err(Ptr p)
{
	if (size(p.index) & 0x1f || size(p.base) & 0x1f)
		return ErrSize; // less than 32 bits
	if (size(p.index)) {
		if (!p.scale || p.scale > 8 || p.scale&(p.scale - 1))
			return ErrScale;
		if (p.index.code == sp.code)
			return ErrReg;
		if (size(p.base) && size(p.base) != size(p.index))
			return ErrSize;
	}
	return 0;
}

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
		case 4: return ModDisp4;
		default: assert(0);
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
		case 8: return Scale8;
		default: assert(0);
	}
}

static u8 sib(Scale scale, u8 index, u8 base)
{
	return scale<<6 | index<<3 | base;
}

static void push_mod_sib_offset(Assembler &a, u8 reg, Ptr p)
{
	if (!size(p.base)) {
		push_byte(a, modrm(ModDisp0, reg, 0b100));
		if (size(p.index))
			push_byte(a, sib(scale(p.scale), code(p.index), 0b101));
		else
			push_byte(a, sib(Scale1, 0b100, 0b101));
		push_bytes(a, p.offset, 4);
		return;
	}
	u8 osz = offsetsize(p);
	if (!size(p.index) && code(p.base) != 0b100) {
		push_byte(a, modrm(mod(osz), reg, code(p.base)));
	} else {
		push_byte(a, modrm(mod(osz), reg, 0b100));
		if (size(p.index))
			push_byte(a, sib(scale(p.scale), code(p.index), code(p.base)));
		else
			push_byte(a, sib(Scale1, 0b100, code(p.base)));
	}
	push_bytes(a, p.offset, osz);
}

static void push_prefixes(Assembler &a, Reg r, Ptr p)
{
	if (size(p) == 32)
		push_byte(a, 0x67);
	if (size(r) == 16)
		push_byte(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	rex |= isnew(r)*REXR | isnew(p.index)*REXX | isnew(p.base)*REXB;
	if (rex)
		push_byte(a, rex);
}

static void inst(Assembler &a, Reg r, Ptr rm, u8 op)
{
	if (!a.err)
		a.err = ptr_err(rm);
	if (a.err)
		return ud2(a);
	push_prefixes(a, r, rm);
	push_byte(a, op + (size(r) > 8));
	push_mod_sib_offset(a, code(r), rm);
}

static void push_prefixes(Assembler &a, Reg r, Reg rm)
{
	if (size(r) == 16)
		push_byte(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	rex |= isnew(r)*REXR | isnew(rm)*REXB;
	if (rex)
		push_byte(a, rex);
}

static void inst(Assembler &a, Reg r, Reg rm, u8 op)
{
	if (size(r) != size(rm)) {
		a.err = ErrSize;
		return ud2(a);
	}
	push_prefixes(a, r, rm);
	push_byte(a, op + (size(r) > 8));
	push_byte(a, modrm(ModDirect, code(r), code(rm)));
}

static void push_prefixes(Assembler &a, Reg r)
{
	if (size(r) == 16)
		push_byte(a, 0x66);
	u8 rex = 0;
	if (size(r) == 64)
		rex |= REXW;
	if (isnew(r))
		rex |= REXB;
	if (rex)
		push_byte(a, rex);
}

static void inst(Assembler &a, Reg dst, u8 src, u8 op)
{
	push_prefixes(a, dst);
	push_byte(a, op + (size(dst) > 8));
	push_byte(a, modrm(ModDirect, src, code(dst)));
}

void mov(Assembler &a, Ptr dst, Reg src) { inst(a, src, dst, 0x88); }
void mov(Assembler &a, Reg dst, Ptr src) { inst(a, dst, src, 0x8a); }
void mov(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0x88); }

void mov(Assembler &a, Reg dst, u64 src)
{
	push_prefixes(a, dst);
	push_byte(a, size(dst) == 8 ? 0xb0 : 0xb8 + code(dst));
	push_bytes(a, src, size(dst)/8);
}

void mov(Assembler &a, Reg dst, void *src)
{
	if (dst.code != rax.code) {
		a.err = ErrReg;
		return ud2(a);
	}
	push_prefixes(a, dst);
	push_byte(a, 0xa0 + (size(dst) > 8));
	push_bytes(a, (u64)src, 8);
}

void mov(Assembler &a, void *dst, Reg src)
{
	if (src.code != rax.code) {
		a.err = ErrReg;
		return ud2(a);
	}
	push_prefixes(a, src);
	push_byte(a, 0xa2 + (size(src) > 8));
	push_bytes(a, (u64)dst, 8);
}

void cmov(Assembler &a, Cond c, Reg dst, Reg src)
{
	if (size(src) != size(dst) || size(src) == 8) {
		a.err = ErrSize;
		return ud2(a);
	}
	push_prefixes(a, dst, src);
	push_byte(a, 0x0f);
	push_byte(a, 0x40 + c);
	push_byte(a, modrm(ModDirect, code(dst), code(src)));
}

void cmov(Assembler &a, Cond c, Reg dst, Ptr src)
{
	if (!a.err)
		a.err = ptr_err(src);
	if (size(src) == 8)
		a.err = ErrSize;
	if (a.err)
		return ud2(a);
	push_prefixes(a, dst, src);
	push_byte(a, 0x0f);
	push_byte(a, 0x40 + c);
	push_mod_sib_offset(a, code(dst), src);
}

void xchg(Assembler &a, Reg dst, Ptr src) { inst(a, dst, src, 0x86); }
void xchg(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0x86); }

void lea(Assembler &a, Reg dst, Ptr src)
{
	if (size(dst) == 8) {
		a.err = ErrSize;
		return ud2(a);
	}
	inst(a, dst, src, 0x8c);
}

void inc(Assembler &a, Reg dst) { inst(a, dst, 0b000, 0xfe); }
void dec(Assembler &a, Reg dst) { inst(a, dst, 0b001, 0xfe); }

static void arith(Assembler &a, Reg dst, u32 src, u8 op)
{
	push_prefixes(a, dst);
	if (dst.code == rax.code) {
		push_byte(a, 0x05);
	} else {
		push_byte(a, 0x80 + (size(dst) > 8));
		push_byte(a, modrm(ModDirect, op, code(dst)));
	}
	if (size(dst) == 64)
		push_bytes(a, src, 4);
	else
		push_bytes(a, src, size(dst)/8);
}

void add(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0b000 << 3); }
void add(Assembler &a, Reg dst, u32 src) { arith(a, dst, src, 0b000); }
void or_(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0b001 << 3); }
void or_(Assembler &a, Reg dst, u32 src) { arith(a, dst, src, 0b001); }
void and_(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0b100 << 3); }
void and_(Assembler &a, Reg dst, u32 src) { arith(a, dst, src, 0b100); }
void sub(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0b101 << 3); }
void sub(Assembler &a, Reg dst, u32 src) { arith(a, dst, src, 0b101); }
void xor_(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0b110 << 3); }
void xor_(Assembler &a, Reg dst, u32 src) { arith(a, dst, src, 0b110); }
void cmp(Assembler &a, Reg dst, Reg src) { inst(a, src, dst, 0b111 << 3); }
void cmp(Assembler &a, Reg dst, u32 src) { arith(a, dst, src, 0b111); }

void mul(Assembler &a, Reg src) { inst(a, src, 0x4, 0xf6); }
void div(Assembler &a, Reg src) { inst(a, src, 0x6, 0xf6); }

void jcc(Assembler &a, Cond c, const char *l)
{
	push_byte(a, 0x0f);
	push_byte(a, 0x80 + c);
	push_label_offset(a, l);
}

static void jump(Assembler &a, const char *dst, u8 op)
{
	push_byte(a, op);
	push_label_offset(a, dst);
}

static void jump(Assembler &a, Ptr dst, u8 op)
{
	if (!a.err)
		a.err = ptr_err(dst);
	if (a.err)
		return ud2(a);
	if (size(dst) == 32)
		push_byte(a, 0x67);
	u8 rex = isnew(dst.index)*REXX | isnew(dst.base)*REXB;
	if (rex)
		push_byte(a, rex);
	push_byte(a, 0xff);
	push_mod_sib_offset(a, op, dst);
}

static void jump(Assembler &a, Reg dst, u8 op)
{
	if (size(dst) != 64) {
		a.err = ErrSize;
		return ud2(a);
	}
	if (isnew(dst))
		push_byte(a, REXB);
	push_byte(a, 0xff);
	push_byte(a, modrm(ModDirect, op, code(dst)));
}

void jmp(Assembler &a, const char *dst) { jump(a, dst, 0xe9); }
void jmp(Assembler &a, Ptr dst) { jump(a, dst, 0b100); }
void jmp(Assembler &a, Reg dst) { jump(a, dst, 0b100); }
void call(Assembler &a, const char *dst) { jump(a, dst, 0xe8); }
void call(Assembler &a, Ptr dst) { jump(a, dst, 0b010); }
void call(Assembler &a, Reg dst) { jump(a, dst, 0b010); }

void push(Assembler &a, Reg dst)
{
	if (size(dst) != 64) {
		a.err = ErrSize;
		return ud2(a);
	}
	if (isnew(dst))
		push_byte(a, REXB);
	push_byte(a, 0x50 + code(dst));
}

void pop(Assembler &a, Reg dst)
{
	if (size(dst) != 64) {
		a.err = ErrSize;
		return ud2(a);
	}
	if (isnew(dst))
		push_byte(a, REXB);
	push_byte(a, 0x58 + code(dst));
}

void ret(Assembler &a) { push_byte(a, 0xc3); }
void ud2(Assembler &a) { push_bytes(a, 0x0b0f, 2); }
void int3(Assembler &a) { push_byte(a, 0xcc); }
void syscall(Assembler &a) { push_bytes(a, 0x050f, 2); }
void nop(Assembler &a) { push_byte(a, 0x90); }
void mfence(Assembler &a) { push_bytes(a, 0xf0ae0f, 3); }
void rdtsc(Assembler &a) { push_bytes(a, 0x310f, 2); }

}
