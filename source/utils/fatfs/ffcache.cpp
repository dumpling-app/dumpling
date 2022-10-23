#include "ffcache.h"

#include <mocha/fsa.h>
#include <memory.h>
#include <coreinit/debug.h>
#include <map>
#include <list>
#include <vector>
#include <algorithm>
#include <limits>

#include <chrono>
#include <mutex>

#include "./../log_freetype.h"

struct sectorCache {
    LBA_t sectorIdx = std::numeric_limits<LBA_t>::max();
    bool dirty = false;
    BYTE* buffer = nullptr;
    std::list<sectorCache*>::iterator it;
};

sectorCache emptySector = {};

struct driveCache {
    bool initialized = false;
    BYTE pdrv = 0;
    uint32_t sectorSize = 0;
    uint32_t sectorCount = 0;
    BYTE* sectorsBuffer = nullptr;

    std::vector<sectorCache*> freeSectors;
    std::list<sectorCache*> lruSectors;
    std::unordered_map<LBA_t, sectorCache*> cachedSectors;

    std::unordered_map<std::string, dirCache> cachedDirSFNs;
    std::unordered_map<std::u16string, dirCache> cachedDirLFNs;

    DWORD lastDirTable = 0xFFFFFFFF;
    DWORD cachedDirLastAllocatedIdx = 0;

    sectorCache* activeCacheWindow = &emptySector;
};

std::array<driveCache, FF_VOLUMES> fatCaches;

static void cache_addLRUEntry(driveCache* cache, sectorCache *ptr) {
    ptr->it = cache->lruSectors.emplace(cache->lruSectors.end(), ptr);
}

static sectorCache* cache_getLRUEntry(driveCache* cache) {
    return cache->lruSectors.front();
}

static void cache_makeMRU(driveCache* cache, sectorCache *ptr) {
    cache->lruSectors.splice(cache->lruSectors.end(), cache->lruSectors, ptr->it);
}

static sectorCache* cache_getFreeSector(driveCache* cache) {
    if (cache->freeSectors.empty())
        return nullptr;

    sectorCache* cached = cache->freeSectors.back();
    cache->freeSectors.resize(cache->freeSectors.size()-1);
    return cached;
}

static sectorCache* cache_getCachedSector(driveCache* cache, LBA_t sectorIdx) {
    if (auto it = cache->cachedSectors.find(sectorIdx); it != cache->cachedSectors.end())
        return it->second;
    return nullptr;
}

static uint32_t cache_getUncachedSectorCount(driveCache* cache, LBA_t sectorIdx, UINT maxSectorCount) {
    for (uint32_t i=0; i<maxSectorCount; i++) {
        if (cache_getCachedSector(cache, sectorIdx+i) != nullptr) {
            return i;
        }
    }
    return maxSectorCount;
}

static DRESULT cache_doRawRead(driveCache* cache, LBA_t sectorIdx, UINT sectorCount, BYTE* outputBuff) {
    FSError res = wiiu_readSectors(cache->pdrv, sectorIdx, sectorCount, outputBuff);
    if (res == FS_ERROR_MEDIA_NOT_READY) {
        return RES_NOTRDY;
    }
    if (res != FS_ERROR_OK) {
        return RES_ERROR;
    }
    return RES_OK;
}

static DRESULT cache_doRawWrite(driveCache* cache, LBA_t sectorIdx, UINT sectorCount, const BYTE* inputBuff) {
    FSError res = wiiu_writeSectors(cache->pdrv, sectorIdx, sectorCount, inputBuff);
    if (res == FS_ERROR_MEDIA_NOT_READY) {
        return RES_NOTRDY;
    }
    if (res != FS_ERROR_OK) {
        return RES_ERROR;
    }
    return RES_OK;
}

static uint32_t cache_getFlushableRange(driveCache* cache, LBA_t sectorIdx) {
    uint32_t maxFlushableSectorCount = 512 < cache->sectorCount ? 512 : cache->sectorCount;
    for (uint32_t i=0; i<maxFlushableSectorCount; i++) {
        auto ptr = cache_getCachedSector(cache, sectorIdx+i);
        if (ptr == nullptr || !ptr->dirty) {
            return i;
        }
    }
    return maxFlushableSectorCount;
}

