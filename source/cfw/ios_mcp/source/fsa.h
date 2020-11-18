#include "common.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

int FSA_Open();

int FSA_Mount(int fd, char* device_path, char* volume_path, u32 flags, char* arg_string, int arg_string_len);
int FSA_Unmount(int fd, char* path, u32 flags);
int FSA_FlushVolume(int fd, char* volume_path);

int FSA_GetDeviceInfo(int fd, char* device_path, int type, u32* out_data);

int FSA_MakeDir(int fd, char* path, u32 flags);
int FSA_OpenDir(int fd, char* path, int* outHandle);
int FSA_ReadDir(int fd, int handle, FSDirectoryEntry* out_data);
int FSA_RewindDir(int fd, int handle);
int FSA_CloseDir(int fd, int handle);
int FSA_ChangeDir(int fd, char* path);

int FSA_OpenFile(int fd, char* path, char* mode, int* outHandle);
int FSA_ReadFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags);
int FSA_WriteFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags);
int FSA_StatFile(int fd, int handle, FSStat* out_data);
int FSA_CloseFile(int fd, int fileHandle);
int FSA_SetPosFile(int fd, int fileHandle, u32 position);
int FSA_GetStat(int fd, char *path, FSStat* out_data);
int FSA_Remove(int fd, char *path);
int FSA_ChangeMode(int fd, char *path, int mode);

int FSA_RawOpen(int fd, char* device_path, int* outHandle);
int FSA_RawRead(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
int FSA_RawWrite(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
int FSA_RawClose(int fd, int device_handle);