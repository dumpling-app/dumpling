.arm

.extern FSA_ioctl0x28_hook

.global _FSA_ioctl0x28_hook
_FSA_ioctl0x28_hook:
    mov r0, r10
    bl FSA_ioctl0x28_hook
    mov r5, r0
    ldr pc, =0x10701128