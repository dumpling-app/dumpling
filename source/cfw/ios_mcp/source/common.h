#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "../../shared.h"

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
} __attribute__((aligned(4))) __attribute__((packed)) FSStat;
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
} __attribute__((packed)) FSBlockInfo;
static_assert(sizeof(FSBlockInfo) == 0x14, "FSBlockInfo struct is not 0x14 bytes!");

#define IPC_CUSTOM_START_MCP_THREAD       0xFE
#define IPC_CUSTOM_MEN_RPX_HOOK_COMPLETED 0xFD
#define IPC_CUSTOM_LOAD_CUSTOM_RPX        0xFC
#define IPC_CUSTOM_START_USB_LOGGING      0xFA
#define IPC_CUSTOM_COPY_ENVIRONMENT_PATH  0xF9
#define IPC_CUSTOM_GET_MOCHA_API_VERSION  0xF8

#define MOCHA_API_VERSION 1 + 1337 /* Special Dumpling Identifier */