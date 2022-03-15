#include "common.h"

int32_t FSA_Open();

int32_t FSA_Mount(int32_t fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int32_t arg_string_len);
int32_t FSA_Unmount(int32_t fd, char* path, uint32_t flags);
int32_t FSA_FlushVolume(int32_t fd, char* volume_path);

int32_t FSA_GetDeviceInfo(int32_t fd, char* device_path, int32_t type, uint32_t* out_data);

int32_t FSA_MakeDir(int32_t fd, char* path, uint32_t flags);
int32_t FSA_OpenDir(int32_t fd, char* path, int32_t* outHandle);
int32_t FSA_ReadDir(int32_t fd, int32_t handle, FSDirectoryEntry* out_data);
int32_t FSA_RewindDir(int32_t fd, int32_t handle);
int32_t FSA_CloseDir(int32_t fd, int32_t handle);
int32_t FSA_ChangeDir(int32_t fd, char* path);

int32_t FSA_OpenFile(int32_t fd, char* path, char* mode, int32_t* outHandle);
int32_t FSA_ReadFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags);
int32_t FSA_WriteFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags);
int32_t FSA_StatFile(int32_t fd, int32_t handle, FSStat* out_data);
int32_t FSA_CloseFile(int32_t fd, int32_t fileHandle);
int32_t FSA_SetPosFile(int32_t fd, int32_t fileHandle, uint32_t position);
int32_t FSA_GetStat(int32_t fd, char *path, FSStat* out_data);
int32_t FSA_Remove(int32_t fd, char *path);
int32_t FSA_ChangeMode(int32_t fd, char *path, int32_t mode);

int32_t FSA_RawOpen(int32_t fd, char* device_path, int32_t* outHandle);
int32_t FSA_RawRead(int32_t fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t sector_offset, int32_t device_handle);
int32_t FSA_RawWrite(int32_t fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle);
int32_t FSA_RawClose(int32_t fd, int32_t device_handle);

void* memset(void* dest, int32_t value, int32_t size);
char* strncpy(char* dest, const char* src, int32_t size);