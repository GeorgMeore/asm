namespace arm64 {

struct Reg {
	u8 code;
	bool sf;
	bool sp;
};

extern const Reg x0,  w0;
extern const Reg x1,  w1;
extern const Reg x2,  w2;
extern const Reg x3,  w3;
extern const Reg x4,  w4;
extern const Reg x5,  w5;
extern const Reg x6,  w6;
extern const Reg x7,  w7;
extern const Reg x8,  w8;
extern const Reg x9,  w9;
extern const Reg x10, w10;
extern const Reg x11, w11;
extern const Reg x12, w12;
extern const Reg x13, w13;
extern const Reg x14, w14;
extern const Reg x15, w15;
extern const Reg ip0, x16, w16;
extern const Reg ip1, x17, w17;
extern const Reg x18, w18;
extern const Reg x19, w19;
extern const Reg x20, w20;
extern const Reg x21, w21;
extern const Reg x22, w22;
extern const Reg x23, w23;
extern const Reg x24, w24;
extern const Reg x25, w25;
extern const Reg x26, w26;
extern const Reg x27, w27;
extern const Reg x28, w28;
extern const Reg fp,  x29, w29;
extern const Reg lr,  x30, w30;
extern const Reg xzr, wzr;
extern const Reg sp,  wsp;

enum Error {
	ErrReg = AsmErrCount,
	ErrSize,
	ErrLogical,
};

enum Cond {
	EQ = 0b0000,
	NE,
	CS,
	CC,
	MI,
	PL,
	VS,
	VC,
	HI,
	LS,
	GE,
	LT,
	GT,
	LE,
	AL,
	NV,
};

enum Ex {
	UXTB = 0b000,
	UXTH,
	UXTW,
	UXTX,
	SXTB,
	SXTH,
	SXTW,
	SXTX,
};

enum Sh {
	LSL = 0b00,
	LSR,
	ASR,
};

void udf(Assembler &a, u16 imm);
void svc(Assembler &a, u16 imm);
void adc(Assembler &a, Reg d, Reg n, Reg m);
void add(Assembler &a, Reg d, Reg n, Reg m);
void add(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3 = 0);
void add(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6 = 0);
void add(Assembler &a, Reg d, Reg n, u16 imm12, Sh s = LSL, u8 simm = 0);
void adds(Assembler &a, Reg d, Reg n, Reg m);
void adds(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3 = 0);
void adds(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6 = 0);
void adds(Assembler &a, Reg d, Reg n, u16 imm12, Sh s = LSL, u8 simm = 0);
void sub(Assembler &a, Reg d, Reg n, Reg m);
void sub(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3 = 0);
void sub(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6 = 0);
void sub(Assembler &a, Reg d, Reg n, u16 imm12, Sh s = LSL, u8 simm = 0);
void subs(Assembler &a, Reg d, Reg n, Reg m);
void subs(Assembler &a, Reg d, Reg n, Reg m, Ex e, u8 imm3 = 0);
void subs(Assembler &a, Reg d, Reg n, Reg m, Sh s, u8 imm6 = 0);
void subs(Assembler &a, Reg d, Reg n, u16 imm12, Sh s = LSL, u8 simm = 0);
void cmp(Assembler &a, Reg n, Reg m);
void cmp(Assembler &a, Reg n, Reg m, Ex e, u8 imm3 = 0);
void cmp(Assembler &a, Reg n, Reg m, Sh s, u8 imm6 = 0);
void cmp(Assembler &a, Reg n, u16 imm12, Sh s = LSL, u8 simm = 0);
void sdiv(Assembler &a, Reg d, Reg n, Reg m);
void udiv(Assembler &a, Reg d, Reg n, Reg m);
void b(Assembler &a, const char *label);
void b(Assembler &a, Cond c, const char *label);
void bl(Assembler &a, const char *label);
void br(Assembler &a, Reg n);
void blr(Assembler &a, Reg n);
void orr(Assembler &a, Reg d, Reg n, Reg m, Sh s = LSL, u8 imm6 = 0);
void orr(Assembler &a, Reg d, Reg n, u64 imm);
void mov(Assembler &a, Reg d, Reg n);
void ret(Assembler &a, Reg n = lr);

}
