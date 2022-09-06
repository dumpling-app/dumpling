#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/foreground.h>
#include <coreinit/memheap.h>
#include <coreinit/memory.h>
#include <coreinit/debug.h>
#include <coreinit/memlist.h>
#include <coreinit/memorymap.h>
#include <coreinit/dynload.h>
#include <coreinit/memdefaultheap.h>
#include <errno.h>
#include <whb/crash.h>

// Some additional stubs that apparently don't work anymore?

void __asan_before_dynamic_init(const void *module_name) {
};

void __asan_after_dynamic_init() {
}

#if defined(__clang__) || defined (__GNUC__)
# define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif


/* Leak detection! Woo! */
typedef struct _lsan_allocation {
    bool allocated;
    void* addr;
    size_t size;
    void* owner;
} lsan_allocation;
#define LSAN_ALLOCS_SZ 0x10000
static lsan_allocation lsan_allocs[LSAN_ALLOCS_SZ];

static MEMHeapHandle mem1_heap;
static MEMHeapHandle bucket_heap;
static MEMHeapHandle asan_heap;
static void* asan_heap_mem;
static unsigned int asan_sz;

static void* asan_shadow;
static unsigned int asan_shadow_off;
static bool asan_ready = false;

#define MEM_TO_SHADOW(ptr) (unsigned char*)((((void*)ptr - asan_heap_mem) >> 3) + asan_shadow)
#define SET_SHADOW(shadow, ptr) (*shadow) |= 1 << ((unsigned int)ptr & 0x7)
#define CLR_SHADOW(shadow, ptr) (*shadow) &= ~(1 << ((unsigned int)ptr & 0x7))
#define GET_SHADOW(shadow, ptr) (*shadow >> ((unsigned int)ptr & 0x7)) & 0x1

