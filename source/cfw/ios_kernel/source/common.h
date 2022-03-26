#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ARM_B(addr, func)   (0xEA000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))
#define ARM_BL(addr, func)  (0xEB000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))
#define THUMB_B(addr, func)     ((0xE000 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x7FF)))
#define THUMB_BL(addr, func)    ((0xF000F800 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x0FFF)) | ((((uint32_t)(func) - (uint32_t)(addr) - 4) << 4) & 0x7FFF000))

#define KERNEL_RUN_ADDR(addr) (void*)(addr - 0x05100000 + 0x13D80000)
#define KERNEL_SRC_ADDR(addr) (void*)(addr - 0x05000000 + 0x081C0000)

typedef struct ThreadContext {
    uint32_t cspr;
    uint32_t gpr[14];
    uint32_t lr;
    uint32_t pc;
    struct ThreadContext* threadQueueNext;
    uint32_t maxPriority;
    uint32_t priority;
    uint32_t state;
    uint32_t pid;
    uint32_t id;
    uint32_t flags;
    uint32_t exitValue;
    struct ThreadContext** joinQueue;
    struct ThreadContext** threadQueue;
    uint8_t unk1[56];
    void* stackPointer;
    uint8_t unk2[8];
    void* sysStackAddr;
    void* userStackAddr;
    uint32_t userStackSize;
    void* threadLocalStorage;
    uint32_t profileCount;
    uint32_t profileTime;
} ThreadContext_t;

typedef struct {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t domain;
    uint32_t type;
    uint32_t cached;
} ios_map_shared_info_t;

typedef enum IOSU_ProcessID {
    IOS_KERNEL = 0,
    IOS_MCP = 1,
    IOS_BSP = 2,
    IOS_CRYPTO = 3,
    IOS_USB = 4,
    IOS_FS = 5,
    IOS_PAD = 6,
    IOS_NET = 7,
    IOS_ACP = 8,
    IOS_NSEC = 9,
    IOS_AUXIL = 10,
    IOS_NIM_BOSS = 11,
    IOS_FPD = 12,
    IOS_TEST = 13,
    COS_KERNEL = 14,
    COS_ROOT = 15,
    COS_02 = 16,
    COS_03 = 17,
    COS_OVERLAY = 18,
    COS_HBM = 19,
    COS_ERROR = 20,
    COS_MASTER = 21,
} IOSU_ProcessID;

typedef enum IOSU_PermGroup {
    BSP = 1,
    DK = 3,
    USB_CLS_DRV = 9,
    UHS = 12,
    FS = 11,
    MCP = 13,
    NIM = 14,
    ACT = 15,
    FPD = 16,
    BOSS = 17,
    ACP = 18,
    PDM = 29,
    AC = 20,
    NDM = 21,
    NSEC = 22,
} IOSU_PermGroup;

#define disableInterrupts   ((int32_t (*)())0x0812E778)
#define enableInterrupts    ((int32_t (*)(int32_t))0x0812E78C)
#define kernelMemset        ((void * (*)(void *, int32_t, uint32_t))0x08131DA0)
#define kernelMemcpy        ((void * (*)(void *, const void *, int32_t))0x08131D04)

#define invalidateICache    ((void(*)())0x0812DCF0)
#define invalidateDCache    ((void(*)(uint32_t, uint32_t))0x08120164)
#define flushDCache         ((void(*)(uint32_t, uint32_t))0x08120160)

#define internal_mapSharedUserExecution ((int32_t (*)(void *))0x08124F88)
#define internal_setClientCapabilites   ((int32_t (*)(IOSU_ProcessID, IOSU_PermGroup, uint64_t))0x081260A8)
#define internal_readOTP                ((int32_t (*)(int32_t, void*, uint32_t))0x08120248)

static inline uint32_t disableMMU() {
    uint32_t controlRegister = 0;
    asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (controlRegister));
    asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (controlRegister & 0xFFFFEFFA));
    return controlRegister;
}

static inline void restoreMMU(uint32_t controlRegister) {
    asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (controlRegister));
}

static inline void setDomainRegister(unsigned int domain_register) {
    asm volatile("MCR p15, 0, %0, c3, c0, 0" : : "r" (domain_register));
}