#include <stdio.h>
#include <stdlib.h>

#include "types.hh"
#include "asm.hh"
#include "amd64.hh"

void expect(const Assembler &a, const u8 b[], u64 s, const char *file, int line)
{
	for (u8 i = 0; i < s; i++) {
		if (a.ip < i || a.b[a.ip-1-i] != b[s-1-i]) {
			printf("Test failed: %s:%d\n", file, line);
			exit(1);
		}
	}
}

#define expect(a, ...) expect(a, (u8[])__VA_ARGS__, sizeof((u8[])__VA_ARGS__), __FILE__, __LINE__)

int main(void)
{
	Assembler a{};
	// instruction                      // expected byte sequence
	mov(a, r9, 123);                    expect(a, {0x49, 0xb9, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
	mov(a, r8d, 123);                   expect(a, {0x41, 0xb8, 0x7b, 0x00, 0x00, 0x00});
	mov(a, r8w, 123);                   expect(a, {0x66, 0x41, 0xb8, 0x7b, 0x00});
	mov(a, r8b, 123);                   expect(a, {0x41, 0xb0, 0x7b});
	mov(a, ptr(r12), rax);              expect(a, {0x49, 0x89, 0x04, 0x24});
	mov(a, ptr(r12), r8w);              expect(a, {0x66, 0x45, 0x89, 0x04, 0x24});
	mov(a, ptr(rsp, rbp*2, -1), rax);   expect(a, {0x48, 0x89, 0x44, 0x6c, 0xff});
	mov(a, rax, ptr(rsp, rbp*2, -1));   expect(a, {0x48, 0x8b, 0x44, 0x6c, 0xff});
	mov(a, ax, ptr(esp, ebp*2, -128));  expect(a, {0x67, 0x66, 0x8b, 0x44, 0x6c, 0x80});
	mov(a, al, ptr(esp, ebp*2, 127));   expect(a, {0x67, 0x8a, 0x44, 0x6c, 0x7f});
	mov(a, ptr(r8, r9*2, 128), r10b);   expect(a, {0x47, 0x88, 0x94, 0x48, 0x80, 0x00, 0x00, 0x00});
	mov(a, rbx, r8);                    expect(a, {0x4c, 0x89, 0xc3});
	mov(a, bx, ax);                     expect(a, {0x66, 0x89, 0xc3});
	mov(a, bl, al);                     expect(a, {0x88, 0xc3});
	mov(a, eax, ecx);                   expect(a, {0x89, 0xc8});
	mov(a, r8b, al);                    expect(a, {0x41, 0x88, 0xc0});
	push(a, rax);                       expect(a, {0x50});
	push(a, r8);                        expect(a, {0x41, 0x50});
	pop(a, r8);                         expect(a, {0x41, 0x58});
	pop(a, rax);                        expect(a, {0x58});
	lea(a, rax, ptr(rsp, rbp*2, -129)); expect(a, {0x48, 0x8d, 0x84, 0x6c, 0x7f, 0xff, 0xff, 0xff});
	lea(a, eax, ptr(rsp, rbp*2, 45));   expect(a, {0x8d, 0x44, 0x6c, 0x2d});
	lea(a, r12w, ptr(rsp, rbp*2, 255)); expect(a, {0x66, 0x44, 0x8d, 0xa4, 0x6c, 0xff, 0x00, 0x00, 0x00});
	jmp(a, rax);                        expect(a, {0xff, 0xe0});
	jmp(a, r12);                        expect(a, {0x41, 0xff, 0xe4});
label(a, "foo");
	jcc(a, AB, "foo");                  expect(a, {0x0f, 0x87, 0xfa, 0xff, 0xff, 0xff});
	jcc(a, EQ, "foo");                  expect(a, {0x0f, 0x84, 0xf4, 0xff, 0xff, 0xff});
	add(a, rax, rbx);                   expect(a, {0x48, 0x01, 0xd8});
	add(a, eax, ecx);                   expect(a, {0x01, 0xc8});
	add(a, al, r12b);                   expect(a, {0x44, 0x00, 0xe0});
	add(a, r13b, r8b);                  expect(a, {0x45, 0x00, 0xc5});
	or_(a, eax, ecx);                   expect(a, {0x09, 0xc8});
	or_(a, al, r12b);                   expect(a, {0x44, 0x08, 0xe0});
	or_(a, r13b, r8b);                  expect(a, {0x45, 0x08, 0xc5});
	and_(a, eax, ecx);                  expect(a, {0x21, 0xc8});
	and_(a, al, r12b);                  expect(a, {0x44, 0x20, 0xe0});
	and_(a, r13b, r8b);                 expect(a, {0x45, 0x20, 0xc5});
	add(a, rax, 588);                   expect(a, {0x48, 0x05, 0x4c, 0x02, 0x00, 0x00});
	add(a, rbx, 588);                   expect(a, {0x48, 0x81, 0xc3, 0x4c, 0x02, 0x00, 0x00});
	add(a, r12, 588);                   expect(a, {0x49, 0x81, 0xc4, 0x4c, 0x02, 0x00, 0x00});
	add(a, r12w, 588);                  expect(a, {0x66, 0x41, 0x81, 0xc4, 0x4c, 0x02});
	add(a, r12b, 588);                  expect(a, {0x41, 0x80, 0xc4, 0x4c});
	mov(a, rax, (void*)0xffff88000023); expect(a, {0x48, 0xa1, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, eax, (void*)0xffff88000023); expect(a, {0xa1, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, ax, (void*)0xffff88000023);  expect(a, {0x66, 0xa1, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, al, (void*)0xffff88000023);  expect(a, {0xa0, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, (void*)0xffff88000023, rax); expect(a, {0x48, 0xa3, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, (void*)0xffff88000023, eax); expect(a, {0xa3, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, (void*)0xffff88000023, ax);  expect(a, {0x66, 0xa3, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	mov(a, (void*)0xffff88000023, al);  expect(a, {0xa2, 0x23, 0x00, 0x00, 0x88, 0xff, 0xff, 0x00, 0x00});
	jmp(a, ptr(eax, ebx*4, 5));         expect(a, {0x67, 0xff, 0x64, 0x98, 0x05});
	jmp(a, ptr(rcx, r9*8, -5));         expect(a, {0x42, 0xff, 0x64, 0xc9, 0xfb});
	call(a, rax);                       expect(a, {0xff, 0xd0});
	call(a, r12);                       expect(a, {0x41, 0xff, 0xd4});
	call(a, ptr(eax, ebx*4, 5));        expect(a, {0x67, 0xff, 0x54, 0x98, 0x05});
	call(a, ptr(rcx, r9*8, -5));        expect(a, {0x42, 0xff, 0x54, 0xc9, 0xfb});
	printf("OK\n");
	clear(a);
}
