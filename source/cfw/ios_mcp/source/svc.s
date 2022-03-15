.section ".text"
.arm
.align 4

.global svcCreateThread
.type svcCreateThread, %function
svcCreateThread:
    .word 0xE7F000F0
    bx lr

.global svcStartThread
.type svcStartThread, %function
svcStartThread:
    .word 0xE7F007F0
    bx lr

.global svcCreateMessageQueue
.type svcCreateMessageQueue, %function
svcCreateMessageQueue:
    .word 0xE7F00CF0
    bx lr

.global svcDestroyMessageQueue
.type svcDestroyMessageQueue, %function
svcDestroyMessageQueue:
    .word 0xE7F00DF0
    bx lr

.global svcReceiveMessage
.type svcReceiveMessage, %function
svcReceiveMessage:
    .word 0xE7F010F0
    bx lr

.global svcAlloc
.type svcAlloc, %function
svcAlloc:
    .word 0xE7F027F0
    bx lr

.global svcAllocAlign
.type svcAllocAlign, %function
svcAllocAlign:
    .word 0xE7F028F0
    bx lr

.global svcFree
.type svcFree, %function
svcFree:
    .word 0xE7F029F0
    bx lr

.global svcRegisterResourceManager
.type svcRegisterResourceManager, %function
svcRegisterResourceManager:
    .word 0xE7F02CF0
    bx lr

.global svcOpen
.type svcOpen, %function
svcOpen:
    .word 0xE7F033F0
    bx lr

.global svcClose
.type svcClose, %function
svcClose:
    .word 0xE7F034F0
    bx lr

.global svcIoctl
.type svcIoctl, %function
svcIoctl:
    .word 0xE7F038F0
    bx lr

.global svcIoctlv
.type svcIoctlv, %function
svcIoctlv:
    .word 0xE7F039F0
    bx lr

.global svcResourceReply
.type svcResourceReply, %function
svcResourceReply:
    .word 0xE7F049F0
    bx lr

.global svcInvalidateDCache
.type svcInvalidateDCache, %function
svcInvalidateDCache:
    .word 0xE7F051F0
    bx lr

.global svcFlushDCache
.type svcFlushDCache, %function
svcFlushDCache:
    .word 0xE7F052F0
    bx lr

.global svcRead32
.type svcRead32, %function
svcRead32:
    .word 0xE7F081F0
    bx lr

.global svcCustomKernelCommand
.type svcCustomKernelCommand, %function
svcCustomKernelCommand:
    .word 0xE7F081F0
    bx lr