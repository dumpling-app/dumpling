#include "ipc.h"
#include "svc.h"

static uint8_t threadStack[0x200] __attribute__((aligned(0x20)));

int32_t mainIPC() {
    return loopServerThread();
}