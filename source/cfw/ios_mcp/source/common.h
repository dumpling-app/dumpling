#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "../../shared.h"

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

typedef struct {
   uint32_t flags;
   uint32_t mode;
   uint32_t owner;
   uint32_t group;
   uint32_t size;
   uint32_t allocSize;
   uint64_t quotaSize;
   uint32_t entryId;
   uint64_t created;
   uint64_t modified;
   uint8_t attributes[48];
} __attribute__((packed)) FSStat;
static_assert(sizeof(FSStat) == 0x64, "FSStat struct is not 0x64 bytes!");

typedef struct {
   FSStat info;
   char name[256];
} FSDirectoryEntry;
static_assert(sizeof(FSDirectoryEntry) == 0x164, "FSDirectoryEntry struct is not 0x164 bytes!");

typedef struct {
    uint8_t unknown[0x1E];
} FSFileSystemInfo;
static_assert(sizeof(FSFileSystemInfo) == 0x1E, "FSFileSystemInfo struct is not 0x1E bytes!");

typedef struct {
    uint8_t unknown1[0x8];
    uint64_t deviceSizeInSectors;
    uint32_t deviceSectorSize;
    uint8_t unknown2[0x14];
} __attribute__((packed)) FSDeviceInfo;
static_assert(sizeof(FSDeviceInfo) == 0x28, "FSDeviceInfo struct is not 0x28 bytes!");

typedef struct {
    uint64_t blocks_count;
    uint64_t some_count;
    uint32_t block_size;
} FSBlockInfo;