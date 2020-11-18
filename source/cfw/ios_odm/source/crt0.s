.section ".init"
.arm
.align 4

.globl _start

.extern odmReadKey
.type odmReadKey, %function

_start:
	mov r1, r10
	mov r2, r8
	b odmReadKey
