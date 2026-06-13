#include <assert.h>

#include "types.hh"
#include "arena.hh"
#include "asm.hh"
#include "arm64.hh"

namespace arm64 {

const Reg x0  = {0,  true, false}, w0  = {0,  false, false};
const Reg x1  = {1,  true, false}, w1  = {1,  false, false};
const Reg x2  = {2,  true, false}, w2  = {2,  false, false};
const Reg x3  = {3,  true, false}, w3  = {3,  false, false};
const Reg x4  = {4,  true, false}, w4  = {4,  false, false};
const Reg x5  = {5,  true, false}, w5  = {5,  false, false};
const Reg x6  = {6,  true, false}, w6  = {6,  false, false};
const Reg x7  = {7,  true, false}, w7  = {7,  false, false};
const Reg x8  = {8,  true, false}, w8  = {8,  false, false};
const Reg x9  = {9,  true, false}, w9  = {9,  false, false};
const Reg x10 = {10, true, false}, w10 = {10, false, false};
const Reg x11 = {11, true, false}, w11 = {11, false, false};
const Reg x12 = {12, true, false}, w12 = {12, false, false};
const Reg x13 = {13, true, false}, w13 = {13, false, false};
const Reg x14 = {14, true, false}, w14 = {14, false, false};
const Reg x15 = {15, true, false}, w15 = {15, false, false};
const Reg ip0 = {16, true, false}, x16 = {16, true,  false}, w16 = {16, false, false};
const Reg ip1 = {17, true, false}, x17 = {17, true,  false}, w17 = {17, false, false};
const Reg x18 = {18, true, false}, w18 = {18, false, false};
const Reg x19 = {19, true, false}, w19 = {19, false, false};
const Reg x20 = {20, true, false}, w20 = {20, false, false};
const Reg x21 = {21, true, false}, w21 = {21, false, false};
const Reg x22 = {22, true, false}, w22 = {22, false, false};
const Reg x23 = {23, true, false}, w23 = {23, false, false};
const Reg x24 = {24, true, false}, w24 = {24, false, false};
const Reg x25 = {25, true, false}, w25 = {25, false, false};
const Reg x26 = {26, true, false}, w26 = {26, false, false};
const Reg x27 = {27, true, false}, w27 = {27, false, false};
const Reg x28 = {28, true, false}, w28 = {28, false, false};
const Reg fp  = {29, true, false}, x29 = {29, true,  false}, w29 = {29, false, false};
const Reg lr  = {30, true, false}, x30 = {30, true,  false}, w30 = {30, false, false};
const Reg xzr = {31, true, false}, wzr = {31, false, false};
const Reg sp  = {31, true, true},  wsp = {31, false, true};

static bool iszr(Reg r) { return r.code == 31 && !r.sp; }
static bool issp(Reg r) { return r.sp; }

struct Inst {
	u32 v;
	u8  n;
};

static void push_bits(Inst &i, u32 v, u8 n)
{
	assert(n > 0 && 32 - i.n >= n);
	assert(v >> n == 0);
	i.v |= v << i.n;
	i.n += n;
}

static void push_inst(Assembler &a, Inst &i)
{
	assert(i.n == 32);
	push_bytes(a, i.v, 4);
}

void udf(Assembler &a, u16 imm)
{
	push_bytes(a, imm, 4);
}

void svc(Assembler &a, u16 imm)
{
	Inst i = {};
	push_bits(i, 0b00001, 5);
	push_bits(i, imm, 16);
	push_bits(i, 0b11010100000, 11);
	push_inst(a, i);
}

static void inst3r(Assembler &a, u8 c1, u16 c2, Reg d, Reg n, Reg m)
{
	if (issp(d) || issp(n) || issp(m)) {
		a.err = ErrReg;
		return udf(a, 0);
	}
	if (d.sf != n.sf || n.sf != m.sf) {
		a.err = ErrSize;
		return udf(a, 0);
	}
	Inst i = {};
	push_bits(i, d.code, 5);
	push_bits(i, n.code, 5);
	push_bits(i, c1, 6);
	push_bits(i, m.code, 5);
	push_bits(i, c2, 10);
	push_bits(i, d.sf, 1);
	push_inst(a, i);
}

void adc(Assembler &a, Reg d, Reg n, Reg m)  { return inst3r(a, 0, 0b0011010000, d, n, m); }
void sdiv(Assembler &a, Reg d, Reg n, Reg m) { return inst3r(a, 3, 0b0011010110, d, n, m); }
void udiv(Assembler &a, Reg d, Reg n, Reg m) { return inst3r(a, 2, 0b0011010110, d, n, m); }

static void inste(Assembler &a, bool spd, u16 c, Reg d, Reg n, Reg m, Ex e, u8 imm3)
{
	if ((spd && iszr(d)) || (!spd && issp(d)) || iszr(n) || issp(m)) {
		a.err = ErrReg;
		return udf(a, 0);
	}
	if (d.sf != n.sf || n.sf != m.sf || imm3 > 4) {
		a.err = ErrSize;
		return udf(a, 0);
	}
	Inst i = {};
	push_bits(i, d.code, 5);
	push_bits(i, n.code, 5);
	push_bits(i, imm3, 3);
	push_bits(i, e, 3);
	push_bits(i, m.code, 5);
	push_bits(i, c, 10);
	push_bits(i, d.sf, 1);
	push_inst(a, i);
}

static void insts(Assembler &a, u8 c, Reg d, Reg n, Reg m, Sh s, u8 imm6)
{
	if (issp(d) || issp(n) || issp(m)) {
		a.err = ErrReg;
		return udf(a, 0);
	}
	if (d.sf != n.sf || n.sf != m.sf || imm6 > ((32<<d.sf) - 1)) {
		a.err = ErrSize;
		return udf(a, 0);
	}
	Inst i = {};
	push_bits(i, d.code, 5);
	push_bits(i, n.code, 5);
	push_bits(i, imm6, 6);
	push_bits(i, m.code, 5);
	push_bits(i, 0, 1);
	push_bits(i, s, 2);
	push_bits(i, c, 7);
	push_bits(i, d.sf, 1);
	push_inst(a, i);
}

static void insti(Assembler &a, bool spd, u8 c, Reg d, Reg n, u16 imm12, Sh s, u8 simm)
{
	if ((spd && iszr(d)) || (!spd && issp(d)) || iszr(n)) {
		a.err = ErrReg;
		return udf(a, 0);
	}
	if (d.sf != n.sf || s != LSL || (simm != 0 && simm != 12) || imm12 > 4095) {
		a.err = ErrSize;
		return udf(a, 0);
	}
	Inst i = {};
	push_bits(i, d.code, 5);
	push_bits(i, n.code, 5);
	push_bits(i, imm12, 12);
	push_bits(i, simm == 12, 1);
	push_bits(i, c, 8);
	push_bits(i, d.sf, 1);
	push_inst(a, i);
}

static void inst3r2(Assembler &a, bool spd, u8 c1, u16 c2, Reg d, Reg n, Reg m)
{
	if (issp(d) || issp(n))
		return inste(a, spd, c2, d, n, m, d.sf ? UXTX : UXTW, 0);
	return insts(a, c1, d, n, m, LSL, 0);
}

void add(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3) { inste(a, true, 0b0001011001, d, n, m, e, imm3); }
void add(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6) { insts(a, 0b0001011, d, n, m, s, imm6); }
void add(Assembler &a, Reg d, Reg n, u16 imm12, Sh s, u8 simm) { insti(a, true, 0b00100010, d, n, imm12, s, simm); }
void add(Assembler &a, Reg d, Reg n, Reg m) { inst3r2(a, true, 0b0001011, 0b0001011001, d, n, m); }

void sub(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3) { inste(a, true, 0b1001011001, d, n, m, e, imm3); }
void sub(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6) { insts(a, 0b1001011, d, n, m, s, imm6); }
void sub(Assembler &a, Reg d, Reg n, u16 imm12, Sh s, u8 simm) { insti(a, true, 0b10100010, d, n, imm12, s, simm); }
void sub(Assembler &a, Reg d, Reg n, Reg m) { inst3r2(a, true, 0b1001011, 0b1001011001, d, n, m); }

void adds(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3) { inste(a, false, 0b0101011001, d, n, m, e, imm3); }
void adds(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6) { insts(a, 0b0101011, d, n, m, s, imm6); }
void adds(Assembler &a, Reg d, Reg n, u16 imm12, Sh s, u8 simm) { insti(a, false, 0b01100010, d, n, imm12, s, simm); }
void adds(Assembler &a, Reg d, Reg n, Reg m) { inst3r2(a, false, 0b0101011, 0b0101011001, d, n, m); }

void subs(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3) { inste(a, false, 0b1101011001, d, n, m, e, imm3); }
void subs(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6) { insts(a, 0b1101011, d, n, m, s, imm6); }
void subs(Assembler &a, Reg d, Reg n, u16 imm12, Sh s, u8 simm) { insti(a, false, 0b11100010, d, n, imm12, s, simm); }
void subs(Assembler &a, Reg d, Reg n, Reg m) { inst3r2(a, false, 0b1101011, 0b1101011001, d, n, m); }

void cmp(Assembler &a, Reg n, Reg m) { subs(a, xzr, n, m); }
void cmp(Assembler &a, Reg n, Reg m, Ex e, u8 imm3) { subs(a, xzr, n, m, e, imm3); }
void cmp(Assembler &a, Reg n, Reg m, Sh s, u8 imm6) { subs(a, xzr, n, m, s, imm6); }
void cmp(Assembler &a, Reg n, u16 imm12, Sh s, u8 simm) { subs(a, xzr, n, imm12, s, simm); }

void b(Assembler &a, const char *label)
{
	Inst i = {};
	push_bits(i, 0, 26); // label placeholder
	push_bits(i, 0b000101, 6);
	push_inst(a, i);
	label_ref(a, label, a.ip - 4, a.ip - 4, 4, 26, 0);
}

void b(Assembler &a, Cond c, const char *label)
{
	Inst i = {};
	push_bits(i, c, 4);
	push_bits(i, 0, 1);
	push_bits(i, 0, 19); // label placeholder
	push_bits(i, 0b01010100, 8);
	push_inst(a, i);
	label_ref(a, label, a.ip - 4, a.ip - 4, 4, 19, 5);
}

void bl(Assembler &a, const char *label)
{
	Inst i = {};
	push_bits(i, 0, 26); // label placeholder
	push_bits(i, 0b100101, 6);
	push_inst(a, i);
	label_ref(a, label, a.ip - 4, a.ip - 4, 4, 26, 0);
}

static void branchreg(Assembler &a, u32 c, Reg n)
{
	if (issp(n)) {
		a.err = ErrReg;
		return udf(a, 0);
	}
	Inst i = {};
	push_bits(i, 0, 5);
	push_bits(i, n.code, 5);
	push_bits(i, c, 22);
	push_inst(a, i);
}

void br(Assembler &a, Reg n)  { branchreg(a, 0b1101011000011111000000, n); }
void blr(Assembler &a, Reg n) { branchreg(a, 0b1101011000111111000000, n); }

}
