#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
	uint32_t command;
	uint32_t result;
	uint32_t fd;
	uint32_t flags;
	uint32_t client_cpu;
	uint32_t client_pid;
	uint64_t client_gid;
	uint32_t server_handle;

	union {
	    uint32_t args[5];

		struct {
			char *device;
			uint32_t mode;
			uint32_t resultfd;
		} open;

		struct {
			void *data;
			uint32_t length;
		} read, write;

		struct {
			int32_t offset;
			int32_t origin;
		} seek;

		struct {
			uint32_t command;
			uint32_t *buffer_in;
			uint32_t length_in;
			uint32_t *buffer_io;
			uint32_t length_io;
		} ioctl;

		struct {
			uint32_t command;
			uint32_t num_in;
			uint32_t num_io;
			struct _ioctlv *vector;
		} ioctlv;
	};

	uint32_t prev_command;
	uint32_t prev_fd;
	uint32_t virt0;
	uint32_t virt1;
} IPCMessage;

typedef enum FSMode {
   FS_MODE_READ_OWNER                   = 0x400,
   FS_MODE_WRITE_OWNER                  = 0x200,
   FS_MODE_EXEC_OWNER                   = 0x100,

   FS_MODE_READ_GROUP                   = 0x040,
   FS_MODE_WRITE_GROUP                  = 0x020,
   FS_MODE_EXEC_GROUP                   = 0x010,

   FS_MODE_READ_OTHER                   = 0x004,
   FS_MODE_WRITE_OTHER                  = 0x002,
   FS_MODE_EXEC_OTHER                   = 0x001,
} FSMode;

typedef enum FSStatFlags {
   FS_STAT_DIRECTORY                = 0x80000000,
} FSStatFlags;

typedef struct __attribute__((__packed__)) {
   FSStatFlags flags;
   FSMode mode;
   uint32_t owner;
   uint32_t group;
   uint32_t size;
   uint32_t allocSize;
   uint64_t quotaSize;
   uint32_t entryId;
   int64_t created;
   int64_t modified;
   uint8_t unknown[0x30];
} FSStat;

typedef struct
{
   FSStat info;
   char name[256];
} FSDirectoryEntry;

typedef void (*usleep_t)(uint32_t);
typedef void* (*memcpy_t)(void*, const void*, int32_t);

static usleep_t usleep = (usleep_t)0x050564E4;
static memcpy_t memcpy = (memcpy_t)0x05054E54;