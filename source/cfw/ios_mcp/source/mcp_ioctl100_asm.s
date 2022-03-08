.extern _MCP_ioctl100_patch
.global MCP_ioctl100_patch
MCP_ioctl100_patch:
    .thumb
    ldr r0, [r7,#0xC]
    bx pc
    nop
    .arm
    ldr r12, =_MCP_ioctl100_patch
    bx r12