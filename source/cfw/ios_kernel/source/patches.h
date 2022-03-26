
extern ThreadContext_t** currentThreadContext;

#define KERNEL_READ32         1
#define KERNEL_WRITE32        2
#define KERNEL_MEMCPY         3
#define KERNEL_GET_CFW_CONFIG 4
#define KERNEL_READ_OTP       5

void installPatches();