static void cache_flushSectorRange(driveCache* cache, sectorCache* sector) {
    if (!sector->dirty)
        return;

    auto count = cache_getFlushableRange(cache, sector->sectorIdx);
    if (count == 0) {
        OSFatal("[ffcache] There shouldn't be a zero here?!\n");
        return;
    }
    //OSReport("[ffcache] Flushed %u offset with %u count!\n", sector->sectorIdx, count);

    BYTE* alignedBuffer = (BYTE*)aligned_alloc(0x40, count*cache->sectorSize);
    for (uint32_t i=0; i<count; i++) {
        auto writeSector = cache_getCachedSector(cache, sector->sectorIdx+i);
        memcpy(alignedBuffer+(i*cache->sectorSize), writeSector->buffer, cache->sectorSize);
        writeSector->dirty = false;
    }
    cache_doRawWrite(cache, sector->sectorIdx, count, alignedBuffer);
    free(alignedBuffer);
}

//static void cache_flushSingleSector(driveCache* cache, sectorCache* sector) {
//    if (sector->dirty) {
//        sector->dirty = false;
//        BYTE* alignedBuffer = (BYTE*)aligned_alloc(0x40, 1*cache->sectorSize);
//        memcpy(alignedBuffer, sector->buffer.data(), cache->sectorSize);
//        cache_doRawWrite(cache, sector->sectorIdx, 1, alignedBuffer);
//        free(alignedBuffer);
//    }
//}

static sectorCache* cache_getNewUncachedSector(driveCache* cache, LBA_t sectorIdx) {
    auto newSector = cache_getFreeSector(cache);
    if (newSector != nullptr) {
        newSector->sectorIdx = sectorIdx;
        newSector->dirty = false;
        cache_addLRUEntry(cache, newSector);
        cache->cachedSectors.emplace(sectorIdx, newSector);
        return newSector;
    }

    // Free oldest sector by replacing contents and moving it to the front
    newSector = cache_getLRUEntry(cache);
    cache_flushSectorRange(cache, newSector);
    cache->cachedSectors.erase(newSector->sectorIdx);
    newSector->sectorIdx = sectorIdx;
    newSector->dirty = false;
    cache_makeMRU(cache, newSector);
    cache->cachedSectors.emplace(sectorIdx, newSector);
    return newSector;
}

static void cache_writeSectors(driveCache* cache, LBA_t sectorIdx, UINT sectorCount, const BYTE* inputBuff) {
    for (uint32_t i=0; i<sectorCount; i++) {
        auto ptr = cache_getCachedSector(cache, sectorIdx+i);
        if (ptr == nullptr)
            ptr = cache_getNewUncachedSector(cache, sectorIdx+i);
        memcpy(ptr->buffer, inputBuff+(i*cache->sectorSize), cache->sectorSize);
        ptr->dirty = true;
    }
}

static void cache_readSectors(driveCache* cache, LBA_t sectorIdx, UINT sectorCount, BYTE* outputBuff) {
    for (uint32_t i=0; i<sectorCount; i++) {
        sectorCache* ptr = cache_getCachedSector(cache, sectorIdx+i);
        if (ptr != nullptr) {
            if (outputBuff != nullptr)
                memcpy(outputBuff+(i*cache->sectorSize), ptr->buffer, cache->sectorSize);
            cache_makeMRU(cache, ptr);
            continue;
        }
        uint32_t readRange = cache_getUncachedSectorCount(cache, sectorIdx+i, sectorCount-i);
        BYTE* alignedBuffer = (BYTE*)aligned_alloc(0x40, (sectorCount-i)*cache->sectorSize);
        cache_doRawRead(cache, sectorIdx+i, sectorCount-i, alignedBuffer);
        for (uint32_t f=i; f<i+readRange; f++) {
            sectorCache* newPtr = cache_getNewUncachedSector(cache, sectorIdx+f);
            memcpy(newPtr->buffer, alignedBuffer+((f-i)*cache->sectorSize), cache->sectorSize);
            if (outputBuff != nullptr)
                memcpy(outputBuff+(i*cache->sectorSize), newPtr->buffer, cache->sectorSize);
        }
        free(alignedBuffer);
        i += (readRange-1);
    }
}

