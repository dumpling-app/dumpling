#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "../../shared.h"

typedef struct __attribute__((packed)) {
    uint32_t initialized;
    uint64_t titleId;
    uint32_t processId;
    uint32_t groupId;
    uint32_t unk0;
    uint64_t capabilityMask;
    uint8_t unk1[0x4518];
    char unk2[0x280];
    char unk3[0x280];
    void *mutex;
} FSAProcessData;
static_assert(sizeof(FSAProcessData) == 0x4A3C, "FSAProcessData: wrong size");

typedef struct __attribute__((packed)) {
    uint32_t opened;
    FSAProcessData *processData;
    char unk0[0x10];
    char unk1[0x90];
    uint32_t unk2;
    char work_dir[0x280];
    uint32_t unk3;
} FSAClientHandle;
static_assert(sizeof(FSAClientHandle) == 0x330, "FSAClientHandle: wrong size");

#define PATCHED_CLIENT_HANDLES_MAX_COUNT 0x40

FSAClientHandle* patchedClientHandles[PATCHED_CLIENT_HANDLES_MAX_COUNT];


int32_t (*const IOS_ResourceReply)(void *, int32_t) = (void *) 0x107f6b4c;
int32_t FSA_ioctl0x28_hook(FSAClientHandle *handle, void *request) {
    int32_t res = -5;
    for (int32_t i = 0; i < PATCHED_CLIENT_HANDLES_MAX_COUNT; i++) {
        if (patchedClientHandles[i] == handle) {
            res = 0;
            break;
        }
        if (patchedClientHandles[i] == 0) {
            patchedClientHandles[i] = handle;
            res = 0;
            break;
        }
    }

    IOS_ResourceReply(request, res);
    return 0;
}


typedef struct __attribute__((packed)) {
    IPCMessage ipcMessage;
} ResourceRequest;

int32_t (*const real_FSA_IOCTLV)(ResourceRequest *, uint32_t, uint32_t) = (void *) 0x10703164;
int32_t (*const get_handle_from_val)(uint32_t)                          = (void *) 0x107046d4;
int32_t FSA_IOCTLV_HOOK(ResourceRequest *request, uint32_t u2, uint32_t u3) {
    FSAClientHandle *clientHandle = (FSAClientHandle *) get_handle_from_val(request->ipcMessage.fd);
    uint64_t oldValue             = clientHandle->processData->capabilityMask;
    bool toBeRestored             = false;
    for (int32_t i = 0; i < PATCHED_CLIENT_HANDLES_MAX_COUNT; i++) {
        if (patchedClientHandles[i] == clientHandle) {
            // printf("IOCTL: Force mask to 0xFFFFFFFFFFFFFFFF for client %08X\n", (unsigned int) clientHandle);
            clientHandle->processData->capabilityMask = 0xffffffffffffffffL;
            toBeRestored = true;
            break;
        }
    }
    int32_t res = real_FSA_IOCTLV(request, u2, u3);

    if (toBeRestored) {
        // printf("IOCTL: Restore mask for client %08X\n", (unsigned int) clientHandle);
        clientHandle->processData->capabilityMask = oldValue;
    }

    return res;
}

int32_t (*const real_FSA_IOCTL)(ResourceRequest *, uint32_t, uint32_t, uint32_t) = (void *) 0x107010a8;
int32_t FSA_IOCTL_HOOK(ResourceRequest *request, uint32_t u2, uint32_t u3, uint32_t u4) {
    FSAClientHandle *clientHandle = (FSAClientHandle *) get_handle_from_val(request->ipcMessage.fd);
    uint64_t oldValue             = clientHandle->processData->capabilityMask;
    bool toBeRestored             = false;
    for (int32_t i = 0; i < PATCHED_CLIENT_HANDLES_MAX_COUNT; i++) {
        if (patchedClientHandles[i] == clientHandle) {
            // printf("IOCTL: Force mask to 0xFFFFFFFFFFFFFFFF for client %08X\n", (unsigned int) clientHandle);
            clientHandle->processData->capabilityMask = 0xffffffffffffffffL;
            toBeRestored = true;
            break;
        }
    }
    int32_t res = real_FSA_IOCTL(request, u2, u3, u4);

    if (toBeRestored) {
        // printf("IOCTL: Restore mask for client %08X\n", (unsigned int) clientHandle);
        clientHandle->processData->capabilityMask = oldValue;
    }

    return res;
}

int32_t (*const real_FSA_IOS_Close)(uint32_t fd, ResourceRequest *request) = (void *) 0x10704864;
int32_t FSA_IOS_Close_Hook(uint32_t fd, ResourceRequest *request) {
    FSAClientHandle *clientHandle = (FSAClientHandle *) get_handle_from_val(fd);
    for (int32_t i = 0; i < PATCHED_CLIENT_HANDLES_MAX_COUNT; i++) {
        if (patchedClientHandles[i] == clientHandle) {
            // printf("Close: %p will be closed, reset slot %d\n", clientHandle, i);
            patchedClientHandles[i] = 0;
            break;
        }
    }
    return real_FSA_IOS_Close(fd, request);
}
