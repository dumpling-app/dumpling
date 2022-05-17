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

static int32_t _ioctl_fd_path_internal(int32_t fd, int32_t ioctl_num, int32_t num_args, char* path, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t *out_data, uint32_t out_data_size) {
    uint8_t *iobuf   = allocIobuf();
    uint32_t *inbuf  = (uint32_t *) iobuf;
    uint32_t *outbuf = (uint32_t *) &iobuf[0x520];

    switch (num_args) {
        case 5:
            inbuf[0x290 / 4] = arg4;
        case 4:
            inbuf[0x28C / 4] = arg3;
        case 3:
            inbuf[0x288 / 4] = arg2;
        case 2:
            inbuf[0x284 / 4] = arg1;
        case 1:
            strncpy((char *) &inbuf[0x01], path, 0x27F);
    }
    int32_t ret = svcIoctl(fd, ioctl_num, inbuf, 0x520, outbuf, 0x293);
    if (out_data && out_data_size) memcpy(out_data, &outbuf[1], out_data_size);

    freeIobuf(iobuf);
    return ret;
}

static int32_t _ioctl_fd_handle_internal(int32_t fd, int32_t ioctl_num, int32_t num_args, int32_t handle, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t *out_data, uint32_t out_data_size) {
    uint8_t *iobuf   = allocIobuf();
    uint32_t *inbuf  = (uint32_t *) iobuf;
    uint32_t *outbuf = (uint32_t *) &iobuf[0x520];

    switch (num_args) {
        case 5:
            inbuf[0x05] = arg3;
        case 4:
            inbuf[0x04] = arg3;
        case 3:
            inbuf[0x03] = arg2;
        case 2:
            inbuf[0x02] = arg1;
        case 1:
            inbuf[0x01] = handle;
    }
    int32_t ret = svcIoctl(fd, ioctl_num, inbuf, 0x520, outbuf, 0x293);
    if (out_data && out_data_size) memcpy(out_data, &outbuf[1], out_data_size);

    freeIobuf(iobuf);
    return ret;
}

// Now make all required wrappers
// clang-format off
#define dispatch_ioctl_arg5_out(fd, ioctl_num, handle_or_path, arg1, arg2, arg3, arg4, out_data, out_data_size) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 5, handle_or_path, arg1, arg2, arg3, arg4, out_data, out_data_size)
#define dispatch_ioctl_arg4_out(fd, ioctl_num, handle_or_path, arg1, arg2, arg3, out_data, out_data_size) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 4, handle_or_path, arg1, arg2, arg3, 0, out_data, out_data_size)
#define dispatch_ioctl_arg3_out(fd, ioctl_num, handle_or_path, arg1, arg2, out_data, out_data_size) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 3, handle_or_path, arg1, arg2, 0, 0, out_data, out_data_size)
#define dispatch_ioctl_arg2_out(fd, ioctl_num, handle_or_path, arg1, out_data, out_data_size) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 2, handle_or_path, arg1, 0, 0, 0, out_data, out_data_size)
#define dispatch_ioctl_arg1_out(fd, ioctl_num, handle_or_path, out_data, out_data_size) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 1, handle_or_path, 0, 0, 0, 0, out_data, out_data_size)

#define dispatch_ioctl_arg5(fd, ioctl_num, handle_or_path, arg1, arg2, arg3, arg4) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 5, handle_or_path, arg1, arg2, arg3, arg4, 0, 0)
#define dispatch_ioctl_arg4(fd, ioctl_num, handle_or_path, arg1, arg2, arg3) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 4, handle_or_path, arg1, arg2, arg3, 0, 0, 0)
#define dispatch_ioctl_arg3(fd, ioctl_num, handle_or_path, arg1, arg2) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 3, handle_or_path, arg1, arg2, 0, 0, 0, 0)
#define dispatch_ioctl_arg2(fd, ioctl_num, handle_or_path, arg1) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 2, handle_or_path, arg1, 0, 0, 0, 0, 0)
#define dispatch_ioctl_arg1(fd, ioctl_num, handle_or_path) _Generic((handle_or_path), char*: _ioctl_fd_path_internal, int32_t: _ioctl_fd_handle_internal)(fd, ioctl_num, 1, handle_or_path, 0, 0, 0, 0, 0, 0)

