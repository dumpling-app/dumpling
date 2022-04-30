#include "common.h"

int32_t FSA_Mount(int32_t fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int32_t arg_string_len);
int32_t FSA_Unmount(int32_t fd, char* path, uint32_t flags);
int32_t FSA_FlushVolume(int32_t fd, char* volume_path);
int32_t FSA_RollbackVolume(int32_t fd, char* volume_path);

int32_t FSA_GetInfo(int32_t fd, char* device_path, int32_t type, uint32_t* out_data);
int32_t FSA_GetStat(int32_t fd, char* path, FSStat* out_data);

int32_t FSA_MakeDir(int32_t fd, char* path, uint32_t flags);
int32_t FSA_OpenDir(int32_t fd, char* path, int32_t* outHandle);
int32_t FSA_ReadDir(int32_t fd, int32_t handle, FSDirectoryEntry* out_data);
int32_t FSA_RewindDir(int32_t fd, int32_t handle);
int32_t FSA_CloseDir(int32_t fd, int32_t handle);
int32_t FSA_ChangeDir(int32_t fd, char* path);
int32_t FSA_GetCwd(int32_t fd, char* out_data, int32_t output_size);

int32_t FSA_MakeQuota(int32_t fd, char* path, uint32_t flags, uint64_t size);
int32_t FSA_FlushQuota(int32_t fd, char* quota_path);
int32_t FSA_RollbackQuota(int32_t fd, char* quota_path);
int32_t FSA_RollbackQuotaForce(int32_t fd, char* quota_path);

int32_t FSA_OpenFile(int32_t fd, char* path, char* mode, int32_t* outHandle);
int32_t FSA_OpenFileEx(int32_t fd, char* path, char* mode, int32_t* outHandle, uint32_t createMode, uint32_t flags, uint32_t preallocSize);
int32_t FSA_ReadFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags);
int32_t FSA_WriteFile(int32_t fd, void* data, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags);
int32_t FSA_ReadFileWithPos(int32_t fd, void* data, uint32_t size, uint32_t cnt, uint32_t position, int32_t fileHandle, uint32_t flags);
int32_t FSA_WriteFileWithPos(int32_t fd, void* data, uint32_t size, uint32_t cnt, uint32_t position, int32_t fileHandle, uint32_t flags);
int32_t FSA_AppendFile(int32_t fd, uint32_t size, uint32_t cnt, int32_t fileHandle);
int32_t FSA_AppendFileEx(int32_t fd, uint32_t size, uint32_t cnt, int32_t fileHandle, uint32_t flags);
int32_t FSA_CloseFile(int32_t fd, int32_t fileHandle);

int32_t FSA_GetStatFile(int32_t fd, int32_t handle, FSStat* out_data);
int32_t FSA_FlushFile(int32_t fd, int32_t handle);
int32_t FSA_TruncateFile(int32_t fd, int32_t handle);
int32_t FSA_GetPosFile(int32_t fd, int32_t fileHandle, uint32_t* out_position);
int32_t FSA_SetPosFile(int32_t fd, int32_t fileHandle, uint32_t position);
int32_t FSA_IsEof(int32_t fd, int32_t fileHandle);

int32_t FSA_Remove(int32_t fd, char* path);
int32_t FSA_Rename(int32_t fd, char* oldPath, char* newPath);
int32_t FSA_ChangeMode(int32_t fd, char* path, int32_t mode);
int32_t FSA_ChangeModeEx(int32_t fd, char* path, int32_t mode, int32_t mask);
int32_t FSA_ChangeOwner(int32_t fd, char* path, uint32_t owner, uint32_t group);

int32_t FSA_RawOpen(int32_t fd, char* device_path, int32_t* outHandle);
int32_t FSA_RawRead(int32_t fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t sector_offset, int32_t device_handle);
int32_t FSA_RawWrite(int32_t fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int32_t device_handle);
int32_t FSA_RawClose(int32_t fd, int32_t device_handle);

void* memset(void* dest, int32_t value, int32_t size);
char* strncpy(char* dest, const char* src, int32_t size);