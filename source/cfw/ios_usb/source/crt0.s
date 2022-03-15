.section ".init"
.arm
.align 4

.global _start

.extern restoreHandle
.type restoreHandle, %function

_start:
    b restoreHandle
    .global IOS_DCFlushAllCache
IOS_DCFlushAllCache:
    MOV R15, R0
clean_loop:
    MRC p15, 0, r15, c7, c10, 3
    BNE clean_loop
    MCR p15, 0, R0, c7, c10, 4