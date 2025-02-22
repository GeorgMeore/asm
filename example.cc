#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "types.hh"
#include "asm.hh"
#include "amd64.hh"

void *map(Assembler &a)
{
	for (Symbol *s = a.syms; s; s = s->next) {
		if (s->refs) {
			printf("error: found unresolved references\n");
			return nullptr;
		}
	}
	void *p = mmap(0, a.ip, PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (p) {
		memcpy(p, a.b, a.ip);
		mprotect(p, a.ip, PROT_READ|PROT_EXEC);
	}
	return p;
}

void fib(Assembler &a)
{
	mov(a, rax, 0UL);
	mov(a, rcx, 1);
label(a, "loop");
	cmp(a, rdi, 0);
	jcc(a, EQ, "return");
	mov(a, rdx, rcx);
	add(a, rcx, rax);
	mov(a, rax, rdx);
	dec(a, rdi);
	jmp(a, "loop");
label(a, "return");
	ret(a);
}

void sum(Assembler &a)
{
label(a, "sum");
	cmp(a, rdi, 0);
	jcc(a, EQ, "return0");
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

int main(void)
{
	Assembler a{};
	fib(a);
	u64 (*fibp)(u64) = (u64(*)(u64))map(a);
	clear(a);
	sum(a);
	u64 (*sump)(u64) = (u64(*)(u64))map(a);
	clear(a);
	for (u64 i = 0; i <= 10; i++)
		printf("fib(%lu) = %lu\n", i, fibp(i));
	for (u64 i = 0; i <= 10; i++)
		printf("sum(%lu) = %lu\n", i, sump(i));
	return 0;
}
