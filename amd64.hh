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

enum Cond {
	AE = 0x3,          // above or equal (CF=0)
	Z = 0x4, EQ = 0x4, // equal (ZF=1)
	AB = 0x7,          // above (CF=0 and ZF=0)
};

// TODO: cmov, xchg
void mov(Assembler &a, PTR dst, R src);
void mov(Assembler &a, R dst, PTR src);
void mov(Assembler &a, R dst, R src);
void mov(Assembler &a, R dst, u64 src);
void mov(Assembler &a, R dst, void *src);
void mov(Assembler &a, void *dst, R src);
void lea(Assembler &a, R dst, PTR src);
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
