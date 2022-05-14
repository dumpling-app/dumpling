#include "common.h"

#define IPC_CUSTOM_START_MCP_THREAD       0xFE

#define FAIL_ON(cond, val)         \
    if (cond) {                    \
        return -29;                \
    }

extern int32_t mainIPC();

int _MCP_ioctl100_patch(IPCMessage *msg) {
    // Give some method to detect this ioctl's prescence, even if the other args are bad
    if (msg->ioctl.buffer_io && msg->ioctl.length_io >= sizeof(uint32_t)) {
        *(uint32_t*)msg->ioctl.buffer_io = 3;
    }

    FAIL_ON(!msg->ioctl.buffer_in, 0);
    FAIL_ON(!msg->ioctl.length_in, 0);

    if (msg->ioctl.buffer_in && msg->ioctl.length_in >= 4) {
        int32_t command = msg->ioctl.buffer_in[0];

        switch (command) {
            case IPC_CUSTOM_START_MCP_THREAD: {
                mainIPC();
                break;
            }
        }
    }
    else {
        return 29;
    }
    return 0;
}