ATTRIBUTE_NO_SANITIZE_ADDRESS
bool __asan_checkrange(void* ptr, size_t sz) {
    bool okay = true;
    if (ptr >= asan_heap_mem && ptr <= asan_shadow) {
        for (int i = 0; i < sz; i++) {
            unsigned char* shadow = MEM_TO_SHADOW(ptr + i);
            bool valid = GET_SHADOW(shadow, ptr + i);
            if (!valid) okay = false;
        }
    }
    return okay;
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void __asan_doreport(void* ptr, void* caller, const char* type); //@ bottom of file


#define ASAN_GENFUNC(type, num) void __asan_##type##num##_noabort(void* ptr) { \
   if (!asan_ready) return; \
   bool valid = __asan_checkrange(ptr, num); \
   if (!valid) __asan_doreport(ptr, __builtin_return_address(0), #type); \
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(load, 1);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(store, 1);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(load, 2);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(store, 2);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(load, 4);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(store, 4);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(load, 8);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(store, 8);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(load, 16);
ATTRIBUTE_NO_SANITIZE_ADDRESS
ASAN_GENFUNC(store, 16);

ATTRIBUTE_NO_SANITIZE_ADDRESS
void __asan_loadN_noabort(void* ptr, unsigned int size) {
    if (!asan_ready) return;
    bool valid = __asan_checkrange(ptr, (size_t)size);
    if (!valid) __asan_doreport(ptr, __builtin_return_address(0), "multiple load");
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void __asan_storeN_noabort(void* ptr, unsigned int size) {
    if (!asan_ready) return;
    bool valid = __asan_checkrange(ptr, (size_t)size);
    if (!valid) __asan_doreport(ptr, __builtin_return_address(0), "multiple store");
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void __asan_handle_no_return() {
    // We only do heap checking, so no need to fiddle here
}

void __init_wut_malloc();

ATTRIBUTE_NO_SANITIZE_ADDRESS
void memoryInitialize(void) {
    for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
        lsan_allocs[i].allocated = false;
    }
    OSReport("[LSAN] LSAN initialized\n");

    MEMHeapHandle mem2_heap_handle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
    unsigned int mem2_alloc_sz = MEMGetAllocatableSizeForExpHeapEx(mem2_heap_handle, 4);
    asan_sz = (mem2_alloc_sz / 4) & ~0x3;

    asan_heap_mem = MEMAllocFromExpHeapEx(mem2_heap_handle, asan_sz * 2, 4);

    asan_heap = MEMCreateExpHeapEx(asan_heap_mem, asan_sz, 0);
    unsigned int asan_heap_sz = MEMGetAllocatableSizeForExpHeapEx(asan_heap, 4);

    OSReport("[ASAN] Allocated wrapheap at %08X, size %08X. Avail RAM is %08X.\n", asan_heap, asan_sz, asan_heap_sz);

    asan_shadow = (void*)(((unsigned int)asan_heap_mem) + asan_sz);
    asan_shadow_off = (unsigned int)asan_heap - (unsigned int)asan_shadow;

    memset(asan_shadow, 0, asan_sz);

    OSReport("[ASAN] Shadow at %08X. Final shadow address at %08X, final alloced at %08X\n", asan_shadow, asan_shadow + asan_sz, asan_heap_mem + (asan_sz * 2));

    asan_ready = true;

    MEMHeapHandle mem1_heap_handle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    unsigned int mem1_allocatable_size = MEMGetAllocatableSizeForFrmHeapEx(mem1_heap_handle, 4);
    void* mem1_memory = MEMAllocFromFrmHeapEx(mem1_heap_handle, mem1_allocatable_size, 4);
    if (mem1_memory)
        mem1_heap = MEMCreateExpHeapEx(mem1_memory, mem1_allocatable_size, 0);

    MEMHeapHandle bucket_heap_handle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);
    unsigned int bucket_allocatable_size = MEMGetAllocatableSizeForFrmHeapEx(bucket_heap_handle, 4);
    void* bucket_memory = MEMAllocFromFrmHeapEx(bucket_heap_handle, bucket_allocatable_size, 4);
    if (bucket_memory)
        bucket_heap = MEMCreateExpHeapEx(bucket_memory, bucket_allocatable_size, 0);
    
    __init_wut_malloc();
}

// ATTRIBUTE_NO_SANITIZE_ADDRESS
// void memoryRelease(void) {
//     for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
//         if (lsan_allocs[i].allocated) {
//             OSReport("[LSAN] Memory leak: %08X bytes at %08X; owner %08X\n", lsan_allocs[i].size, lsan_allocs[i].addr, lsan_allocs[i].owner);
//             break;
//         }
//     }
//     asan_ready = false;
//     MEMDestroyExpHeap(asan_heap);
//     MEMFreeToExpHeap(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2), asan_heap_mem);
//     asan_heap = NULL;
//     asan_heap_mem = NULL;
//     MEMDestroyExpHeap(mem1_heap);
//     MEMFreeToFrmHeap(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1), MEM_FRM_HEAP_FREE_ALL);
//     mem1_heap = NULL;
//     MEMDestroyExpHeap(bucket_heap);
//     MEMFreeToFrmHeap(MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG), MEM_FRM_HEAP_FREE_ALL);
//     bucket_heap = NULL;
// }


static MEMHeapHandle sRootMem2Heap;
static uint32_t sRootHeapMem2Addr = 0;
static uint32_t sRootHeapMem2Size = 0;


static void* CustomAllocFromDefaultHeap(uint32_t size) {
    return MEMAllocFromExpHeapEx(sRootMem2Heap, size, 4);
}

static void* CustomAllocFromDefaultHeapEx(uint32_t size, int32_t alignment) {
    return MEMAllocFromExpHeapEx(sRootMem2Heap, size, alignment);
}

static void CustomFreeToDefaultHeap(void* ptr) {
    MEMFreeToExpHeap(sRootMem2Heap, ptr);
}

static OSDynLoad_Error CustomDynLoadAlloc(int32_t size, int32_t align, void** outAddr) {
    if (!outAddr) {
        return OS_DYNLOAD_INVALID_ALLOCATOR_PTR;
    }

    if (align >= 0 && align < 4) {
        align = 4;
    }
    else if (align < 0 && align > -4) {
        align = -4;
    }

    if (!(*outAddr = MEMAllocFromDefaultHeapEx(size, align))) {
        return OS_DYNLOAD_OUT_OF_MEMORY;
    }

    return OS_DYNLOAD_OK;
}

static void CustomDynLoadFree(void* addr) {
    MEMFreeToDefaultHeap(addr);
}



OSMutex stuff;

void __preinit_user(MEMHeapHandle* mem1, MEMHeapHandle* foreground, MEMHeapHandle* mem2) {
    uint32_t addr, size;

    MEMAllocFromDefaultHeap = CustomAllocFromDefaultHeap;
    MEMAllocFromDefaultHeapEx = CustomAllocFromDefaultHeapEx;
    MEMFreeToDefaultHeap = CustomFreeToDefaultHeap;

    OSGetMemBound(OS_MEM2, &addr, &size);
    sRootMem2Heap = MEMCreateExpHeapEx((void*)addr, size, MEM_HEAP_FLAG_USE_LOCK);
    sRootHeapMem2Addr = addr;
    sRootHeapMem2Size = size;
    *mem2 = sRootMem2Heap;

    if (OSGetForegroundBucket(NULL, NULL)) {
        OSGetMemBound(OS_MEM1, &addr, &size);
        *mem1 = MEMCreateFrmHeapEx((void *)addr, size, 0);

        OSGetForegroundBucketFreeArea(&addr, &size);
        *foreground = MEMCreateFrmHeapEx((void *)addr, size, 0);
    }

    for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
        lsan_allocs[i].allocated = false;
    }
    OSReport("[LSAN] LSAN initialized\n");
    OSInitMutex(&stuff);

    MEMHeapHandle mem2_heap_handle = *mem2;
    unsigned int mem2_alloc_sz = MEMGetAllocatableSizeForExpHeapEx(mem2_heap_handle, 4);
    asan_sz = (mem2_alloc_sz / 4) & ~0x3;

    asan_heap_mem = MEMAllocFromExpHeapEx(mem2_heap_handle, asan_sz * 2, 4);

    asan_heap = MEMCreateExpHeapEx(asan_heap_mem, asan_sz, 0);
    unsigned int asan_heap_sz = MEMGetAllocatableSizeForExpHeapEx(asan_heap, 4);

    OSReport("[ASAN] Allocated wrapheap at %08X, size %08X. Avail RAM is %08X.\n", asan_heap, asan_sz, asan_heap_sz);

    asan_shadow = (void*)(((unsigned int)asan_heap_mem) + asan_sz);
    asan_shadow_off = (unsigned int)asan_heap - (unsigned int)asan_shadow;

    memset(asan_shadow, 0, asan_sz);

    OSReport("[ASAN] Shadow at %08X. Final shadow address at %08X, final alloced at %08X\n", asan_shadow, asan_shadow + asan_sz, asan_heap_mem + (asan_sz * 2));

    asan_ready = true;

    MEMHeapHandle mem1_heap_handle = *mem1;
    unsigned int mem1_allocatable_size = MEMGetAllocatableSizeForFrmHeapEx(mem1_heap_handle, 4);
    void* mem1_memory = MEMAllocFromFrmHeapEx(mem1_heap_handle, mem1_allocatable_size, 4);
    if (mem1_memory)
        mem1_heap = MEMCreateExpHeapEx(mem1_memory, mem1_allocatable_size, 0);

    MEMHeapHandle bucket_heap_handle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);
    unsigned int bucket_allocatable_size = MEMGetAllocatableSizeForFrmHeapEx(bucket_heap_handle, 4);
    void* bucket_memory = MEMAllocFromFrmHeapEx(bucket_heap_handle, bucket_allocatable_size, 4);
    if (bucket_memory)
        bucket_heap = MEMCreateExpHeapEx(bucket_memory, bucket_allocatable_size, 0);


    OSDynLoad_SetAllocator(CustomDynLoadAlloc, CustomDynLoadFree);
    OSDynLoad_SetTLSAllocator(CustomDynLoadAlloc, CustomDynLoadFree);
}

