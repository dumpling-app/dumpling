#include "fsa.h"
#include "svc.h"

typedef void* (*memcpy_t)(void*, const void*, int32_t);
static memcpy_t memcpy = (memcpy_t)0x05054E54;

void* memset(void* dest, int32_t value, int32_t size) {
    // todo: Find memset function for speedup or use longer types to do faster copying
    for (int32_t i=0; i<size; i++) {
        ((char*)dest)[i] = value;
    }
    return dest;
}

char* strncpy(char* dest, const char* src, int32_t size) {
    for (int32_t i=0; i<size; i++) {
        dest[i] = src[i];
        if (dest[i] == '\0') return dest;
    }
    return dest;
}

static void* allocIobuf() {
    void* ptr = svcAlloc(0xCAFF, 0x828);
    memset(ptr, 0x00, 0x828);
    return ptr;
}

static void freeIobuf(void* ptr) {
    svcFree(0xCAFF, ptr);
}

int32_t FSA_Mount(int32_t fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int32_t arg_string_len) {
    uint8_t* iobuf = allocIobuf();
    uint8_t* inbuf8 = iobuf;
    uint8_t* outbuf8 = &iobuf[0x520];
    iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
    uint32_t* inbuf = (uint32_t*)inbuf8;
    uint32_t* outbuf = (uint32_t*)outbuf8;

    strncpy((char*)&inbuf8[0x04], device_path, 0x27F);
    strncpy((char*)&inbuf8[0x284], volume_path, 0x27F);
    inbuf[0x504 / 4] = (uint32_t)flags;
    inbuf[0x508 / 4] = (uint32_t)arg_string_len;

    iovec[0].ptr = inbuf;
    iovec[0].length = 0x520;
    iovec[1].ptr = arg_string;
    iovec[1].length = arg_string_len;
    iovec[2].ptr = outbuf;
    iovec[2].length = 0x293;

    int32_t ret = svcIoctlv(fd, 0x01, 2, 1, iovec);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_Unmount(int32_t fd, char* path, uint32_t flags) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);
    inbuf[0x284 / 4] = flags;

    int32_t ret = svcIoctl(fd, 0x02, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_FlushVolume(int32_t fd, char* volume_path) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], volume_path, 0x27F);

    int32_t ret = svcIoctl(fd, 0x1B, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_MakeDir(int32_t fd, char* path, uint32_t flags) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);
    inbuf[0x284 / 4] = flags;

    int32_t ret = svcIoctl(fd, 0x07, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_OpenDir(int32_t fd, char* path, int32_t* outHandle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);

    int32_t ret = svcIoctl(fd, 0x0A, inbuf, 0x520, outbuf, 0x293);

    if (outHandle) *outHandle = outbuf[1];

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_ReadDir(int32_t fd, int32_t handle, FSDirectoryEntry* out_data) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = handle;

    int32_t ret = svcIoctl(fd, 0x0B, inbuf, 0x520, outbuf, 0x293);

    if (out_data) memcpy(out_data, &outbuf[1], sizeof(FSDirectoryEntry));

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_RewindDir(int32_t fd, int32_t handle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = handle;

    int32_t ret = svcIoctl(fd, 0x0C, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_CloseDir(int32_t fd, int32_t handle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = handle;

    int32_t ret = svcIoctl(fd, 0x0D, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_ChangeDir(int32_t fd, char* path) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);

    int32_t ret = svcIoctl(fd, 0x05, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_OpenFile(int32_t fd, char* path, char* mode, int32_t* outHandle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);
    strncpy((char*)&inbuf[0xA1], mode, 0x10);

    int32_t ret = svcIoctl(fd, 0x0E, inbuf, 0x520, outbuf, 0x293);

    if (outHandle) *outHandle = outbuf[1];

    freeIobuf(iobuf);
    return ret;
}

int32_t _FSA_ReadWriteFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags, bool read) {
    uint8_t* iobuf = allocIobuf();
    uint8_t* inbuf8 = iobuf;
    uint8_t* outbuf8 = &iobuf[0x520];
    iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
    uint32_t* inbuf = (uint32_t*)inbuf8;
    uint32_t* outbuf = (uint32_t*)outbuf8;

    inbuf[0x08 / 4] = size;
    inbuf[0x0C / 4] = cnt;
    inbuf[0x14 / 4] = fileHandle;
    inbuf[0x18 / 4] = flags;

    iovec[0].ptr = inbuf;
    iovec[0].length = 0x520;

    iovec[1].ptr = data;
    iovec[1].length = size * cnt;

    iovec[2].ptr = outbuf;
    iovec[2].length = 0x293;

    int32_t ret;
    if (read) ret = svcIoctlv(fd, 0x0F, 1, 2, iovec);
    else ret = svcIoctlv(fd, 0x10, 2, 1, iovec);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_ReadFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags) {
    return _FSA_ReadWriteFile(fd, data, size, cnt, fileHandle, flags, true);
}

int32_t FSA_WriteFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags) {
    return _FSA_ReadWriteFile(fd, data, size, cnt, fileHandle, flags, false);
}

int32_t FSA_StatFile(int32_t fd, int32_t handle, FSStat* out_data) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = handle;

    int32_t ret = svcIoctl(fd, 0x14, inbuf, 0x520, outbuf, 0x293);

    if (out_data) memcpy(out_data, &outbuf[1], sizeof(FSStat));

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_CloseFile(int32_t fd, int32_t fileHandle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = fileHandle;

    int32_t ret = svcIoctl(fd, 0x15, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_SetPosFile(int32_t fd, int32_t fileHandle, uint32_t position) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = fileHandle;
    inbuf[2] = position;

    int32_t ret = svcIoctl(fd, 0x12, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_GetStat(int32_t fd, char *path, FSStat* out_data) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);
    inbuf[0x284/4] = 5;

    int32_t ret = svcIoctl(fd, 0x18, inbuf, 0x520, outbuf, 0x293);

    if (out_data) memcpy(out_data, &outbuf[1], sizeof(FSStat));

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_Remove(int32_t fd, char *path) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);

    int32_t ret = svcIoctl(fd, 0x08, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_ChangeMode(int32_t fd, char *path, int32_t mode) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], path, 0x27F);
    inbuf[0x284/4] = mode;
    inbuf[0x288/4] = 0x777; // mask

    int32_t ret = svcIoctl(fd, 0x20, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

// type 4:
// - 0x08: device size in sectors (uint64_t)
// - 0x10: device sector size (uint32_t)
int32_t FSA_GetDeviceInfo(int32_t fd, char* device_path, int32_t type, uint32_t* out_data) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], device_path, 0x27F);
    inbuf[0x284 / 4] = type;

    int32_t ret = svcIoctl(fd, 0x18, inbuf, 0x520, outbuf, 0x293);

    int32_t size = 0;

    switch(type) {
        case 0:
        case 1:
        case 7:
            size = 0x8;
            break;
        case 2:
            size = 0x4;
            break;
        case 3:
            size = 0x1E;
            break;
        case 4:
            size = 0x28;
            break;
        case 5:
            size = 0x64;
            break;
        case 6:
        case 8:
            size = 0x14;
            break;
        default:
            size = 0;
    }

    memcpy(out_data, &outbuf[1], size);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_RawOpen(int32_t fd, char* device_path, int32_t* outHandle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    strncpy((char*)&inbuf[0x01], device_path, 0x27F);

    int32_t ret = svcIoctl(fd, 0x6A, inbuf, 0x520, outbuf, 0x293);

    if (outHandle) *outHandle = outbuf[1];

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_RawClose(int32_t fd, int32_t device_handle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

    inbuf[1] = device_handle;

    int32_t ret = svcIoctl(fd, 0x6D, inbuf, 0x520, outbuf, 0x293);

    freeIobuf(iobuf);
    return ret;
}

// offset in blocks of 0x1000 bytes
int32_t FSA_RawRead(int32_t fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle) {
    uint8_t* iobuf = allocIobuf();
    uint8_t* inbuf8 = iobuf;
    uint8_t* outbuf8 = &iobuf[0x520];
    iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
    uint32_t* inbuf = (uint32_t*)inbuf8;
    uint32_t* outbuf = (uint32_t*)outbuf8;

    // note : offset_bytes = blocks_offset * size_bytes
    inbuf[0x08 / 4] = (blocks_offset >> 32);
    inbuf[0x0C / 4] = (blocks_offset & 0xFFFFFFFF);
    inbuf[0x10 / 4] = cnt;
    inbuf[0x14 / 4] = size_bytes;
    inbuf[0x18 / 4] = device_handle;

    iovec[0].ptr = inbuf;
    iovec[0].length = 0x520;

    iovec[1].ptr = data;
    iovec[1].length = size_bytes * cnt;

    iovec[2].ptr = outbuf;
    iovec[2].length = 0x293;

    int32_t ret = svcIoctlv(fd, 0x6B, 1, 2, iovec);

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_RawWrite(int32_t fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle) {
    uint8_t* iobuf = allocIobuf();
    uint8_t* inbuf8 = iobuf;
    uint8_t* outbuf8 = &iobuf[0x520];
    iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
    uint32_t* inbuf = (uint32_t*)inbuf8;
    uint32_t* outbuf = (uint32_t*)outbuf8;
    
    inbuf[0x08 / 4] = (blocks_offset >> 32);
    inbuf[0x0C / 4] = (blocks_offset & 0xFFFFFFFF);
    inbuf[0x10 / 4] = cnt;
    inbuf[0x14 / 4] = size_bytes;
    inbuf[0x18 / 4] = device_handle;
    
    iovec[0].ptr = inbuf;
    iovec[0].length = 0x520;
    
    iovec[1].ptr = data;
    iovec[1].length = size_bytes * cnt;
    
    iovec[2].ptr = outbuf;
    iovec[2].length = 0x293;
    
    int32_t ret = svcIoctlv(fd, 0x6C, 2, 1, iovec);
    
    freeIobuf(iobuf);
    return ret;
}