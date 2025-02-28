struct R {
	u8 code;
	u8 size;
};

extern const R rax, eax,  ax,   al;
extern const R rcx, ecx,  cx,   cl;
extern const R rdx, edx,  dx,   dl;
extern const R rbx, ebx,  bx,   bl;
extern const R rsp, esp,  sp,   spl;
extern const R rbp, ebp,  bp,   bpl;
extern const R rsi, esi,  si,   sil;
extern const R rdi, edi,  di,   dil;
extern const R r8,  r8d,  r8w,  r8b;
extern const R r9,  r9d,  r9w,  r9b;
extern const R r10, r10d, r10w, r10b;
extern const R r11, r11d, r11w, r11b;
extern const R r12, r12d, r12w, r12b;
extern const R r13, r13d, r13w, r13b;
extern const R r14, r14d, r14w, r14b;
extern const R r15, r15d, r15w, r15b;

enum Amd64Error {
	Amd64ErrScale,
	Amd64ErrReg,
	Amd64ErrSize,
};

struct I {
	R  index;
	u8 scale;
};

I operator*(R r, u8 scale);

struct PTR {
	s32 offset;
	R   base;
	R   index;
	u8  scale;
};

PTR ptr(R base, I i, s32 offset = 0);
PTR ptr(R base, s32 offset = 0);
PTR ptr(I i, s32 offset = 0);
PTR ptr(s32 offset);

enum Cond {
	CondO = 0x0,                              // overflow (OF=1)
	CondNO = 0x1,                             // not overflow (OF=0)
	CondB = 0x2, CondC = 0x2, CondNAE = 0x2,  // below/carry/not above or equal (CF=1)
	CondAE = 0x3, CondNB = 0x3, CondNC = 0x3, // above or equal/not below/not carry (CF=0)
	CondZ = 0x4, CondE = 0x4,                 // zero/equal (ZF=1)
	CondNE = 0x5, CondNZ = 0x5,               // not equal/not zero (ZF=0)
	CondBE = 0x6, CondNA = 0x6,               // below or equal/not above (CF=1 or ZF=1)
	CondA = 0x7, CondNBE = 0x7,               // above/not below or equal (CF=0 and ZF=0)
	CondS = 0x8,                              // sign (SF=1)
	CondNS = 0x9,                             // not sign (SF=0)
	CondP = 0xa, CondPE = 0xa,                // parity/parity even (PF=1)
	CondNP = 0xb, CondPO = 0xb,               // not parity/parity odd (PF=0)
	CondL = 0xc, CondNGE = 0xc,               // less/not greater or equal (SF!=OF)
	CondGE = 0xd, CondNL = 0xd,               // greater or equal/not less (SF=OF)
	CondLE = 0xe, CondNG = 0xe,               // less or equal/not greater (ZF=1 or SF!=OF)
	CondG = 0xf, CondNLE = 0xf,               // greater/not less or equal (ZF=0 and SF=OF)
};

void mov(Assembler &a, PTR dst, R src);
void mov(Assembler &a, R dst, PTR src);
void mov(Assembler &a, R dst, R src);
void mov(Assembler &a, R dst, u64 src);
void mov(Assembler &a, R dst, void *src);
void mov(Assembler &a, void *dst, R src);
void cmov(Assembler &a, Cond c, R dst, PTR src);
void cmov(Assembler &a, Cond c, R dst, R src);
void xchg(Assembler &a, R dst, PTR src);
void xchg(Assembler &a, R dst, R src);
void lea(Assembler &a, R dst, PTR src);
void inc(Assembler &a, R dst);
void dec(Assembler &a, R dst);
void add(Assembler &a, R dst, R src);
void add(Assembler &a, R dst, u32 src);
void or_(Assembler &a, R dst, R src);
void or_(Assembler &a, R dst, u32 src);
void and_(Assembler &a, R dst, R src);
void and_(Assembler &a, R dst, u32 src);
void sub(Assembler &a, R dst, R src);
void sub(Assembler &a, R dst, u32 src);
void xor_(Assembler &a, R dst, R src);
void xor_(Assembler &a, R dst, u32 src);
void cmp(Assembler &a, R dst, R src);
void cmp(Assembler &a, R dst, u32 src);
void mul(Assembler &a, R src);
void div(Assembler &a, R src);
void jcc(Assembler &a, Cond c, const char *l);
void jmp(Assembler &a, const char *dst);
void jmp(Assembler &a, PTR dst);
void jmp(Assembler &a, R dst);
void call(Assembler &a, const char *dst);
void call(Assembler &a, PTR dst);
void call(Assembler &a, R dst);
void push(Assembler &a, R dst);
void pop(Assembler &a, R dst);
void ret(Assembler &a);
void ud2(Assembler &a);
void int3(Assembler &a);
void syscall(Assembler &a);
void nop(Assembler &a);
void mfence(Assembler &a);
