#include <stdio.h>
#include <sys/mman.h>

#include "types.hh"
#include "arena.hh"
#include "asm.hh"
#include "amd64.hh"

void *link(Assembler &a)
{
	if (a.err) {
		printf("error: assembly error: %d\n", a.err);
		return 0;
	}
	for (Symbol *s = a.syms; s; s = s->next) {
		if (s->refs) {
			printf("error: found unresolved references\n");
			return 0;
		}
	}
	mprotect(a.code, a.ip, PROT_READ|PROT_EXEC);
	return a.code;
}

void sum(Assembler &a)
{
label(a, "sum");
	cmp(a, rdi, 0);
	jcc(a, CondE, "return0");
	push(a, rdi);
	dec(a, rdi);
	call(a, "sum");
	pop(a, rdi);
	add(a, rax, rdi);
	ret(a);
label(a, "return0");
	xor_(a, rax, rax);
	ret(a);
}

void hello(Assembler &a)
{
	mov(a, rax, 1);
	mov(a, rdi, (u64)0);
	mov(a, rsi, (u64)"Hello, world!\n");
	mov(a, rdx, 14);
	syscall(a);
	ret(a);
}

// We can do dynamic code generation!
int main()
{
	Assembler a{};
	hello(a);
	void (*hellop)() = (void(*)())link(a);
	hellop();
	clear(a);
	sum(a);
	u64 (*sump)(u64) = (u64(*)(u64))link(a);
	for (u64 i = 0; i <= 10; i++)
		printf("sum(%lu) = %lu\n", i, sump(i));
	clear(a);
	return 0;
}
