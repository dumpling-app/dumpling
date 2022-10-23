#include "common.h"

typedef struct {
    void* ptr;
    uint32_t length;
    uint32_t unknown;
} iovec_s;

void* svcAlloc(uint32_t heapHandle, uint32_t size);
void* svcAllocAlign(uint32_t heapHandle, uint32_t size, uint32_t alignment);
void svcFree(uint32_t heapHandle, void* ptr);
int32_t svcOpen(char* name, int32_t mode);
int32_t svcClose(int32_t svcHandle);
int32_t svcSuspend(int32_t svcHandle);
int32_t svcResume(int32_t svcHandle);
int32_t svcIoctl(int32_t svcHandle, uint32_t request, void* inputBuffer, uint32_t inputBufferLen, void* outputBuffer, uint32_t outputBufferLen);
int32_t svcIoctlv(int32_t svcHandle, uint32_t request, uint32_t vectorCountIn, uint32_t vectorCountOut, iovec_s* vector);
int32_t svcInvalidateDCache(void* address, uint32_t size);
int32_t svcFlushDCache(void* address, uint32_t size);

int32_t svcCreateThread(int32_t (*callback)(void* args), void* args, uint32_t* stackTop, uint32_t stackSize, int32_t priority, int32_t detached);
int32_t svcStartThread(int32_t threadHandle);
int32_t svcCreateMessageQueue(uint32_t *ptr, uint32_t numMessages);
int32_t svcDestroyMessageQueue(int32_t queueHandle);
int32_t svcRegisterResourceManager(const char* device, int32_t queueHandle);
int32_t svcReceiveMessage(int32_t queueHandle, IPCMessage** ipcMessageBuffer, uint32_t flags);
int32_t svcResourceReply(IPCMessage* ipcMessage, uint32_t result);
uint32_t svcRead32(uint32_t address);
int32_t svcCustomKernelCommand(uint32_t command, ...);