#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7, MACRO_NAME, ...) MACRO_NAME
#define dispatch_ioctl_out(fd, ioctl_num, ...) GET_MACRO(__VA_ARGS__, dispatch_ioctl_arg5_out, dispatch_ioctl_arg4_out, dispatch_ioctl_arg3_out, dispatch_ioctl_arg2_out, dispatch_ioctl_arg1_out, NULL, NULL)(fd, ioctl_num, __VA_ARGS__)
#define dispatch_ioctl(fd, ioctl_num, ...) GET_MACRO(__VA_ARGS__,  NULL, NULL, dispatch_ioctl_arg5, dispatch_ioctl_arg4, dispatch_ioctl_arg3, dispatch_ioctl_arg2, dispatch_ioctl_arg1)(fd, ioctl_num, __VA_ARGS__)
// clang-format on

int32_t _FSA_ReadWriteFileWithPos(int32_t fd, void* data, uint32_t size, uint32_t cnt, uint32_t pos, int32_t fileHandle, uint32_t flags, bool read) {
    uint8_t* iobuf = allocIobuf();
    uint8_t* inbuf8 = iobuf;
    uint8_t* outbuf8 = &iobuf[0x520];
    iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
    uint32_t* inbuf = (uint32_t*)inbuf8;
    uint32_t* outbuf = (uint32_t*)outbuf8;

    inbuf[0x08 / 4] = size;
    inbuf[0x0C / 4] = cnt;
    inbuf[0x10 / 4] = pos;
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

// offset in blocks of 0x1000 bytes
int32_t _FSA_RawReadWrite(int32_t fd, void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle, bool read) {
    uint8_t *iobuf      = allocIobuf();
    uint32_t *inbuf     = (uint32_t *) iobuf;
    uint32_t *outbuf    = (uint32_t *) &iobuf[0x520];
    iovec_s *iovec = (iovec_s *) &iobuf[0x7C0];

    // offset_bytes = blocks_offset * size_bytes
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

    int32_t ret;
    if (read) ret = svcIoctlv(fd, 0x6B, 1, 2, iovec);
    else ret = svcIoctlv(fd, 0x6C, 2, 1, iovec);

    freeIobuf(iobuf);
    return ret;
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
    return dispatch_ioctl(fd, 0x02, path, flags);
}

// type:
// 0 = FSA_GetFreeSpaceSize
// 1 = FSA_GetDirSize
// 2 = FSA_GetEntryNum
// 3 = FSA_GetFileSystemInfo
// 4 = FSA_GetDeviceInfo
// 5 = FSA_GetStat
// 6 = FSA_GetBadBlockInfo
// 7 = FSA_GetJournalFreeSpace
// 8 = FSA_GetFragmentBlockInfo
int32_t FSA_GetInfo(int32_t fd, char *device_path, int32_t type, uint32_t *out_data) {
    int32_t size = 0;
    switch (type) {
        case 0:
        case 1:
        case 7:
            size = sizeof(uint64_t);
            break;
        case 2:
            size = sizeof(uint32_t);
            break;
        case 3:
            size = sizeof(FSFileSystemInfo);
            break;
        case 4:
            size = sizeof(FSDeviceInfo);
            break;
        case 5:
            size = sizeof(FSStat);
            break;
        case 6:
        case 8:
            size = sizeof(FSBlockInfo);
            break;
    }

    return dispatch_ioctl_out(fd, 0x18, device_path, type, out_data, size);
}

int32_t FSA_OpenDir(int32_t fd, char* path, int32_t* outHandle) {
    return dispatch_ioctl_out(fd, 0x0A, path, (uint32_t *) outHandle, sizeof(int32_t));
}

int32_t FSA_ReadDir(int32_t fd, int32_t handle, FSDirectoryEntry* out_data) {
    return dispatch_ioctl_out(fd, 0x0B, handle, (uint32_t *) out_data, sizeof(FSDirectoryEntry));
}

int32_t FSA_CloseDir(int32_t fd, int32_t handle) {
    return dispatch_ioctl(fd, 0x0D, handle);
}

int32_t FSA_MakeDir(int32_t fd, char* path, uint32_t flags) {
    return dispatch_ioctl(fd, 0x07, path, flags);
}

int32_t FSA_OpenFile(int32_t fd, char* path, char* mode, int32_t* outHandle) {
    return FSA_OpenFileEx(fd, path, mode, 0, 0, 0, outHandle);
}

int32_t FSA_ReadFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags) {
    return _FSA_ReadWriteFileWithPos(fd, data, size, cnt, 0, fileHandle, flags, true);
}

