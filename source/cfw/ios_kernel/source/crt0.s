.section ".init"
.arm
.align 4

.extern mainKernel
.type mainKernel, %function

_start:
    b mainKernel
