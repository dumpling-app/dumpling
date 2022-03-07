#include "fsa.h"
#include "ipc.h"
#include "svc.h"

int32_t IPC_ioctl(IPCMessage *message) {
    int32_t res = 0;

    switch(message->ioctl.command) {
        case IOCTL_MEM_WRITE: {
            if (message->ioctl.length_in < 4) return IOS_ERROR_INVALID_SIZE;
            else memcpy((void*)message->ioctl.buffer_in[0], message->ioctl.buffer_in + 1, message->ioctl.length_in - 4);
            break;
        }
        case IOCTL_MEM_READ: {
            if (message->ioctl.length_in < 4) return IOS_ERROR_INVALID_SIZE;
            else memcpy(message->ioctl.buffer_io, (const void*)message->ioctl.buffer_in[0], message->ioctl.length_io);
            break;
        }
        case IOCTL_SVC: {
            if ((message->ioctl.length_in < 4) || (message->ioctl.length_io < 4)) return IOS_ERROR_INVALID_SIZE;
            else {
                int32_t svc_id = message->ioctl.buffer_in[0];
                int32_t size_args = message->ioctl.buffer_in[0];

                uint32_t arguments[8];
                memset(arguments, 0, sizeof(arguments));
                memcpy(arguments, message->ioctl.buffer_in + 1, (size_args < 32) ? size_args : 32);

                message->ioctl.buffer_io[0] = ((int (*const)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))((void*)0x050567EC/*MCP_SVC_BASE*/ + svc_id * 8))(arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], arguments[7]);
            }
            break;
        }
        case IOCTL_MEMCPY: {
            if (message->ioctl.length_in < 12) return IOS_ERROR_INVALID_SIZE;
            else memcpy((void*)message->ioctl.buffer_in[0], (const void*)message->ioctl.buffer_in[1], message->ioctl.buffer_in[2]);
            break;
        }
        case IOCTL_REPEATED_WRITE: {
            if (message->ioctl.length_in < 12) return IOS_ERROR_INVALID_SIZE;
            else {
                uint32_t* dest = (uint32_t*)message->ioctl.buffer_in[0];
                uint32_t* cache_range = (uint32_t*)(message->ioctl.buffer_in[0] & ~0xFF);
                uint32_t value = message->ioctl.buffer_in[1];
                uint32_t n = message->ioctl.buffer_in[2];

                uint32_t old = *dest;
                for (uint32_t i=0; i<n; i++) {
                    if (*dest != old) {
                        if (*dest == 0) old = *dest;
                        else {
                            *dest = value;
                            svcFlushDCache(cache_range, 256);
                        }
                    }
                    else {
                        svcInvalidateDCache(cache_range, 256);
                        usleep(50);
                    }
                }
            }
            break;
        }
        case IOCTL_KERN_READ32: {
            // todo: Implement this in the kernel
            if ((message->ioctl.length_in < 4) || (message->ioctl.length_io < 4)) return IOS_ERROR_INVALID_SIZE;
            else {
                uint32_t size = message->ioctl.length_io/4;
                for (uint32_t i=0; i<size; i++) {
                    message->ioctl.buffer_io[i] = svcCustomKernelCommand(KERNEL_READ32, message->ioctl.buffer_in[0] + (i*4));
                }
            }
            break;
        }
        case IOCTL_KERN_WRITE32: {
            // todo: Implement this in the kernel
            if ((message->ioctl.length_in < 4) || (message->ioctl.length_io < 4)) return IOS_ERROR_INVALID_SIZE;
            else {
                uint32_t size = message->ioctl.length_io/4;
                for (uint32_t i=0; i<size; i++) {
                    message->ioctl.buffer_io[i] = svcCustomKernelCommand(KERNEL_WRITE32, message->ioctl.buffer_in[0] + (i*4));
                }
            }
            break;
        }
        case IOCTL_FSA_OPEN: {
            // points to "/dev/fsa" string in mcp data section
            message->ioctl.buffer_io[0] = svcOpen((char*)0x0506963C, 0);
            break;
        }
        case IOCTL_FSA_CLOSE: {
            int fd = message->ioctl.buffer_in[0];
            message->ioctl.buffer_io[0] = svcClose(fd);
            break;
        }
        case IOCTL_FSA_MOUNT: {
            int fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            char *volume_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[2];
            uint32_t flags = message->ioctl.buffer_in[3];
            char *arg_string = (message->ioctl.buffer_in[4] > 0) ? (((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[4]) : 0;
            int arg_string_len = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_Mount(fd, device_path, volume_path, flags, arg_string, arg_string_len);
            break;
        }
        case IOCTL_FSA_UNMOUNT: {
            int fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            uint32_t flags = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_Unmount(fd, device_path, flags);
            break;
        }
        case IOCTL_FSA_FLUSHVOLUME: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_FlushVolume(fd, path);
            break;
        }
        case IOCTL_FSA_GETDEVICEINFO: {
            int fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            int type = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_GetDeviceInfo(fd, device_path, type, message->ioctl.buffer_io + 1);
            break;
        }
        case IOCTL_FSA_OPENDIR: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_OpenDir(fd, path, (int*)message->ioctl.buffer_io + 1);
            break;
        }
        case IOCTL_FSA_READDIR: {
            int fd = message->ioctl.buffer_in[0];
            int handle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_ReadDir(fd, handle, (FSDirectoryEntry*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_CLOSEDIR: {
            int fd = message->ioctl.buffer_in[0];
            int handle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_CloseDir(fd, handle);
            break;
        }
        case IOCTL_FSA_MAKEDIR: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            uint32_t flags = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_MakeDir(fd, path, flags);
            break;
        }
        case IOCTL_FSA_OPENFILE: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            char *mode = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_OpenFile(fd, path, mode, (int*)message->ioctl.buffer_io + 1);
            break;
        }
        case IOCTL_FSA_READFILE: {
            int fd = message->ioctl.buffer_in[0];
            uint32_t size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            int fileHandle = message->ioctl.buffer_in[3];
            uint32_t flags = message->ioctl.buffer_in[4];

            message->ioctl.buffer_io[0] = FSA_ReadFile(fd, ((uint8_t*)message->ioctl.buffer_io) + 0x40, size, cnt, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_WRITEFILE: {
            int fd = message->ioctl.buffer_in[0];
            uint32_t size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            int fileHandle = message->ioctl.buffer_in[3];
            uint32_t flags = message->ioctl.buffer_in[4];

            message->ioctl.buffer_io[0] = FSA_WriteFile(fd, ((uint8_t*)message->ioctl.buffer_in) + 0x40, size, cnt, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_STATFILE: {
            int fd = message->ioctl.buffer_in[0];
            int fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_StatFile(fd, fileHandle, (FSStat*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_CLOSEFILE: {
            int fd = message->ioctl.buffer_in[0];
            int fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_CloseFile(fd, fileHandle);
            break;
        }
        case IOCTL_FSA_SETFILEPOS: {
            int fd = message->ioctl.buffer_in[0];
            int fileHandle = message->ioctl.buffer_in[1];
            uint32_t position = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_SetPosFile(fd, fileHandle, position);
            break;
        }
        case IOCTL_FSA_GETSTAT: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_GetStat(fd, path, (FSStat*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_REMOVE: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_Remove(fd, path);
            break;
        }
        case IOCTL_FSA_REWINDDIR: {
            int fd = message->ioctl.buffer_in[0];
            int dirFd = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RewindDir(fd, dirFd);
            break;
        }
        case IOCTL_FSA_CHDIR: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_ChangeDir(fd, path);
            break;
        }
        case IOCTL_FSA_RAW_OPEN: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RawOpen(fd, path, (int*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_RAW_READ: {
            int fd = message->ioctl.buffer_in[0];
            uint32_t block_size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            uint64_t sector_offset = ((uint64_t)message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];
            int deviceHandle = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_RawRead(fd, ((uint8_t*)message->ioctl.buffer_io) + 0x40, block_size, cnt, sector_offset, deviceHandle);
            break;
        }
        case IOCTL_FSA_RAW_WRITE: {
            int fd = message->ioctl.buffer_in[0];
            uint32_t block_size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            uint64_t sector_offset = ((uint64_t)message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];
            int deviceHandle = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_RawWrite(fd, ((uint8_t*)message->ioctl.buffer_in) + 0x40, block_size, cnt, sector_offset, deviceHandle);
            break;
        }
        case IOCTL_FSA_RAW_CLOSE: {
            int fd = message->ioctl.buffer_in[0];
            int deviceHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RawClose(fd, deviceHandle);
            break;
        }
        case IOCTL_FSA_CHANGEMODE: {
            int fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            int mode = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_ChangeMode(fd, path, mode);
            break;
        }
        case IOCTL_CHECK_IF_IOSUHAX: {
            message->ioctl.buffer_io[0] = IOSUHAX_MAGIC_WORD;
            break;
        }
        default: {
            res = IOS_ERROR_INVALID_ARG;
            break;
        }
    }

    return res;
}

int32_t loopServerThread() {
    int32_t res = 0;

    uint32_t messageQueue[0x10];
    // int32_t queueHandle = svcCreateMessageQueue(messageQueue, sizeof(messageQueue)/4);
    
    // if (svcRegisterResourceManager("/dev/iosuhax", queueHandle) != 0) {
    //     svcDestroyMessageQueue(queueHandle);
    //     queueHandle = *(int32_t*)0x5070AEC;
    //     return 0;
    // }

    int32_t queueHandle = *(int32_t*)0x5070AEC;

    bool loopIPCServer = true;
    IPCMessage *message;
    while(loopIPCServer) {
        if (svcReceiveMessage(queueHandle, &message, 0) < 0) {
            usleep(1000*5);
            continue;
        }

        switch(message->command) {
            case IOS_OPEN: {
                res = 0;
                break;
            }
            case IOS_CLOSE: {
                res = 0;
                loopIPCServer = false;
                break;
            }
            case IOS_IOCTL: {
                res = IPC_ioctl(message);
                break;
            }
            default: {
                res = IOS_ERROR_UNKNOWN_VALUE;
                break;
            }
        }
        svcResourceReply(message, res);
    }

    // svcDestroyMessageQueue(queueHandle);
    return res;
}

#define STACK_SIZE (1000)

int32_t startIpcServer() {
    // uint8_t* threadStack = svcAllocAlign(0xCAFF, STACK_SIZE, 0x20);
    return loopServerThread();
    // int threadId = svcCreateThread(loopServerThread, 0, (uint32_t*)(threadStack + STACK_SIZE), STACK_SIZE, 0x78, 1);
    // if (threadId >= 0)
    //    svcStartThread(threadId);
    //return 1;
}