int32_t FSA_WriteFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags) {
    return _FSA_ReadWriteFileWithPos(fd, data, size, cnt, 0, fileHandle, flags, false);
}

int32_t FSA_GetStatFile(int32_t fd, int32_t handle, FSStat* out_data) {
    return dispatch_ioctl_out(fd, 0x14, handle, (uint32_t *) out_data, sizeof(FSStat));
}

int32_t FSA_CloseFile(int32_t fd, int32_t fileHandle) {
    return dispatch_ioctl(fd, 0x15, fileHandle);
}

int32_t FSA_SetPosFile(int32_t fd, int32_t fileHandle, uint32_t position) {
    return dispatch_ioctl(fd, 0x12, fileHandle, position);
}

int32_t FSA_GetStat(int32_t fd, char *path, FSStat *out_data) {
    return FSA_GetInfo(fd, path, 5, (uint32_t *) out_data);
}

int32_t FSA_Remove(int32_t fd, char *path) {
    return dispatch_ioctl(fd, 0x08, path);
}

int32_t FSA_RewindDir(int32_t fd, int32_t handle) {
    return dispatch_ioctl(fd, 0x0C, handle);
}

int32_t FSA_ChangeDir(int32_t fd, char* path) {
    return dispatch_ioctl(fd, 0x05, path);
}

