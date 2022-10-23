/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing       */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "ffcache.h"


/*-----------------------------------------------------------------------*/
/* Internal Wii U Functions                                              */
/*-----------------------------------------------------------------------*/

#if USE_RAMDISK == 0

#include <coreinit/filesystem.h>
#include <coreinit/debug.h>
#include <stdlib.h>
#include <mocha/mocha.h>
#include <mocha/fsa.h>
#include <coreinit/filesystem_fsa.h>

// Some state has to be kept for the mounting of the devices. The cleanup is done by fat32.cpp.
const char* fatDevPaths[FF_VOLUMES] = {"/dev/sdcard01", "/dev/usb01", "/dev/usb02"};
bool fatMounted[FF_VOLUMES] = {false, false, false};
FSAClientHandle fatClients[FF_VOLUMES] = {};
IOSHandle fatHandles[FF_VOLUMES] = {-1, -1, -1};
const WORD fatSectorSizes[FF_VOLUMES] = {512, 512, 512};
WORD fatCacheSizes[FF_VOLUMES] = {32*8*4*4, 32*8*4*8, 32*8*4};


DSTATUS wiiu_mountDrive(BYTE pdrv) {
    // Create and open raw device
    fatClients[pdrv] = FSAAddClient(NULL);
    Mocha_UnlockFSClientEx(fatClients[pdrv]);

    FSError res = FSAEx_RawOpenEx(fatClients[pdrv], fatDevPaths[pdrv], &fatHandles[pdrv]);
    if (res < 0) {
        OSReport("Couldn't open %d with error %d\n", pdrv, res);
        FSADelClient(fatClients[pdrv]);
        return STA_NODISK;
    }
    OSReport("Mounted %d drive\n", pdrv);
    fatMounted[pdrv] = true;
    return 0;
}

DSTATUS wiiu_unmountDrive(BYTE pdrv) {
    FSAEx_RawCloseEx(fatClients[pdrv], fatHandles[pdrv]);
    FSADelClient(fatClients[pdrv]);
    fatMounted[pdrv] = false;
    return 0;
}


FSError wiiu_readSectors(BYTE pdrv, LBA_t sectorIdx, UINT sectorCount, BYTE* outputBuff) {
    FSError status = FSAEx_RawReadEx(fatClients[pdrv], outputBuff, fatSectorSizes[pdrv], sectorCount, sectorIdx, fatHandles[pdrv]);
    //OSReport("[CacheRead] buff=%x, idx=%u, cnt=%u; ret=%d\n", (void*)outputBuff, sectorIdx, sectorCount, status);
    return status;
}

FSError wiiu_writeSectors(BYTE pdrv, LBA_t sectorIdx, UINT sectorCount, const BYTE* inputBuff) {
    FSError status = FSAEx_RawWriteEx(fatClients[pdrv], inputBuff, fatSectorSizes[pdrv], sectorCount, sectorIdx, fatHandles[pdrv]);
    //OSReport("[CacheWrite] buff=%x, idx=%u, cnt=%u; ret=%d\n", (void*)inputBuff, sectorIdx, sectorCount, status);
    return status;
}


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES)
        return STA_NOINIT;
    if (!fatMounted[pdrv]) {
        return STA_NOINIT;
    }
    uint8_t* headerBuff = aligned_alloc(0x40, fatSectorSizes[pdrv]);
    FSError status = FSAEx_RawReadEx(fatClients[pdrv], headerBuff, fatSectorSizes[pdrv], 1, 0, fatHandles[pdrv]);
    free(headerBuff);

    if (status != FS_ERROR_OK) OSReport("Non-zero status while getting disk status %d\n", status);
    if (status == FS_ERROR_WRITE_PROTECTED) return STA_PROTECT;
    if (status == FS_ERROR_MEDIA_NOT_READY) return STA_NODISK;
	if (status != FS_ERROR_OK) return STA_NOINIT;
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return STA_NOINIT;
    if (fatMounted[pdrv]) return STA_NOINIT;
    // todo: Support drives with non-512 sector sizes
    ffcache_initialize(pdrv, fatSectorSizes[pdrv], fatCacheSizes[pdrv]);
    return wiiu_mountDrive(pdrv);
}

