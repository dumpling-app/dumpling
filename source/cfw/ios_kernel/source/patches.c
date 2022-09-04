#include "common.h"
#include "patches.h"

#include "../../ios_mcp/ios_mcp_syms.h"
#include "../../ios_fs/ios_fs_syms.h"


int32_t kernel_syscall_0x81(uint32_t command, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    ThreadContext_t **currentThreadContext = (ThreadContext_t **)0x08173BA0;
    uint32_t *domainAccessPermissions      = (uint32_t *)0x081A4000;

    int32_t result = 0;
    int32_t level = disableInterrupts();
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
    // Insert jump to the kernel_syscall_0x81
    *(volatile uint32_t*)0x0812CD2C = ARM_B(0x0812CD2C, kernel_syscall_0x81);

    // Add IOCTL 0x28 to indicate the calling client should have full fs permissions
    *(volatile uint32_t*)0x10701248 = _FSA_ioctl0x28_hook;

    // Patch FSA raw access
    *(volatile uint32_t*)0x1070FAE8 = 0x05812070;
    *(volatile uint32_t*)0x1070FAEC = 0xEAFFFFF9;

    // Give clients that called IOCTL 0x28 full permissions
    *(volatile uint32_t*)0x10704540 = ARM_BL(0x10704540, FSA_IOCTLV_HOOK);
    *(volatile uint32_t*)0x107044f0 = ARM_BL(0x107044F0, FSA_IOCTL_HOOK);
    *(volatile uint32_t*)0x10704458 = ARM_BL(0x10704458, FSA_IOS_Close_Hook);

    // Reset FS bss section
    kernelMemset(KERNEL_SRC_ADDR(_fs_bss_start), 0, _fs_bss_end - _fs_bss_start);
    
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

    // Patch FS to syslog everything
    *(volatile uint32_t*)(0x107F5720) = ARM_B(0x107F5720, 0x107F0C84);

    // Patch MCP to syslog everything
    *(volatile uint32_t*)KERNEL_RUN_ADDR(0x05055438) = ARM_B(0x05055438, 0x0503dcf8);

    // Zero out the MCP payload's .bss data area
    kernelMemset(KERNEL_SRC_ADDR(_mcp_bss_start), 0, _mcp_bss_end - _mcp_bss_start);

    // Insert jump to the custom ioctl100 patch
    *(volatile uint32_t*)KERNEL_SRC_ADDR(0x05025242) = THUMB_BL(0x05025242, MCP_ioctl100_patch);

    int32_t (*internal_MapSharedUserExecution)(void *descr) = (void*)0x08124F88;
    ios_map_shared_info_t map_info;
    map_info.paddr  = (uint32_t)KERNEL_SRC_ADDR(_mcp_bss_start);
    map_info.vaddr  = _mcp_bss_start;
    map_info.size   = 0x3000;
    map_info.domain = 1; // MCP
    map_info.type   = 3; // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
    map_info.cached = 0xFFFFFFFF;
    internal_MapSharedUserExecution(&map_info); // actually a bss section but oh well it will have read/write

    map_info.paddr  = (uint32_t)KERNEL_RUN_ADDR(_mcp_text_start);
    map_info.vaddr  = _mcp_text_start;
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