#include <coreinit/mutex.h>

ATTRIBUTE_NO_SANITIZE_ADDRESS
void* _memalign_r(struct _reent* r, size_t alignment, size_t size) {
    OSLockMutex(&stuff);
    void* ptr = MEMAllocFromExpHeapEx(asan_heap, size, alignment);

    if (!ptr) {
        OSUnlockMutex(&stuff);
        return 0;
    };

    bool lsan_ok = false;
    for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
        if (!lsan_allocs[i].allocated) {
            lsan_allocs[i].allocated = true;
            lsan_allocs[i].addr = ptr;
            lsan_allocs[i].size = size;
            lsan_allocs[i].owner = __builtin_return_address(0);
            lsan_ok = true;
            OSReport("[DSAN] Saved alloc %08X, sz %08X\n", ptr, size);
            break;
        }
    }

    if (!lsan_ok) {
        OSReport("[A/LSAN] WARNING: Too many allocs!\n");
    }
    else {
        /* Add to ASAN */
        for (size_t i = 0; i < size; i++) {
            SET_SHADOW(MEM_TO_SHADOW(ptr + i), ptr + i);
        }
    }

    OSUnlockMutex(&stuff);
    return ptr;
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void* _malloc_r(struct _reent* r, size_t size) {
    return _memalign_r(r, 4, size);
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void _free_r(struct _reent* r, void* ptr) {
    OSLockMutex(&stuff);
    if (ptr) {
        size_t sz = 0;

        bool lsan_ok = false;
        for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
            if (lsan_allocs[i].allocated && lsan_allocs[i].addr == ptr) {
                lsan_allocs[i].allocated = false;
                sz = lsan_allocs[i].size; //Thanks, LSAN!
                lsan_ok = true;
                OSReport("[DSAN] Freed %08X, sz %08X\n", lsan_allocs[i].addr, lsan_allocs[i].size);
                break;
            }
        }

        if (!lsan_ok) {
            OSReport("[LSAN] WARNING: attempted free at address %08X with size %08x; not in table!\n", ptr);
            for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
                if (lsan_allocs[i].allocated) {
                    OSReport(" - Existing Allocations: %08X with size %08X\n", lsan_allocs[i].addr, lsan_allocs[i].size);
                }
            }
            OSSleepTicks(OSSecondsToTicks(100000));
            OSUnlockMutex(&stuff);
            return;
        }

        for (size_t i = 0; i < sz; i++) {
            CLR_SHADOW(MEM_TO_SHADOW(ptr + i), ptr + i);
        }

        MEMFreeToExpHeap(asan_heap, ptr);
    }
    OSUnlockMutex(&stuff);
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
size_t _malloc_usable_size_r(struct _reent* r, void* ptr) {
    return MEMGetSizeForMBlockExpHeap(ptr);
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void* _realloc_r(struct _reent* r, void* ptr, size_t size) {
    void* realloc_ptr = NULL;
    OSLockMutex(&stuff);
    if (!ptr) {
        OSUnlockMutex(&stuff);
        return _malloc_r(r, size);
    }

    if (_malloc_usable_size_r(r, ptr) >= size) {
        for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
            if (lsan_allocs[i].addr == ptr) {
                lsan_allocs[i].size = size;
                break;
            }
        }
        for (size_t i = 0; i < size; i++) {
            SET_SHADOW(MEM_TO_SHADOW(ptr + i), ptr + i);
        }
        OSUnlockMutex(&stuff);
        return ptr;
    }

    realloc_ptr = _malloc_r(r, size);

    if (!realloc_ptr) {
        OSUnlockMutex(&stuff);
        return NULL;
    }

    memcpy(realloc_ptr, ptr, _malloc_usable_size_r(r, ptr));
    _free_r(r, ptr);
    OSUnlockMutex(&stuff);
    return realloc_ptr;
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void* _calloc_r(struct _reent* r, size_t num, size_t size) {
    void* ptr = _malloc_r(r, num * size);

    if (ptr)
        memset(ptr, 0, num * size);

    return ptr;
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void* _valloc_r(struct _reent* r, size_t size) {
    return _memalign_r(r, 64, size);
}


/* some wrappers */

void* MEM2_alloc(unsigned int size, unsigned int align) {
    return _memalign_r(NULL, align, size);
    //return MEMAllocFromExpHeapEx(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2), size, align);
}

void MEM2_free(void* ptr) {
    if (ptr)
        _free_r(NULL, ptr);
    //MEMFreeToExpHeap(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2), ptr);
}

void* MEM1_alloc(unsigned int size, unsigned int align) {
    if (align < 4)
        align = 4;
    return MEMAllocFromExpHeapEx(mem1_heap, size, align);
}

void MEM1_free(void* ptr) {
    if (ptr)
        MEMFreeToExpHeap(mem1_heap, ptr);
}

void* MEMBucket_alloc(unsigned int size, unsigned int align) {
    if (align < 4)
        align = 4;
    return MEMAllocFromExpHeapEx(bucket_heap, size, align);
}

void MEMBucket_free(void* ptr) {
    if (ptr)
        MEMFreeToExpHeap(bucket_heap, ptr);
}

typedef struct _framerec {
    struct _framerec* up;
    void* lr;
} frame_rec, * frame_rec_t;

extern unsigned int __code_start;
#define TEXT_START (void*)&__code_start
extern unsigned int __code_end;
#define TEXT_END (void*)&__code_end

#define buf_add(fmt, ...) cur_buf += snprintf(cur_buf, 2047 - (buf - cur_buf), fmt, __VA_ARGS__)

void __asan_doreport(void* ptr, void* caller, const char* type) {
    char buf[2048];
    char* cur_buf = buf;
    buf_add("======================== ASAN: out-of-bounds %s\n", type);

    if (caller >= TEXT_START && caller < TEXT_END) {
        char symbolName[64];
        OSGetSymbolName((unsigned int)caller, symbolName, 63);
        //don't want library name today
        char* name = strchr(symbolName, '|') + sizeof(char);

        buf_add("Source: %08X(%08X):%s\n", caller, caller - TEXT_START, name);
    }
    else {
        buf_add("Source: %08X(        ):<unknown>\n", caller);
    }

    //Thanks, LSAN!
    int closest_over = -1, closest_under = -1, uaf = -1;
    for (int i = 0; i < LSAN_ALLOCS_SZ; i++) {
        /* Candidate for closest if we are an overflow */
        if (lsan_allocs[i].allocated && ptr > lsan_allocs[i].addr + lsan_allocs[i].size) {
            if (closest_over == -1) {
                closest_over = i;
                /* If this allocation is closer than the one in closest_over, update */
            }
            else if (ptr - (lsan_allocs[i].addr + lsan_allocs[i].size) < ptr - (lsan_allocs[closest_over].addr + lsan_allocs[closest_over].size)) {
                closest_over = i;
            }
            /* Candidate for under */
        }
        else if (lsan_allocs[i].allocated && ptr < lsan_allocs[i].addr) {
            if (closest_under == -1) {
                closest_under = i;
            }
            else if (lsan_allocs[i].addr - ptr < lsan_allocs[closest_under].addr - ptr) {
                closest_under = i;
            }
        }
        else if (!lsan_allocs[i].allocated && ptr > lsan_allocs[i].addr && ptr < lsan_allocs[i].addr + lsan_allocs[i].size) {
            if (uaf == -1) {
                uaf = i;
            }
        }
    }

    buf_add("Bad %s was on address %08X. Guessing possible issues:\n", type, ptr);

    buf_add("Guess     Chunk    ChunkSz  Dist     ChunkOwner\n", 0);
    if (closest_over >= 0) {
        char symbolName[64];
        OSGetSymbolName((unsigned int)lsan_allocs[closest_over].owner, symbolName, 63);
        char* name = strchr(symbolName, '|') + sizeof(char);

        buf_add("Overflow  %08X %08X %08X %s\n", lsan_allocs[closest_over].addr, lsan_allocs[closest_over].size, ptr - (lsan_allocs[closest_over].addr + lsan_allocs[closest_over].size), name);
    }
    if (closest_under >= 0) {
        char symbolName[64];
        OSGetSymbolName((unsigned int)lsan_allocs[closest_under].owner, symbolName, 63);
        char* name = strchr(symbolName, '|') + sizeof(char);

        buf_add("Underflow %08X %08X %08X %s\n", lsan_allocs[closest_under].addr, lsan_allocs[closest_under].size, lsan_allocs[closest_under].addr - ptr, name);
    }
    if (uaf >= 0) {
        char symbolName[64];
        OSGetSymbolName((unsigned int)lsan_allocs[uaf].owner, symbolName, 63);
        char* name = strchr(symbolName, '|') + sizeof(char);

        buf_add("UaF       %08X %08X %08X %s\n", lsan_allocs[uaf].addr, lsan_allocs[uaf].size, ptr - lsan_allocs[uaf].addr, name);
    }

    OSFatal(buf);
}