/*-----------------------------------------------------------------------*/
/* Shutdown a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_shutdown (
        BYTE pdrv				/* Physical drive number to identify the drive */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return STA_NOINIT;
    ffcache_shutdown(pdrv);
    if (!fatMounted[pdrv]) return STA_NOINIT;
    return wiiu_unmountDrive(pdrv);
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return RES_PARERR;
    if (!fatMounted[pdrv]) return RES_NOTRDY;

    //return wiiu_readSectors(pdrv, sector, count, buff) == FS_ERROR_OK ? RES_OK : RES_ERROR;
    return ffcache_readSectors(pdrv, sector, count, buff);
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return RES_PARERR;
    if (!fatMounted[pdrv]) return RES_NOTRDY;

    //return wiiu_writeSectors(pdrv, sector, count, buff) == FS_ERROR_OK ? RES_OK : RES_ERROR;
    return ffcache_writeSectors(pdrv, sector, count, buff);
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return RES_ERROR;
    if (!fatMounted[pdrv]) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC: {
            DEBUG_OSReport("[disk_ioctl] Requested a sync, flushing cached sectors!");
            //return ffcache_flushSectors(pdrv);
            return RES_OK;
        }
        case CTRL_FORCE_SYNC: {
            DEBUG_OSReport("[disk_ioctl] Requested a forced sync, flushing cached sectors!");
            //return ffcache_flushSectors(pdrv);
            return RES_OK;
        }
        case SET_CACHE_COUNT: {
            DEBUG_OSReport("[disk_ioctl] Requested changing the cache size to %d", *((WORD*)buff));
            fatCacheSizes[pdrv] = *((WORD*)buff);
            return RES_OK;
        }
        case GET_SECTOR_COUNT: {
            DEBUG_OSReport("[disk_ioctl] Requested sector count!");
//            FSADeviceInfo deviceInfo = {};
//            if (FSAGetDeviceInfo(fatClients[pdrv], fatDevPaths[pdrv], &deviceInfo) != FS_ERROR_OK) return RES_ERROR;
//            *(LBA_t*)buff = deviceInfo.deviceSizeInSectors;
            return RES_OK;
        }
        case GET_SECTOR_SIZE: {
            DEBUG_OSReport("[disk_ioctl] Requested sector size which is currently %d!", fatSectorSizes[pdrv]);
            *(WORD*)buff = (WORD)fatSectorSizes[pdrv];
            return RES_OK;
        }
        case GET_BLOCK_SIZE: {
            DEBUG_OSReport("[disk_ioctl] Requested block size which is unknown!");
            *(WORD*)buff = 1;
            return RES_OK;
        }
        case CTRL_TRIM: {
            DEBUG_OSReport("[disk_ioctl] Requested trim!");
            return RES_OK;
        }
        default:
            return RES_PARERR;
    }

	return RES_PARERR;
}

DWORD get_fattime() {
    OSCalendarTime output;
    OSTicksToCalendarTime(OSGetTime(), &output);
    return (DWORD) (output.tm_year - 1980) << 25 |
           (DWORD) (output.tm_mon + 1) << 21 |
           (DWORD) output.tm_mday << 16 |
           (DWORD) output.tm_hour << 11 |
           (DWORD) output.tm_min << 5 |
           (DWORD) output.tm_sec >> 1;
}

#else


#include <coreinit/filesystem.h>
#include <coreinit/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Some state has to be kept for the mounting of the devices. The cleanup is done by fat32.cpp.
bool fatMounted[FF_VOLUMES] = {false};

#define SPLIT_TOTAL_SIZE 10737418240
#define SPLIT_TOTAL_SIZE_SECTORS (SPLIT_TOTAL_SIZE/512)
#define SPLIT_IMAGE_COUNT 5
#define SPLIT_IMAGE_SIZE_SECTORS (SPLIT_TOTAL_SIZE/SPLIT_IMAGE_COUNT/512)

FSFileHandle fatHandles[FF_VOLUMES][SPLIT_IMAGE_COUNT];
const WORD fatSectorSizes[FF_VOLUMES] = {512};
WORD fatCacheSizes[FF_VOLUMES] = {32*8*4};

FSClient* client;
FSCmdBlock fsCmd;

char sMountPath[0x80];

DSTATUS wiiu_mountDrive(BYTE pdrv) {
    client = aligned_alloc(0x20, sizeof(FSClient));
    FSAddClient(client, 0);
    FSInitCmdBlock(&fsCmd);

    FSMountSource mountSource;
    FSGetMountSource(client, &fsCmd, FS_MOUNT_SOURCE_SD, &mountSource, -1);

    FSMount(client, &fsCmd, &mountSource, sMountPath, sizeof(sMountPath), FS_ERROR_FLAG_ALL);

    // Check raw disk image
    for (uint8_t i=0; i<SPLIT_IMAGE_COUNT; i++) {
        char name[255];
        snprintf(name, 254, "%s/split%u.img", sMountPath, i);
        FSStatus result = FSOpenFile(client, &fsCmd, name, "r+", &fatHandles[pdrv][i], FS_ERROR_FLAG_ALL);
        OSReport("Opened disk image %s with size of with result %d!\n", name, result);
        if (result != FS_STATUS_OK) {
            FSDelClient(client, 0);
            return STA_NOINIT;
        }
    }
    fatMounted[pdrv] = true;
    return 0;
}

