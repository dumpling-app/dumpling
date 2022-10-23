#ifndef _FFCACHE_DEFINED
#define _FFCACHE_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "diskio.h"

DRESULT ffcache_initialize(BYTE pdrv, DWORD sectorByteSize, DWORD sectorCount);
void ffcache_shutdown(BYTE pdrv);

DRESULT ffcache_readSectors(BYTE pdrv, LBA_t sectorOffset, UINT sectorCount, BYTE *bufferOut);
DRESULT ffcache_writeSectors(BYTE pdrv, LBA_t sectorOffset, UINT sectorCount, const BYTE *bufferIn);
DRESULT ffcache_flushSectors(BYTE pdrv);
uint64_t cache_getTime();
BYTE* ffcache_getSector(BYTE pdrv, LBA_t sectorOffset);

typedef struct dirCache {
    DWORD	cluster; // Which cluster this directory is located at
    LBA_t	sectorOffset;
    DWORD	dirIdx; // Offset from cluster start for this specific dir entry (cluster being a chain of file entries)
    DWORD	lfnStartDirIdx;
} dirCache;


dirCache* dirCache_createSFN(BYTE pdrv, DIR* cache);
dirCache* dirCache_findSFN(BYTE pdrv, BYTE* sfnWithoutStatus);

dirCache* dirCache_createLFN(BYTE pdrv, const WCHAR* fullName, DIR* cache);
dirCache* dirCache_findLFN(BYTE pdrv, const WCHAR* fullName);

void dirCache_setLastAllocatedIdx(BYTE pdrv, DWORD lastIdx, DWORD lastDirTable);
DWORD dirCache_getLastClusterIdx(BYTE pdrv, DWORD currDirTableIdx);
void dirCache_clear(BYTE pdrv);


#if USE_DEBUG_STUBS
    #define DEBUG_OSReport(fmt, args...) OSReport(fmt "\n", ## args)
#else
    #define DEBUG_OSReport(fmt, args...) do {} while (false)
#endif

uint64_t profile_startSegment();
void profile_incrementCounter(const char* segmentName);
void profile_endSegment(const char* segmentName, uint64_t startTime);
#if USE_DEBUG_STUBS
    #define DEBUG_profile_startSegment(sectionName) uint64_t sectionName##Time = profile_startSegment();
    #define DEBUG_profile_endSegment(sectionName) profile_endSegment( #sectionName "Time" , sectionName##Time );
    #define DEBUG_profile_incrementCounter(counterName) profile_incrementCounter( #counterName );
#else
    #define DEBUG_profile_startSegment(sectionName) do {} while (false)
    #define DEBUG_profile_endSegment(sectionName) do {} while (false)
    #define DEBUG_profile_incrementCounter(counterName) do {} while (false)
#endif


#ifdef __cplusplus
}
#endif

#endif /* _FFCACHE_DEFINED */