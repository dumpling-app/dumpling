#include "common.h"
#include "patches.h"

#include "../../ios_mcp/ios_mcp_syms.h"

ThreadContext_t **currentThreadContext = (ThreadContext_t **)0x08173ba0;
uint32_t *domainAccessPermissions      = (uint32_t *)0x081a4000;

#define disable_interrupts   ((int32_t (*)())0x0812E778)
#define enable_interrupts    ((int32_t (*)(int32_t))0x0812E78C)
#define kernel_memset        ((void *(*) (void *, int32_t, uint32_t))0x08131DA0)
#define kernel_memcpy        ((void *(*) (void *, const void *, int32_t))0x08131D04)

#define KERNEL_READ32         1
#define KERNEL_WRITE32        2
#define KERNEL_MEMCPY         3
#define KERNEL_GET_CFW_CONFIG 4
#define KERNEL_READ_OTP       5


int kernel_syscall_0x81(uint32_t command, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int result = 0;
    int level = disable_interrupts();
    setDomainRegister(domainAccessPermissions[0]); // 0 = KERNEL

    switch (command) {
        case KERNEL_READ32: {
            result = *(volatile uint32_t*)arg1;
            break;
        }
        case KERNEL_WRITE32: {
            *(volatile uint32_t*) arg1 = arg2;
            break;
        }
        case KERNEL_MEMCPY: {
            kernel_memcpy((void *)arg1, (void *)arg2, arg3);
            break;
        }
        case KERNEL_GET_CFW_CONFIG: {
            break;
        }
        case KERNEL_READ_OTP: {
            int32_t (*read_otp_internal)(int32_t index, void *out_buf, uint32_t size) = (int32_t (*)(int32_t, void *, uint32_t))0x08120248;
            read_otp_internal(0, (void *)(arg1), 0x400);
            break;
        }
        default: {
            result = -1;
            break;
        }
    }

    setDomainRegister(domainAccessPermissions[(*currentThreadContext)->pid]);
    enable_interrupts(level);

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
    kernel_memset(KERNEL_SRC_ADDR(0x050BD000), 0, 0x2F00);

    // Insert jump to the custom ioctl100 patch
    *(volatile uint32_t*)KERNEL_SRC_ADDR(0x05025242) = THUMB_BL(0x05025242, MCP_ioctl100_patch);

    // Insert jump to the kernel_syscall_0x81
    *(volatile uint32_t*)0x0812CD2C = ARM_B(0x0812CD2C, kernel_syscall_0x81);

    int32_t (*_iosMapSharedUserExecution)(void *descr) = (void *)0x08124F88;

    ios_map_shared_info_t map_info;
    map_info.paddr  = 0x050BD000 - 0x05000000 + 0x081C0000;
    map_info.vaddr  = 0x050BD000;
    map_info.size   = 0x3000;
    map_info.domain = 1; // MCP
    map_info.type   = 3; // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
    map_info.cached = 0xFFFFFFFF;
    _iosMapSharedUserExecution(&map_info); // actually a bss section but oh well it will have read/write

    map_info.paddr  = 0x05116000 - 0x05100000 + 0x13D80000;
    map_info.vaddr  = 0x05116000;
    map_info.size   = 0x4000;
    map_info.domain = 1; // MCP
    map_info.type   = 3; // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read write
    map_info.cached = 0xFFFFFFFF;
    _iosMapSharedUserExecution(&map_info);
}