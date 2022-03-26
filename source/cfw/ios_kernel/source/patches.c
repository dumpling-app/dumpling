#include "common.h"
#include "patches.h"

#include "../../ios_mcp/ios_mcp_syms.h"


int kernel_syscall_0x81(uint32_t command, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    ThreadContext_t **currentThreadContext = (ThreadContext_t **)0x08173BA0;
    uint32_t *domainAccessPermissions      = (uint32_t *)0x081A4000;

    int result = 0;
    int level = disableInterrupts();
    setDomainRegister(domainAccessPermissions[0]); // 0 = KERNEL

    switch (command) {
        case KERNEL_READ32: {
            result = *(volatile uint32_t*)arg1;
            break;
        }
        case KERNEL_WRITE32: {
            *(volatile uint32_t*)arg1 = arg2;
            break;
        }
        case KERNEL_MEMCPY: {
            kernelMemcpy((void *)arg1, (void *)arg2, arg3);
            break;
        }
        case KERNEL_GET_CFW_CONFIG: {
            break;
        }
        case KERNEL_READ_OTP: {
            internal_readOTP(0, (void *)arg1, 0x400);
            break;
        }
        default: {
            result = -1;
            break;
        }
    }

    setDomainRegister(domainAccessPermissions[(*currentThreadContext)->pid]);
    enableInterrupts(level);

    return result;
}

void installPatches() {
    // Patch FSA raw access
    *(volatile uint32_t*)0x1070FAE8 = 0x05812070;
    *(volatile uint32_t*)0x1070FAEC = 0xEAFFFFF9;

    // Patch /dev/odm IOCTL 0x06 to return the disc key if in_buf[0] > 2.
    // *(volatile uint32_t*)0x10739948 = 0xe3a0b001; // mov r11, 0x01
    // *(volatile uint32_t*)0x1073994C = 0xe3a07020; // mov r7, 0x20
    // *(volatile uint32_t*)0x10739950 = 0xea000013; // b LAB_107399a8

    // Patch kernel dev node registration
    *(volatile uint32_t*)0x081430B4 = 1;

    // Fix 10 minute timeout that crashes MCP after 10 minutes of booting
    *(volatile uint32_t*)KERNEL_SRC_ADDR(0x05022474) = 0xFFFFFFFF; // NEW_TIMEOUT

    // Give PPC permission to bsp::ee:read permission to be able to read seeprom
    *(volatile uint32_t*)(0xe6044db0 - 0xe6042000 + 0x13d02000) = 0x000001F0;

    // Zero out the MCP payload's .bss data area
    kernelMemset(KERNEL_SRC_ADDR(0x050BD000), 0, 0x2F00);

    // Insert jump to the custom ioctl100 patch
    *(volatile uint32_t*)KERNEL_SRC_ADDR(0x05025242) = THUMB_BL(0x05025242, MCP_ioctl100_patch);

    // Insert jump to the kernel_syscall_0x81
    *(volatile uint32_t*)0x0812CD2C = ARM_B(0x0812CD2C, kernel_syscall_0x81);

    int32_t (*internal_MapSharedUserExecution)(void *descr) = (void*)0x08124F88;
    ios_map_shared_info_t map_info;
    map_info.paddr  = 0x050BD000 - 0x05000000 + 0x081C0000;
    map_info.vaddr  = 0x050BD000;
    map_info.size   = 0x3000;
    map_info.domain = 1; // MCP
    map_info.type   = 3; // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
    map_info.cached = 0xFFFFFFFF;
    internal_MapSharedUserExecution(&map_info); // actually a bss section but oh well it will have read/write

    map_info.paddr  = 0x05116000 - 0x05100000 + 0x13D80000;
    map_info.vaddr  = 0x05116000;
    map_info.size   = 0x4000;
    map_info.domain = 1; // MCP
    map_info.type   = 3; // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read write
    map_info.cached = 0xFFFFFFFF;
    internal_MapSharedUserExecution(&map_info);

    internal_setClientCapabilites(COS_MASTER, FS, 0xFFFFFFFFFFFFFFFF);
    internal_setClientCapabilites(COS_MASTER, MCP, 0xFFFFFFFFFFFFFFFF);
    internal_setClientCapabilites(COS_MASTER, ACT, 0xFFFFFFFFFFFFFFFF);
    internal_setClientCapabilites(COS_MASTER, FPD, 0xFFFFFFFFFFFFFFFF);
    internal_setClientCapabilites(COS_MASTER, NSEC, 0xFFFFFFFFFFFFFFFF);
    internal_setClientCapabilites(COS_MASTER, BSP, 0xFFFFFFFFFFFFFFFF);
    internal_setClientCapabilites(COS_MASTER, ACP, 0xFFFFFFFFFFFFFFFF);
}