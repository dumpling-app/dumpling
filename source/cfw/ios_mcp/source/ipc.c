#include "fsa.h"
#include "ipc.h"
#include "svc.h"

static bool loopIPCServer;
static uint8_t threadStack[0x1000] __attribute__((aligned(0x20)));

typedef void (*usleep_t)(uint32_t);
typedef void* (*memcpy_t)(void*, const void*, int32_t);

static usleep_t usleep = (usleep_t)0x050564E4;
static memcpy_t memcpy = (memcpy_t)0x05054E54;

static int32_t IPC_ioctl(IPCMessage *message) {
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
        case IOCTL_KILL_SERVER: {
            loopIPCServer = false;
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
            if ((message->ioctl.length_in < 4) || (message->ioctl.length_io < 4)) return IOS_ERROR_INVALID_SIZE;
            else {
                uint32_t size = message->ioctl.length_io/4;
                for (uint32_t i=0; i<size; i++) {
                    message->ioctl.buffer_io[i] = svcCustomKernelCommand(KERNEL_WRITE32, message->ioctl.buffer_in[0] + (i*4));
                }
            }
            break;
        }
        case IOCTL_READ_OTP: {
            if ((message->ioctl.length_io < 0x400)) {
                res = IOS_ERROR_INVALID_SIZE;
            } else {
                svcCustomKernelCommand(KERNEL_READ_OTP, message->ioctl.buffer_io);
            }
            break;
        }
        // All FSA commands
        case IOCTL_FSA_OPEN: {
            // points to "/dev/fsa" string in mcp data section
            message->ioctl.buffer_io[0] = svcOpen((char*)0x0506963C, 0);
            break;
        }
        case IOCTL_FSA_CLOSE: {
            int32_t fd = message->ioctl.buffer_in[0];
            message->ioctl.buffer_io[0] = svcClose(fd);
            break;
        }
        case IOCTL_FSA_MOUNT: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            char *volume_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[2];
            uint32_t flags = message->ioctl.buffer_in[3];
            char *arg_string = (message->ioctl.buffer_in[4] > 0) ? (((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[4]) : 0;
            int32_t arg_string_len = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_Mount(fd, device_path, volume_path, flags, arg_string, arg_string_len);
            break;
        }
        case IOCTL_FSA_UNMOUNT: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            uint32_t flags = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_Unmount(fd, device_path, flags);
            break;
        }
        case IOCTL_FSA_FLUSHVOLUME: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_FlushVolume(fd, path);
            break;
        }
        case IOCTL_FSA_ROLLBACKVOLUME: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RollbackVolume(fd, device_path);
            break;
        }
        case IOCTL_FSA_GETINFO: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *device_path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            int32_t type = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_GetInfo(fd, device_path, type, message->ioctl.buffer_io + 1);
            break;
        }
        case IOCTL_FSA_GETSTAT: {
            int fd            = message->ioctl.buffer_in[0];
            char *device_path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_GetStat(fd, device_path, (FSStat*)message->ioctl.buffer_io + 1);
            break;
        }
        case IOCTL_FSA_MAKEDIR: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            uint32_t flags = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_MakeDir(fd, path, flags);
            break;
        }
        case IOCTL_FSA_OPENDIR: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_OpenDir(fd, path, (int32_t*)message->ioctl.buffer_io + 1);
            break;
        }
        case IOCTL_FSA_READDIR: {
            int32_t fd = message->ioctl.buffer_in[0];
            int32_t handle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_ReadDir(fd, handle, (FSDirectoryEntry*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_REWINDDIR: {
            int32_t fd     = message->ioctl.buffer_in[0];
            int32_t handle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RewindDir(fd, handle);
            break;
        }
        case IOCTL_FSA_CLOSEDIR: {
            int32_t fd = message->ioctl.buffer_in[0];
            int32_t handle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_CloseDir(fd, handle);
            break;
        }
        case IOCTL_FSA_CHDIR: {
            int32_t fd            = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_ChangeDir(fd, path);
            break;
        }
        case IOCTL_FSA_GETCWD: {
            int32_t fd            = message->ioctl.buffer_in[0];
            int32_t output_size = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_GetCwd(fd, (char*)(message->ioctl.buffer_io + 1), output_size);
            break;
        }
        case IOCTL_FSA_MAKEQUOTA: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            uint32_t flags  = message->ioctl.buffer_in[2];
            uint64_t size   = ((uint64_t) message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];

            message->ioctl.buffer_io[0] = FSA_MakeQuota(fd, path, flags, size);
            break;
        }
        case IOCTL_FSA_FLUSHQUOTA: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_FlushQuota(fd, path);
            break;
        }
        case IOCTL_FSA_ROLLBACKQUOTA: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RollbackQuota(fd, path);
            break;
        }
        case IOCTL_FSA_ROLLBACKQUOTAFORCE: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RollbackQuotaForce(fd, path);
            break;
        }
        case IOCTL_FSA_OPENFILE: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            char *mode = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_OpenFile(fd, path, mode, (int32_t*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_OPENFILEEX: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            char *mode = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[2];
            uint32_t createMode = message->ioctl.buffer_in[3];
            uint32_t flags = message->ioctl.buffer_in[4];
            uint32_t preallocSize = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_OpenFileEx(fd, path, mode, createMode, flags, preallocSize, (int32_t*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_READFILE: {
            int32_t fd = message->ioctl.buffer_in[0];
            uint32_t size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            int32_t fileHandle = message->ioctl.buffer_in[3];
            uint32_t flags = message->ioctl.buffer_in[4];

            message->ioctl.buffer_io[0] = FSA_ReadFile(fd, ((uint8_t*)message->ioctl.buffer_io) + 0x40, size, cnt, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_WRITEFILE: {
            int32_t fd = message->ioctl.buffer_in[0];
            uint32_t size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            int32_t fileHandle = message->ioctl.buffer_in[3];
            uint32_t flags = message->ioctl.buffer_in[4];

            message->ioctl.buffer_io[0] = FSA_WriteFile(fd, ((uint8_t*)message->ioctl.buffer_in) + 0x40, size, cnt, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_READFILEWITHPOS: {
            int32_t fd         = message->ioctl.buffer_in[0];
            uint32_t size       = message->ioctl.buffer_in[1];
            uint32_t cnt        = message->ioctl.buffer_in[2];
            uint32_t pos        = message->ioctl.buffer_in[3];
            int32_t fileHandle = message->ioctl.buffer_in[4];
            uint32_t flags      = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_ReadFileWithPos(fd, ((uint8_t *) message->ioctl.buffer_io) + 0x40, size, cnt, pos, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_WRITEFILEWITHPOS: {
            int32_t fd         = message->ioctl.buffer_in[0];
            uint32_t size       = message->ioctl.buffer_in[1];
            uint32_t cnt        = message->ioctl.buffer_in[2];
            uint32_t pos        = message->ioctl.buffer_in[3];
            int32_t fileHandle = message->ioctl.buffer_in[4];
            uint32_t flags      = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_WriteFileWithPos(fd, ((uint8_t *) message->ioctl.buffer_in) + 0x40, size, cnt, pos, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_APPENDFILE: {
            int32_t fd         = message->ioctl.buffer_in[0];
            uint32_t size       = message->ioctl.buffer_in[1];
            uint32_t cnt        = message->ioctl.buffer_in[2];
            int32_t fileHandle = message->ioctl.buffer_in[3];

            message->ioctl.buffer_io[0] = FSA_AppendFile(fd, size, cnt, fileHandle);
            break;
        }
        case IOCTL_FSA_APPENDFILEEX: {
            int32_t fd         = message->ioctl.buffer_in[0];
            uint32_t size       = message->ioctl.buffer_in[1];
            uint32_t cnt        = message->ioctl.buffer_in[2];
            int32_t fileHandle = message->ioctl.buffer_in[3];
            uint32_t flags      = message->ioctl.buffer_in[4];

            message->ioctl.buffer_io[0] = FSA_AppendFileEx(fd, size, cnt, fileHandle, flags);
            break;
        }
        case IOCTL_FSA_GETSTATFILE: {
            int32_t fd = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_GetStatFile(fd, fileHandle, (FSStat*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_CLOSEFILE: {
            int32_t fd = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_CloseFile(fd, fileHandle);
            break;
        }
        case IOCTL_FSA_FLUSHFILE: {
            int32_t fd         = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_FlushFile(fd, fileHandle);
            break;
        }
        case IOCTL_FSA_TRUNCATEFILE: {
            int32_t fd         = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_TruncateFile(fd, fileHandle);
            break;
        }
        case IOCTL_FSA_GETPOSFILE: {
            int32_t fd         = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_GetPosFile(fd, fileHandle, (uint32_t *)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_SETPOSFILE: {
            int32_t fd         = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];
            uint32_t position   = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_SetPosFile(fd, fileHandle, position);
            break;
        }
        case IOCTL_FSA_ISEOF: {
            int32_t fd         = message->ioctl.buffer_in[0];
            int32_t fileHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_IsEof(fd, fileHandle);
            break;
        }
        case IOCTL_FSA_REMOVE: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_Remove(fd, path);
            break;
        }
        case IOCTL_FSA_RENAME: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *oldPath = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            char *newPath = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_Rename(fd, oldPath, newPath);
            break;
        }
        case IOCTL_FSA_CHANGEMODE: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            int32_t mode   = message->ioctl.buffer_in[2];

            message->ioctl.buffer_io[0] = FSA_ChangeMode(fd, path, mode);
            break;
        }
        case IOCTL_FSA_CHANGEMODEEX: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            int32_t mode   = message->ioctl.buffer_in[2];
            int32_t mask   = message->ioctl.buffer_in[3];

            message->ioctl.buffer_io[0] = FSA_ChangeModeEx(fd, path, mode, mask);
            break;
        }
        case IOCTL_FSA_CHANGEOWNER: {
            int32_t fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];
            uint32_t owner  = message->ioctl.buffer_in[2];
            uint32_t group  = message->ioctl.buffer_in[3];

            message->ioctl.buffer_io[0] = FSA_ChangeOwner(fd, path, owner, group);
            break;
        }
        case IOCTL_FSA_RAW_OPEN: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *)message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RawOpen(fd, path, (int32_t*)(message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_RAW_READ: {
            int32_t fd = message->ioctl.buffer_in[0];
            uint32_t block_size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            uint64_t sector_offset = ((uint64_t)message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];
            int32_t deviceHandle = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_RawRead(fd, ((uint8_t*)message->ioctl.buffer_io) + 0x40, block_size, cnt, sector_offset, deviceHandle);
            break;
        }
        case IOCTL_FSA_RAW_WRITE: {
            int32_t fd = message->ioctl.buffer_in[0];
            uint32_t block_size = message->ioctl.buffer_in[1];
            uint32_t cnt = message->ioctl.buffer_in[2];
            uint64_t sector_offset = ((uint64_t)message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];
            int32_t deviceHandle = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_RawWrite(fd, ((uint8_t*)message->ioctl.buffer_in) + 0x40, block_size, cnt, sector_offset, deviceHandle);
            break;
        }
        case IOCTL_FSA_RAW_CLOSE: {
            int32_t fd = message->ioctl.buffer_in[0];
            int32_t deviceHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RawClose(fd, deviceHandle);
            break;
        }
        case IOCTL_FSA_REGISTERFLUSHQUOTA: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RegisterFlushQuota(fd, path);
            break;
        }
        case IOCTL_FSA_FLUSHMULTIQUOTA: {
            int32_t fd = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_FlushMultiQuota(fd, path);
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

static int32_t loopServerThread(void* args) {
    int32_t res = 0;

    uint32_t messageQueue[0x10];
    int32_t queueHandle = svcCreateMessageQueue(messageQueue, sizeof(messageQueue)/4);
    
    if (svcRegisterResourceManager("/dev/iosuhax", queueHandle) != 0) {
        svcDestroyMessageQueue(queueHandle);
        return 0;
    }

    IPCMessage *message;
    loopIPCServer = true;
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

    svcDestroyMessageQueue(queueHandle);
    return res;
}

int startIpcServer() {
    int32_t threadId = svcCreateThread(loopServerThread, 0, (uint32_t*)(threadStack + sizeof(threadStack)), sizeof(threadStack), 0x78, 1);
    if (threadId >= 0)
       svcStartThread(threadId);
    return 1;
}