#ifdef __cplusplus
extern "C" {
#endif

DRESULT ffcache_initialize(BYTE pdrv, DWORD sectorByteSize, DWORD sectorCount) {
    if (fatCaches[pdrv].initialized)
        return RES_ERROR;

    OSReport("[ffcache] Initializing cache...\n");

    fatCaches[pdrv].initialized = true;
    fatCaches[pdrv].sectorSize = sectorByteSize;
    fatCaches[pdrv].sectorCount = sectorCount;
    fatCaches[pdrv].pdrv = pdrv;

    fatCaches[pdrv].freeSectors.clear();
    fatCaches[pdrv].cachedSectors.clear();
    fatCaches[pdrv].lruSectors.clear();

    // Initialize sectors with sector index far outside possible range to prevent them from being used
    fatCaches[pdrv].sectorsBuffer = (BYTE*)aligned_alloc(0x40, fatCaches[pdrv].sectorCount*fatCaches[pdrv].sectorSize);
    for (uint32_t i=0; i<sectorCount; i++) {
        fatCaches[pdrv].freeSectors.emplace_back(new sectorCache{.buffer = fatCaches[pdrv].sectorsBuffer+(i*fatCaches[pdrv].sectorSize)});
    }
    return RES_OK;
}

DRESULT ffcache_flushSectors(BYTE pdrv) {
    auto cache = &fatCaches[pdrv];
    if (!cache->initialized)
        return RES_ERROR;

    while (!cache->lruSectors.empty()) {
        sectorCache* sector = cache->lruSectors.front();
        cache_flushSectorRange(cache, sector);
        cache->cachedSectors.erase(sector->sectorIdx);
        cache->freeSectors.emplace_back(sector);
        cache->lruSectors.pop_front();
    }
    return RES_OK;
}

void ffcache_shutdown(BYTE pdrv) {
    auto cache = &fatCaches[pdrv];
    if (!fatCaches[pdrv].initialized)
        return;

    // Delete all sectors
    ffcache_flushSectors(pdrv);
    while (!cache->freeSectors.empty()) {
        sectorCache* sector = cache->freeSectors.back();
        cache_flushSectorRange(cache, sector);
        cache->freeSectors.pop_back();
        delete sector;
    }

    free(cache->sectorsBuffer);
    dirCache_clear(pdrv);
    OSReport("[ffcache] Clearing %d cachedSectors, %d freeSectors, %d lruSectors...\n", cache->cachedSectors.size(), cache->freeSectors.size(), cache->lruSectors.size());
    fatCaches[pdrv].initialized = false;
}

DRESULT ffcache_readSectors(BYTE pdrv, LBA_t sectorOffset, UINT sectorCount, BYTE *bufferOut) {
    if (!fatCaches[pdrv].initialized)
        return RES_ERROR;

    cache_readSectors(&fatCaches[pdrv], sectorOffset, sectorCount, bufferOut);

    return RES_OK;
}

DRESULT ffcache_writeSectors(BYTE pdrv, LBA_t sectorOffset, UINT sectorCount, const BYTE *bufferIn) {
    if (!fatCaches[pdrv].initialized)
        return RES_ERROR;

    cache_writeSectors(&fatCaches[pdrv], sectorOffset, sectorCount, bufferIn);

    return RES_OK;
}

BYTE* ffcache_getSector(BYTE pdrv, LBA_t sectorOffset) {
    driveCache* cache = &fatCaches[pdrv];
    sectorCache* sector = nullptr;
    if (fatCaches[pdrv].activeCacheWindow->sectorIdx == sectorOffset) {
        sector = fatCaches[pdrv].activeCacheWindow;
        cache_makeMRU(&fatCaches[pdrv], sector);
    }
    else {
        sector = cache_getCachedSector(cache, sectorOffset);
        if (sector == nullptr) {
            sector = cache_getNewUncachedSector(cache, sectorOffset);
            cache_doRawRead(cache, sectorOffset, 1, sector->buffer);
        }
        else {
            cache_makeMRU(&fatCaches[pdrv], sector);
        }
        fatCaches[pdrv].activeCacheWindow = sector;
    }
    sector->dirty = true;
    return sector->buffer;
}


dirCache* dirCache_createSFN(BYTE pdrv, DIR* cache) {
    std::string keySFN(cache->fn, cache->fn + 11);
    DEBUG_OSReport("[dirCache] Added SFN for %.011s for cwd in %u with sect=%u, clust=%u, dptr=%u, blk_ofs=%u", keySFN.c_str(), cache->obj.sclust, cache->sect, cache->clust, cache->blk_ofs);
    auto [it, inserted] = fatCaches[pdrv].cachedDirSFNs.emplace(keySFN, dirCache{
        .cluster = cache->clust,
        .sectorOffset = cache->sect,
        .dirIdx = cache->dptr,
        .lfnStartDirIdx = cache->blk_ofs
    });
    if (!inserted) {
        DEBUG_OSReport("Tried inserting shortname %.011s twice?!", cache->fn);
        OSSleepTicks(OSSecondsToTicks(3));
    }
    return &it->second;
}

dirCache* dirCache_findSFN(BYTE pdrv, BYTE* sfnWithoutStatus) {
    auto it = fatCaches[pdrv].cachedDirSFNs.find(std::string(sfnWithoutStatus, sfnWithoutStatus+11));
    if (it == fatCaches[pdrv].cachedDirSFNs.end()) {
        return nullptr;
    }
    return &it->second;
}

dirCache* dirCache_createLFN(BYTE pdrv, const WCHAR* fullName, DIR* cache) {
    std::u16string utf16KeyName((char16_t*)fullName);
#if USE_DEBUG_STUBS
    std::string str;
    std::transform(utf16KeyName.begin(), utf16KeyName.end(), std::back_inserter(str), [](wchar_t c) {
        return (char)c;
    });
    DEBUG_OSReport("[dirCache] Added LFN for %.030s for cwd in %u with sect=%u, clust=%u, dptr=%u, blk_ofs=%u", str.c_str(), cache->obj.sclust, cache->sect, cache->clust, cache->blk_ofs);
#endif
    auto [it, inserted] = fatCaches[pdrv].cachedDirLFNs.emplace(utf16KeyName, dirCache{
            .cluster = cache->clust,
            .sectorOffset = cache->sect,
            .dirIdx = cache->dptr,
            .lfnStartDirIdx = cache->blk_ofs
    });
#if USE_DEBUG_STUBS
    if (!inserted) {
        DEBUG_OSReport("Tried inserting shortname %.030s twice?!", str.c_str());
        OSSleepTicks(OSSecondsToTicks(3));
    }
#endif
    return &it->second;
}

dirCache* dirCache_findLFN(BYTE pdrv, const WCHAR* fullName) {
    auto it = fatCaches[pdrv].cachedDirLFNs.find((char16_t*)fullName);
    if (it == fatCaches[pdrv].cachedDirLFNs.end()) {
        return nullptr;
    }
    return &it->second;
}





void dirCache_clear(BYTE pdrv) {
    DEBUG_OSReport("Clear dirCache!");
    fatCaches[pdrv].cachedDirLastAllocatedIdx = 0;
    fatCaches[pdrv].lastDirTable = 0xFFFFFFFF;
    fatCaches[pdrv].cachedDirSFNs.clear();
    fatCaches[pdrv].cachedDirLFNs.clear();
}



void dirCache_setLastAllocatedIdx(BYTE pdrv, DWORD lastIdx, DWORD lastDirTable) {
    if (lastDirTable == fatCaches[pdrv].lastDirTable) {
        if (lastIdx <= fatCaches[pdrv].cachedDirLastAllocatedIdx) {
            DEBUG_OSReport("HOW CAN THE INDEX BE SMALLER?!");
            //OSFatal("HOW CAN THE INDEX BE SMALLER?!\n");
        }
        fatCaches[pdrv].cachedDirLastAllocatedIdx = lastIdx;
    }
    else {
        fatCaches[pdrv].lastDirTable = lastDirTable;
        fatCaches[pdrv].cachedDirLastAllocatedIdx = 0;
    }
}

DWORD dirCache_getLastClusterIdx(BYTE pdrv, DWORD currDirTableIdx) {
    if (currDirTableIdx != fatCaches[pdrv].lastDirTable) return 0;
    return fatCaches[pdrv].cachedDirLastAllocatedIdx;
}


std::mutex _mutex;
std::unordered_map<const char*, double> segmentTimes;

uint64_t cache_getTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t profile_startSegment() {
    return cache_getTime();
}

void profile_endSegment(const char* segmentName, uint64_t startTime) {
    std::scoped_lock<std::mutex> lck(_mutex);
    double timeSpent = (double)(cache_getTime() - startTime);
    segmentTimes[segmentName] += timeSpent;
}

void profile_incrementCounter(const char* segmentName) {
    std::scoped_lock<std::mutex> lck(_mutex);
    segmentTimes[segmentName] += 1.0f;
}

double profile_getSegment(const char* segmentName) {
    std::scoped_lock<std::mutex> lck(_mutex);
    double ret = segmentTimes[segmentName];
    segmentTimes[segmentName] = 0;
    return ret;
}



#ifdef __cplusplus
}
#endif