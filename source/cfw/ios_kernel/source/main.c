#include "main.h"
#include "patches.h"

#include "../../ios_mcp/ios_mcp.h"
#include "../../ios_mcp/ios_mcp_syms.h"
#include "../../ios_usb/ios_usb.h"

static const uint8_t repairData_setFaultBehavior[] = {
    0xE1,0x2F,0xFF,0x1E,0xE9,0x2D,0x40,0x30,0xE5,0x93,0x20,0x00,0xE1,0xA0,0x40,0x00,
    0xE5,0x92,0x30,0x54,0xE1,0xA0,0x50,0x01,0xE3,0x53,0x00,0x01,0x0A,0x00,0x00,0x02,
    0xE1,0x53,0x00,0x00,0xE3,0xE0,0x00,0x00,0x18,0xBD,0x80,0x30,0xE3,0x54,0x00,0x0D,
};
static const uint8_t repairData_setPanicBehavior[] = {
    0x08,0x16,0x6C,0x00,0x00,0x00,0x18,0x0C,0x08,0x14,0x40,0x00,0x00,0x00,0x9D,0x70,
    0x08,0x16,0x84,0x0C,0x00,0x00,0xB4,0x0C,0x00,0x00,0x01,0x01,0x08,0x14,0x40,0x00,
    0x08,0x15,0x00,0x00,0x08,0x17,0x21,0x80,0x08,0x17,0x38,0x00,0x08,0x14,0x30,0xD4,
    0x08,0x14,0x12,0x50,0x08,0x14,0x12,0x94,0xE3,0xA0,0x35,0x36,0xE5,0x93,0x21,0x94,
    0xE3,0xC2,0x2E,0x21,0xE5,0x83,0x21,0x94,0xE5,0x93,0x11,0x94,0xE1,0x2F,0xFF,0x1E,
    0xE5,0x9F,0x30,0x1C,0xE5,0x9F,0xC0,0x1C,0xE5,0x93,0x20,0x00,0xE1,0xA0,0x10,0x00,
    0xE5,0x92,0x30,0x54,0xE5,0x9C,0x00,0x00,
};
static const uint8_t repairData_USBRootThread[] = {
    0xE5,0x8D,0xE0,0x04,0xE5,0x8D,0xC0,0x08,0xE5,0x8D,0x40,0x0C,0xE5,0x8D,0x60,0x10,
    0xEB,0x00,0xB2,0xFD,0xEA,0xFF,0xFF,0xC9,0x10,0x14,0x03,0xF8,0x10,0x62,0x4D,0xD3,
    0x10,0x14,0x50,0x00,0x10,0x14,0x50,0x20,0x10,0x14,0x00,0x00,0x10,0x14,0x00,0x90,
    0x10,0x14,0x00,0x70,0x10,0x14,0x00,0x98,0x10,0x14,0x00,0x84,0x10,0x14,0x03,0xE8,
    0x10,0x14,0x00,0x3C,0x00,0x00,0x01,0x73,0x00,0x00,0x01,0x76,0xE9,0x2D,0x4F,0xF0,
    0xE2,0x4D,0xDE,0x17,0xEB,0x00,0xB9,0x92,0xE3,0xA0,0x10,0x00,0xE3,0xA0,0x20,0x03,
    0xE5,0x9F,0x0E,0x68,0xEB,0x00,0xB3,0x20,
};

int32_t mainKernel() {
    // Flush whole D-cache
    flushDCache(0x081200F0, 0x4001);

    // Save state
    int32_t level = disableInterrupts();
    
    uint32_t controlRegister = disableMMU();

    // Save the request handle to reply later in the USB module
    *(uint32_t*)0x01E10000 = *(uint32_t*)0x1016AD18;

    // Patch kernel_error_handler to exit out immediately when called
    *(int32_t*)0x08129A24 = 0xE12FFF1E; // bx lr

    // Fix memory that the exploit had to corrupt while trying to load this file
    kernelMemcpy((void*)0x081298BC, repairData_setFaultBehavior, sizeof(repairData_setFaultBehavior));
    kernelMemcpy((void*)0x081296E4, repairData_setPanicBehavior, sizeof(repairData_setPanicBehavior));
    kernelMemcpy((void*)0x10100174, repairData_USBRootThread, sizeof(repairData_USBRootThread));

    // Copy ios_mcp and ios_usb so that they can be run
    kernelMemcpy((void*)0x101312D0, (void*)0x01E50000, sizeof(ios_usb_bin));
    kernelMemcpy(KERNEL_RUN_ADDR(_mcp_start), (void*)0x01E70020, *(uint32_t*)0x01E70000);

    installPatches();

    *(int32_t*)(0x01555500) = 0;

    // Restore state
    restoreMMU(controlRegister);
    enableInterrupts(level);

    // Flush whole D-cache and I-cache again
    invalidateDCache(0x081298BC, 0x4001);
    invalidateICache();

    return 0;
}