int32_t FSA_Rename(int32_t fd, char *old_path, char *new_path) {
	uint8_t* iobuf = allocIobuf();
	uint32_t* inbuf = (uint32_t*)iobuf;
	uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], old_path, 0x27F);
	strncpy((char*)&inbuf[0x284/4], new_path, 0x27F);

	int32_t ret = svcIoctl(fd, 0x09, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int32_t FSA_RawOpen(int32_t fd, char* device_path, int32_t* outHandle) {
    return dispatch_ioctl_out(fd, 0x6A, device_path, (uint32_t *) outHandle, sizeof(int32_t));
}

int32_t FSA_RawRead(int32_t fd, void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle) {
    return _FSA_RawReadWrite(fd, data, size_bytes, cnt, blocks_offset, device_handle, true);
}

int32_t FSA_RawWrite(int32_t fd, void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle) {
    return _FSA_RawReadWrite(fd, data, size_bytes, cnt, blocks_offset, device_handle, false);
}

int32_t FSA_RawClose(int32_t fd, int32_t device_handle) {
    return dispatch_ioctl(fd, 0x6D, device_handle);
}

int32_t FSA_ChangeMode(int32_t fd, char *path, int32_t mode) {
    return FSA_ChangeModeEx(fd, path, mode, 0x777);
}

int32_t FSA_FlushVolume(int32_t fd, char* volume_path) {
    return dispatch_ioctl(fd, 0x1B, volume_path);
}

int32_t FSA_ChangeOwner(int32_t fd, char *path, uint32_t owner, uint32_t group) {
    return dispatch_ioctl(fd, 0x70, path, 0, owner, 0, group);
}

// flags:
// - 1: open encrypted files, will fail unencrypted files
// - 2: preallocate size
int32_t FSA_OpenFileEx(int32_t fd, char* path, char* mode, uint32_t createMode, uint32_t flags, uint32_t preallocSize, int32_t* outHandle) {
    uint8_t* iobuf = allocIobuf();
    uint32_t* inbuf = (uint32_t*)iobuf;
    uint32_t* outbuf = (uint32_t*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);
	strncpy((char*)&inbuf[0x284/4], mode, 0x10);
	inbuf[0x294/4] = createMode;
	inbuf[0x298/4] = flags;
	inbuf[0x29C/4] = preallocSize;

	int32_t ret = svcIoctl(fd, 0x0E, inbuf, 0x520, outbuf, 0x293);

	if (outHandle) *outHandle = outbuf[1];

	freeIobuf(iobuf);
	return ret;
}

int32_t FSA_ReadFileWithPos(int32_t fd, void* data, uint32_t size, uint32_t cnt, uint32_t position, int32_t fileHandle, uint32_t flags) {
    return _FSA_ReadWriteFileWithPos(fd, data, size, cnt, 0, fileHandle, flags, true);
}

int32_t FSA_WriteFileWithPos(int32_t fd, void* data, uint32_t size, uint32_t cnt, uint32_t position, int32_t fileHandle, uint32_t flags) {
    return _FSA_ReadWriteFileWithPos(fd, data, size, cnt, 0, fileHandle, flags, false);
}

int32_t FSA_AppendFile(int32_t fd, uint32_t size, uint32_t cnt, int32_t fileHandle) {
    return FSA_AppendFileEx(fd, size, cnt, fileHandle, 0);
}

// flags:
// - 1: affects the way the blocks are allocated
// - 2: maybe will cause it to allocate it at the end of the quota?
int32_t FSA_AppendFileEx(int32_t fd, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags) {
    return dispatch_ioctl(fd, 0x19, fileHandle, size, cnt, flags);
}

int32_t FSA_FlushFile(int32_t fd, int32_t fileHandle) {
    return dispatch_ioctl(fd, 0x17, fileHandle);
}

int32_t FSA_TruncateFile(int32_t fd, int32_t fileHandle) {
    return dispatch_ioctl(fd, 0x1A, fileHandle);
}

int32_t FSA_GetPosFile(int32_t fd, int32_t fileHandle, uint32_t *out_position) {
    return dispatch_ioctl_out(fd, 0x11, fileHandle, (uint32_t *) out_position, sizeof(uint32_t));
}

int32_t FSA_IsEof(int32_t fd, int32_t fileHandle) {
    return dispatch_ioctl(fd, 0x13, fileHandle);
}

int32_t FSA_RollbackVolume(int32_t fd, char* volume_path) {
    return dispatch_ioctl(fd, 0x1C, volume_path);
}

int32_t FSA_GetCwd(int32_t fd, char *out_data, int32_t output_size) {
    uint8_t *iobuf   = allocIobuf();
    uint32_t *inbuf  = (uint32_t *) iobuf;
    uint32_t *outbuf = (uint32_t *) &iobuf[0x520];

    int32_t ret = svcIoctl(fd, 0x06, inbuf, 0x520, outbuf, 0x293);

    if (output_size > 0x27F) {
        output_size = 0x27F;
    }
    if (out_data) {
        strncpy(out_data, (char *) &outbuf[1], output_size);
    }

    freeIobuf(iobuf);
    return ret;
}

int32_t FSA_MakeQuota(int32_t fd, char *path, uint32_t flags, uint64_t size) {
    return dispatch_ioctl(fd, 0x1D, path, flags, (size >> 32), (size & 0xFFFFFFFF));
}

int32_t FSA_FlushQuota(int32_t fd, char *quota_path) {
    return dispatch_ioctl(fd, 0x1E, quota_path);
}

int32_t FSA_RollbackQuota(int32_t fd, char *quota_path) {
    return dispatch_ioctl(fd, 0x1F, quota_path, 0);
}

int32_t FSA_RollbackQuotaForce(int32_t fd, char *quota_path) {
    return dispatch_ioctl(fd, 0x1F, quota_path, 0x80000000);
}

int32_t FSA_ChangeModeEx(int32_t fd, char *path, int32_t mode, int32_t mask) {
    return dispatch_ioctl(fd, 0x20, path, mode, mask);
}

int32_t FSA_RegisterFlushQuota(int32_t fd, char *quota_path) {
    return dispatch_ioctl(fd, 0x22, quota_path);
}

int32_t FSA_FlushMultiQuota(int32_t fd, char *quota_path) {
    return dispatch_ioctl(fd, 0x23, quota_path);
}