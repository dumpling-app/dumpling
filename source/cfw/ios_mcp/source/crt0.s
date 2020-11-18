.section ".init"
.arm
.align 4

.global _start

.extern mainIPC
.type mainIPC, %function

_start:
	@ Run wupserver thread
	mov r0, #0
	bl mainIPC
	@ Jump back to original code
	ldr r1,[pc]
	bx r1
	.word (0x05027954)+1


# Required for switch cases code with thumb code generation enabled, gotten from https://subversion.assembla.com/svn/chdk-s1.sx280/lib/libc/thumb1_case.S
.text
.align 0
.force_thumb
.syntax unified
.global __gnu_thumb1_case_uqi
.thumb_func
.type __gnu_thumb1_case_uqi,function
__gnu_thumb1_case_uqi:
	push	{r1}
	mov	r1, lr
	lsrs	r1, r1, #1
	lsls	r1, r1, #1
	ldrb	r1, [r1, r0]
	lsls	r1, r1, #1
	add	lr, lr, r1
	pop	{r1}
	bx	lr
	.size __gnu_thumb1_case_uqi, . - __gnu_thumb1_case_uqi
	
.text
.align 0
.force_thumb
.syntax unified
.global __gnu_thumb1_case_uhi
.thumb_func
.type __gnu_thumb1_case_uhi,function
__gnu_thumb1_case_uhi:
	push	{r0, r1}
	mov	r1, lr
	lsrs	r1, r1, #1
	lsls	r0, r0, #1
	lsls	r1, r1, #1
	ldrh	r1, [r1, r0]
	lsls	r1, r1, #1
	add	lr, lr, r1
	pop	{r0, r1}
	bx	lr
	.size __gnu_thumb1_case_uhi, . - __gnu_thumb1_case_uhi