DSTATUS wiiu_unmountDrive(BYTE pdrv) {
    for (uint8_t i=0; i<SPLIT_IMAGE_COUNT; i++) {
        FSCloseFile(client, &fsCmd, fatHandles[pdrv][i], FS_ERROR_FLAG_ALL);
    }
    FSDelClient(client, 0);
    free(client);

    fatMounted[pdrv] = false;
    return 0;
}

FSError wiiu_readSectors(BYTE pdrv, LBA_t sectorIdx, UINT sectorCount, BYTE* outputBuff) {
    void* tempBuff = aligned_alloc(0x40, 1*fatSectorSizes[pdrv]);
    for (uint32_t i=0; i<sectorCount; i++) {
        LBA_t currSectorIdx = sectorIdx + i;
        uint32_t fileIdx = 0;
        if (currSectorIdx != 0) {
            fileIdx = currSectorIdx / SPLIT_IMAGE_SIZE_SECTORS;
            currSectorIdx = currSectorIdx % SPLIT_IMAGE_SIZE_SECTORS;
        }

        FSStatus status = FSReadFileWithPos(client, &fsCmd, tempBuff, 1*fatSectorSizes[pdrv], 1, currSectorIdx*fatSectorSizes[pdrv], fatHandles[pdrv][fileIdx], 0, FS_ERROR_FLAG_ALL);
        memcpy(outputBuff+(i*fatSectorSizes[pdrv]), tempBuff, 1*fatSectorSizes[pdrv]);
        //DEBUG_OSReport("[CacheRead] buff=%x, idx=%u -> relativeIdx=%u, cnt=%u; ret=%d", (void*)outputBuff, sectorIdx, currSectorIdx, sectorCount, status);
    }
    free(tempBuff);
    return FS_ERROR_OK;
}

FSError wiiu_writeSectors(BYTE pdrv, LBA_t sectorIdx, UINT sectorCount, const BYTE* inputBuff) {
    void* tempBuff = aligned_alloc(0x40, 1*fatSectorSizes[pdrv]);
    for (uint32_t i=0; i<sectorCount; i++) {
        LBA_t currSectorIdx = sectorIdx + i;
        uint32_t fileIdx = 0;
        if (currSectorIdx != 0) {
            fileIdx = currSectorIdx / SPLIT_IMAGE_SIZE_SECTORS;
            currSectorIdx = currSectorIdx % SPLIT_IMAGE_SIZE_SECTORS;
        }

        memcpy(tempBuff, ((void*)inputBuff)+(i*fatSectorSizes[pdrv]), 1*fatSectorSizes[pdrv]);
        FSStatus status = FSWriteFileWithPos(client, &fsCmd, tempBuff, 1*fatSectorSizes[pdrv], 1, currSectorIdx*fatSectorSizes[pdrv], fatHandles[pdrv][fileIdx], 0, FS_ERROR_FLAG_ALL);
        //DEBUG_OSReport("[CacheWrite] buff=%x, idx=%u -> relativeIdx=%u, cnt=%u; ret=%d", (void*)inputBuff, sectorIdx, currSectorIdx, sectorCount, status);
    }
    free(tempBuff);
    return FS_ERROR_OK;
}


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
        BYTE pdrv		/* Physical drive number to identify the drive */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES)
        return STA_NOINIT;
    if (!fatMounted[pdrv]) {
        return STA_NOINIT;
    }
//    uint8_t* headerBuff = aligned_alloc(0x40, fatSectorSizes[pdrv]);
//    FSError status = FSAEx_RawReadEx(fatClients[pdrv], headerBuff, fatSectorSizes[pdrv], 1, 0, fatHandles[pdrv]);
//    free(headerBuff);
//    if (status != FS_ERROR_OK) OSReport("Non-zero status while getting disk status %d\n", status);
//    if (status == FS_ERROR_WRITE_PROTECTED) return STA_PROTECT;
//    if (status == FS_ERROR_MEDIA_NOT_READY) return STA_NODISK;
//    if (status != FS_ERROR_OK) return STA_NOINIT;
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
        BYTE pdrv				/* Physical drive number to identify the drive */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES)
        return STA_NOINIT;
    if (fatMounted[pdrv])
        return STA_NOINIT;
    // todo: Get sector size instead of hard-coding it
    ffcache_initialize(pdrv, fatSectorSizes[pdrv], fatCacheSizes[pdrv]);
    return wiiu_mountDrive(pdrv);
}

