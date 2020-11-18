#include "fsa.h"
#include "svc.h"

static void* allocIobuf()
{
	void* ptr = svcAlloc(0xCAFF, 0x828);

	memset(ptr, 0x00, 0x828);

	return ptr;
}

static void freeIobuf(void* ptr)
{
	svcFree(0xCAFF, ptr);
}

int FSA_Mount(int fd, char* device_path, char* volume_path, u32 flags, char* arg_string, int arg_string_len)
{
	u8* iobuf = allocIobuf();
	u8* inbuf8 = iobuf;
	u8* outbuf8 = &iobuf[0x520];
	iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
	u32* inbuf = (u32*)inbuf8;
	u32* outbuf = (u32*)outbuf8;

	strncpy((char*)&inbuf8[0x04], device_path, 0x27F);
	strncpy((char*)&inbuf8[0x284], volume_path, 0x27F);
	inbuf[0x504 / 4] = (u32)flags;
	inbuf[0x508 / 4] = (u32)arg_string_len;

	iovec[0].ptr = inbuf;
	iovec[0].length = 0x520;
	iovec[1].ptr = arg_string;
	iovec[1].length = arg_string_len;
	iovec[2].ptr = outbuf;
	iovec[2].length = 0x293;

	int ret = svcIoctlv(fd, 0x01, 2, 1, iovec);

	freeIobuf(iobuf);
	return ret;
}

int FSA_Unmount(int fd, char* path, u32 flags)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);
	inbuf[0x284 / 4] = flags;

	int ret = svcIoctl(fd, 0x02, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_FlushVolume(int fd, char* volume_path)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], volume_path, 0x27F);

	int ret = svcIoctl(fd, 0x1B, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_MakeDir(int fd, char* path, u32 flags)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);
	inbuf[0x284 / 4] = flags;

	int ret = svcIoctl(fd, 0x07, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_OpenDir(int fd, char* path, int* outHandle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);

	int ret = svcIoctl(fd, 0x0A, inbuf, 0x520, outbuf, 0x293);

	if(outHandle) *outHandle = outbuf[1];

	freeIobuf(iobuf);
	return ret;
}

int FSA_ReadDir(int fd, int handle, FSDirectoryEntry* out_data)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = handle;

	int ret = svcIoctl(fd, 0x0B, inbuf, 0x520, outbuf, 0x293);

	if(out_data) memcpy(out_data, &outbuf[1], sizeof(FSDirectoryEntry));

	freeIobuf(iobuf);
	return ret;
}

int FSA_RewindDir(int fd, int handle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = handle;

	int ret = svcIoctl(fd, 0x0C, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_CloseDir(int fd, int handle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = handle;

	int ret = svcIoctl(fd, 0x0D, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_ChangeDir(int fd, char* path)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);

	int ret = svcIoctl(fd, 0x05, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_OpenFile(int fd, char* path, char* mode, int* outHandle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);
	strncpy((char*)&inbuf[0xA1], mode, 0x10);

	int ret = svcIoctl(fd, 0x0E, inbuf, 0x520, outbuf, 0x293);

	if(outHandle) *outHandle = outbuf[1];

	freeIobuf(iobuf);
	return ret;
}

int _FSA_ReadWriteFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags, bool read)
{
	u8* iobuf = allocIobuf();
	u8* inbuf8 = iobuf;
	u8* outbuf8 = &iobuf[0x520];
	iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
	u32* inbuf = (u32*)inbuf8;
	u32* outbuf = (u32*)outbuf8;

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

	int ret;
	if(read) ret = svcIoctlv(fd, 0x0F, 1, 2, iovec);
	else ret = svcIoctlv(fd, 0x10, 2, 1, iovec);

	freeIobuf(iobuf);
	return ret;
}

int FSA_ReadFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags)
{
	return _FSA_ReadWriteFile(fd, data, size, cnt, fileHandle, flags, true);
}

int FSA_WriteFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags)
{
	return _FSA_ReadWriteFile(fd, data, size, cnt, fileHandle, flags, false);
}

int FSA_StatFile(int fd, int handle, FSStat* out_data)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = handle;

	int ret = svcIoctl(fd, 0x14, inbuf, 0x520, outbuf, 0x293);

	if(out_data) memcpy(out_data, &outbuf[1], sizeof(FSStat));

	freeIobuf(iobuf);
	return ret;
}

int FSA_CloseFile(int fd, int fileHandle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = fileHandle;

	int ret = svcIoctl(fd, 0x15, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_SetPosFile(int fd, int fileHandle, u32 position)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = fileHandle;
	inbuf[2] = position;

	int ret = svcIoctl(fd, 0x12, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_GetStat(int fd, char *path, FSStat* out_data)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);
	inbuf[0x284/4] = 5;

	int ret = svcIoctl(fd, 0x18, inbuf, 0x520, outbuf, 0x293);

	if(out_data) memcpy(out_data, &outbuf[1], sizeof(FSStat));

	freeIobuf(iobuf);
	return ret;
}

int FSA_Remove(int fd, char *path)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);

	int ret = svcIoctl(fd, 0x08, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_ChangeMode(int fd, char *path, int mode)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], path, 0x27F);
	inbuf[0x284/4] = mode;
	inbuf[0x288/4] = 0x777; // mask

	int ret = svcIoctl(fd, 0x20, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

// type 4 :
// 		0x08 : device size in sectors (u64)
// 		0x10 : device sector size (u32)
int FSA_GetDeviceInfo(int fd, char* device_path, int type, u32* out_data)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], device_path, 0x27F);
	inbuf[0x284 / 4] = type;

	int ret = svcIoctl(fd, 0x18, inbuf, 0x520, outbuf, 0x293);

	int size = 0;

	switch(type)
	{
		case 0: case 1: case 7:
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
		case 6: case 8:
			size = 0x14;
			break;
	}

	memcpy(out_data, &outbuf[1], size);

	freeIobuf(iobuf);
	return ret;
}

int FSA_RawOpen(int fd, char* device_path, int* outHandle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	strncpy((char*)&inbuf[0x01], device_path, 0x27F);

	int ret = svcIoctl(fd, 0x6A, inbuf, 0x520, outbuf, 0x293);

	if(outHandle) *outHandle = outbuf[1];

	freeIobuf(iobuf);
	return ret;
}

int FSA_RawClose(int fd, int device_handle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = device_handle;

	int ret = svcIoctl(fd, 0x6D, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

// offset in blocks of 0x1000 bytes
int FSA_RawRead(int fd, void* data, u32 size_bytes, u32 cnt, u64 blocks_offset, int device_handle)
{
	u8* iobuf = allocIobuf();
	u8* inbuf8 = iobuf;
	u8* outbuf8 = &iobuf[0x520];
	iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
	u32* inbuf = (u32*)inbuf8;
	u32* outbuf = (u32*)outbuf8;

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

	int ret = svcIoctlv(fd, 0x6B, 1, 2, iovec);

	freeIobuf(iobuf);
	return ret;
}

int FSA_RawWrite(int fd, void* data, u32 size_bytes, u32 cnt, u64 blocks_offset, int device_handle)
{
	u8* iobuf = allocIobuf();
	u8* inbuf8 = iobuf;
	u8* outbuf8 = &iobuf[0x520];
	iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
	u32* inbuf = (u32*)inbuf8;
	u32* outbuf = (u32*)outbuf8;

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

	int ret = svcIoctlv(fd, 0x6C, 2, 1, iovec);

	freeIobuf(iobuf);
	return ret;
}