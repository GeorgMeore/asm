#include <stdio.h>

#include "types.hh"
#include "arena.hh"
#include "asm.hh"
#include "amd64.hh"

void dump(Assembler &a)
{
	if (a.err) {
		printf("error: assembly error: %d\n", a.err);
		return;
	}
	for (Symbol *s = a.syms; s; s = s->next) {
		if (s->refs) {
			printf("error: found unresolved references\n");
			return;
		}
	}
	for (u32 i = 0; i < a.ip; i++)
		putchar(a.code[i]);
}

void fib(Assembler &a)
{
label(a, "fib");
	mov(a, rax, 0UL);
	mov(a, rcx, 1);
label(a, "loop");
	cmp(a, rdi, 0);
	jcc(a, CondE, "return");
	mov(a, rdx, rcx);
	add(a, rcx, rax);
	mov(a, rax, rdx);
	dec(a, rdi);
	jmp(a, "loop");
label(a, "return");
	ret(a);
}

void start(Assembler &a)
{
	mov(a, rdi, 10);
	call(a, "fib");
	mov(a, rdi, rax);
	mov(a, rax, 60);
	syscall(a);
}

// You can use bin2elf.sh on the output of this program to get an executable
// that exits with 10nth fibonacci number as it's exit code.
int main()
{
	Assembler a{};
	start(a);
	fib(a);
	dump(a);
	clear(a);
}