/*-----------------------------------------------------------------------*/
/* Shutdown a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_shutdown (
        BYTE pdrv				/* Physical drive number to identify the drive */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES)
        return STA_NOINIT;
    ffcache_shutdown(pdrv);
    if (!fatMounted[pdrv])
        return STA_NOINIT;
    return wiiu_unmountDrive(pdrv);
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
        BYTE pdrv,		/* Physical drive number to identify the drive */
        BYTE *buff,		/* Data buffer to store read data */
        LBA_t sector,	/* Start sector in LBA */
        UINT count		/* Number of sectors to read */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return RES_PARERR;
    if (!fatMounted[pdrv]) return RES_NOTRDY;

    return wiiu_readSectors(pdrv, sector, count, buff) == FS_ERROR_OK ? RES_OK : RES_ERROR;
    //return ffcache_readSectors(pdrv, sector, count, buff);
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
        BYTE pdrv,			/* Physical drive number to identify the drive */
        const BYTE *buff,	/* Data to be written */
        LBA_t sector,		/* Start sector in LBA */
        UINT count			/* Number of sectors to write */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return RES_PARERR;
    if (!fatMounted[pdrv]) return RES_NOTRDY;

    return wiiu_writeSectors(pdrv, sector, count, buff) == FS_ERROR_OK ? RES_OK : RES_ERROR;
    //return ffcache_writeSectors(pdrv, sector, count, buff);
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
        BYTE pdrv,		/* Physical drive number (0..) */
        BYTE cmd,		/* Control code */
        void *buff		/* Buffer to send/receive control data */
)
{
    if (pdrv < 0 || pdrv >= FF_VOLUMES) return RES_ERROR;
    if (!fatMounted[pdrv]) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC: {
            DEBUG_OSReport("[disk_ioctl] Requested a sync, flushing currently cached sectors!");
            for (uint8_t i=0; i<SPLIT_IMAGE_COUNT; i++) {
                FSFlushFile(client, &fsCmd, fatHandles[pdrv][i], FS_ERROR_FLAG_ALL);
            }
            return RES_OK;
        }
        case CTRL_FORCE_SYNC: {
            DEBUG_OSReport("[disk_ioctl] Requested a forced sync, flushing currently cached sectors!");
            for (uint8_t i=0; i<SPLIT_IMAGE_COUNT; i++) {
                FSFlushFile(client, &fsCmd, fatHandles[pdrv][i], FS_ERROR_FLAG_ALL);
            }
            return RES_OK;
        }
        case SET_CACHE_COUNT: {
            DEBUG_OSReport("[disk_ioctl] Requested changing the cache size to %d", *((WORD*)buff));
            fatCacheSizes[pdrv] = *((WORD*)buff);
            return RES_OK;
        }
        case GET_SECTOR_COUNT: {
            DEBUG_OSReport("[disk_ioctl] Requested sector count!");
            *(LBA_t*)buff = SPLIT_TOTAL_SIZE_SECTORS;
            return RES_OK;
        }
        case GET_SECTOR_SIZE: {
            DEBUG_OSReport("[disk_ioctl] Requested sector size which is currently %d!", fatSectorSizes[pdrv]);
            *(WORD*)buff = (WORD)fatSectorSizes[pdrv];
            return RES_OK;
        }
        case GET_BLOCK_SIZE: {
            DEBUG_OSReport("[disk_ioctl] Requested block size which is unknown!");
            *(WORD*)buff = 1;
            return RES_OK;
        }
        case CTRL_TRIM: {
            DEBUG_OSReport("[disk_ioctl] Requested trim!");
            //return RES_OK;
        }
        default:
            return RES_PARERR;
    }

    return RES_PARERR;
}

DWORD get_fattime() {
    OSCalendarTime output;
    OSTicksToCalendarTime(OSGetTime(), &output);
    return (DWORD) (output.tm_year - 1980) << 25 |
           (DWORD) (output.tm_mon + 1) << 21 |
           (DWORD) output.tm_mday << 16 |
           (DWORD) output.tm_hour << 11 |
           (DWORD) output.tm_min << 5 |
           (DWORD) output.tm_sec >> 1;
}

#endif