#include "common.h"
#include "svc.h"

#define FAIL_ON(cond, val)         \
    if (cond) {                    \
        return 29;                \
    }

extern int32_t mainIPC();

typedef void (*usleep_t)(uint32_t);
static usleep_t usleep = (usleep_t)0x050564E4;


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
            case IPC_CUSTOM_START_USB_LOGGING: {
                if (*((uint32_t *)0x050290DC) == 0x42424242) {
                    // Skip syslog after a reload
                    break;
                }
                svcCustomKernelCommand(KERNEL_WRITE32, 0x050290dc, 0x42424242);
                svcCustomKernelCommand(KERNEL_WRITE32, 0x050290d8, 0x20004770);
                svcCustomKernelCommand(KERNEL_WRITE32, 0xe4007828, 0xe3a00000);
                usleep(1000*10);

                int32_t handle = svcOpen("/dev/testproc1", 0);
                if (handle > 0) {
                    svcResume(handle);
                    svcClose(handle);
                }

                handle = svcOpen("/dev/usb_syslog", 0);
                if (handle > 0) {
                    svcResume(handle);
                    svcClose(handle);
                }

                bool showCompleteLog = msg->ioctl.buffer_in && msg->ioctl.length_in >= 0x08 && msg->ioctl.buffer_in[1] == 1;
                if (!showCompleteLog) {
                    // Kill existing syslogs to avoid long catch up
                    uint32_t *bufferPtr = (uint32_t *)(*(uint32_t *)0x05095ecc);
                    bufferPtr[0]        = 0;
                    bufferPtr[1]        = 0;
                }

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