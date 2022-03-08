#include "ipc.h"

static bool threadsStarted = false;

int32_t mainIPC() {
    if (!threadsStarted) {
        threadsStarted = true;
        startIpcServer();
    }
    return 0;
}