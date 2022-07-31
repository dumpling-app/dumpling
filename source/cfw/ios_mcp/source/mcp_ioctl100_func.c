#include "common.h"

#define FAIL_ON(cond, val)         \
    if (cond) {                    \
        return 29;                \
    }

extern int32_t mainIPC();

int _MCP_ioctl100_patch(IPCMessage *msg) {
    FAIL_ON(!msg->ioctl.buffer_in, 0);
    FAIL_ON(!msg->ioctl.length_in, 0);

    if (msg->ioctl.buffer_in && msg->ioctl.length_in >= 4) {
        int32_t command = msg->ioctl.buffer_in[0];

        switch (command) {
            case IPC_CUSTOM_START_MCP_THREAD: {
                mainIPC();
                return 0;
                break;
            }
            case IPC_CUSTOM_GET_MOCHA_API_VERSION: {
                if (msg->ioctl.buffer_io && msg->ioctl.length_io >= 8) {
                    msg->ioctl.buffer_io[0] = 0xCAFEBABE;
                    msg->ioctl.buffer_io[1] = MOCHA_API_VERSION;
                    return 0;
                }
                else {
                    return 29;
                }
                break;
            }
            default: {
            }
        }
    }
    else {
        return 29;
    }
    return 0;
}