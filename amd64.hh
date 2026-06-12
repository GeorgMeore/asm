namespace amd64 {

struct Reg {
	u8 code;
	u8 size;
};

extern const Reg rax, eax,  ax,   al;
extern const Reg rcx, ecx,  cx,   cl;
extern const Reg rdx, edx,  dx,   dl;
extern const Reg rbx, ebx,  bx,   bl;
extern const Reg rsp, esp,  sp,   spl;
extern const Reg rbp, ebp,  bp,   bpl;
extern const Reg rsi, esi,  si,   sil;
extern const Reg rdi, edi,  di,   dil;
extern const Reg r8,  r8d,  r8w,  r8b;
extern const Reg r9,  r9d,  r9w,  r9b;
extern const Reg r10, r10d, r10w, r10b;
extern const Reg r11, r11d, r11w, r11b;
extern const Reg r12, r12d, r12w, r12b;
extern const Reg r13, r13d, r13w, r13b;
extern const Reg r14, r14d, r14w, r14b;
extern const Reg r15, r15d, r15w, r15b;

enum Error {
	ErrScale = AsmErrCount,
	ErrReg,
	ErrSize,
};

struct I {
	Reg index;
	u8  scale;
};

I operator*(Reg r, u8 scale);

struct Ptr {
	s32 offset;
	Reg base;
	Reg index;
	u8  scale;
};

Ptr ptr(Reg base, I i, s32 offset = 0);
Ptr ptr(Reg base, s32 offset = 0);
Ptr ptr(I i, s32 offset = 0);
Ptr ptr(s32 offset);

enum Cond {
	O  = 0x0,                       // overflow (OF=1)
	NO = 0x1,                       // not overflow (OF=0)
	B  = 0x2, C   = 0x2, NAE = 0x2, // below/carry/not above or equal (CF=1)
	AE = 0x3, NB  = 0x3, NC = 0x3,  // above or equal/not below/not carry (CF=0)
	Z  = 0x4, E   = 0x4,            // zero/equal (ZF=1)
	NE = 0x5, NZ  = 0x5,            // not equal/not zero (ZF=0)
	BE = 0x6, NA  = 0x6,            // below or equal/not above (CF=1 or ZF=1)
	A  = 0x7, NBE = 0x7,            // above/not below or equal (CF=0 and ZF=0)
	S  = 0x8,                       // sign (SF=1)
	NS = 0x9,                       // not sign (SF=0)
	P  = 0xa, PE  = 0xa,            // parity/parity even (PF=1)
	NP = 0xb, PO  = 0xb,            // not parity/parity odd (PF=0)
	L  = 0xc, NGE = 0xc,            // less/not greater or equal (SF!=OF)
	GE = 0xd, NL  = 0xd,            // greater or equal/not less (SF=OF)
	LE = 0xe, NG  = 0xe,            // less or equal/not greater (ZF=1 or SF!=OF)
	G  = 0xf, NLE = 0xf,            // greater/not less or equal (ZF=0 and SF=OF)
};

void mov(Assembler &a, Ptr dst, Reg src);
void mov(Assembler &a, Reg dst, Ptr src);
void mov(Assembler &a, Reg dst, Reg src);
void mov(Assembler &a, Reg dst, u64 src);
void mov(Assembler &a, Reg dst, void *src);
void mov(Assembler &a, void *dst, Reg src);
void cmov(Assembler &a, Cond c, Reg dst, Ptr src);
void cmov(Assembler &a, Cond c, Reg dst, Reg src);
void xchg(Assembler &a, Reg dst, Ptr src);
void xchg(Assembler &a, Reg dst, Reg src);
void lea(Assembler &a, Reg dst, Ptr src);
void inc(Assembler &a, Reg dst);
void dec(Assembler &a, Reg dst);
void add(Assembler &a, Reg dst, Reg src);
void add(Assembler &a, Reg dst, u32 src);
void or_(Assembler &a, Reg dst, Reg src);
void or_(Assembler &a, Reg dst, u32 src);
void and_(Assembler &a, Reg dst, Reg src);
void and_(Assembler &a, Reg dst, u32 src);
void sub(Assembler &a, Reg dst, Reg src);
void sub(Assembler &a, Reg dst, u32 src);
void xor_(Assembler &a, Reg dst, Reg src);
void xor_(Assembler &a, Reg dst, u32 src);
void cmp(Assembler &a, Reg dst, Reg src);
void cmp(Assembler &a, Reg dst, u32 src);
void mul(Assembler &a, Reg src);
void div(Assembler &a, Reg src);
void jcc(Assembler &a, Cond c, const char *l);
void jmp(Assembler &a, const char *dst);
void jmp(Assembler &a, Ptr dst);
void jmp(Assembler &a, Reg dst);
void call(Assembler &a, const char *dst);
void call(Assembler &a, Ptr dst);
void call(Assembler &a, Reg dst);
void push(Assembler &a, Reg dst);
void pop(Assembler &a, Reg dst);
void ret(Assembler &a);
void ud2(Assembler &a);
void int3(Assembler &a);
void syscall(Assembler &a);
void nop(Assembler &a);
void mfence(Assembler &a);
void rdtsc(Assembler &a);

}
