#include "main.h"
#include "seeprom.h"
#include "../../ios_mcp/ios_mcp.h"
#include "../../ios_odm/ios_odm.h"
#include "../../ios_usb/ios_usb.h"

int32_t mainKernel() {
    // Flush whole D-cache
    flushDCache(0x081200F0, 0x4001);

    // Save state
    int32_t level = disableInterrupts();
    
    readSeeprom((uint16_t*)(0x01E10400+1024));
    flushDCache(0x01E10400+1024, 512);
    
    uint32_t controlRegister = disableMMU();

    // Save the request handle to reply later in the USB module
    *(uint32_t*)0x01E10000 = *(uint32_t*)0x1016AD18;

    // Patch kernel_error_handler to exit out immediately when called
    *(int32_t*)0x08129A24 = 0xE12FFF1E; // bx lr

    // Fix memory that the exploit had to corrupt while trying to load this file
    kernelMemcpy((void*)0x081298BC, repairData_setFaultBehavior, sizeof(repairData_setFaultBehavior));
    kernelMemcpy((void*)0x081296E4, repairData_setPanicBehavior, sizeof(repairData_setPanicBehavior));
    kernelMemcpy((void*)0x10100174, repairData_USBRootThread, sizeof(repairData_USBRootThread));

    readOTP(0, (void*)0x01E10400, 1024);
    flushDCache(0x01E10400, 1024);

    // Copy ios_mcp, ios_usb and ios_odm code so that it can be run
    kernelMemcpy((void*)0x101312D0, (void*)0x01E50000, sizeof(ios_usb_bin));
    kernelMemcpy((void*)0x107F81C4, (void*)ios_odm_bin, sizeof(ios_odm_bin));

    *(uint32_t*)KERNEL_RUN_ADDR(0x0510E56C) = 0x47700000; // bx lr
    kernelMemcpy(KERNEL_RUN_ADDR(0x0510E570), (void*)0x01E70020, *(uint32_t*)0x01E70000);

	// Replace ioctl 0x62 code with a jump to the custom MCP payload that'll run the iosuhax server IPC
    *(uint32_t*)KERNEL_SRC_ADDR(0x05026BA8) = 0x47780000; // bx pc
    *(uint32_t*)KERNEL_SRC_ADDR(0x05026BAC) = 0xE59F1000; // ldr r1, [pc]
    *(uint32_t*)KERNEL_SRC_ADDR(0x05026BB0) = 0xE12FFF11; // bx r1
    *(uint32_t*)KERNEL_SRC_ADDR(0x05026BB4) = 0x0510E570; // ios_mcp code

    // Inject a call to replace the read key function in the ODM payload
    *(volatile uint32_t*)(0x1073B310) = ARM_BL(0x1073B310, 0x107F81C4); // odmReadKey

    *(int32_t*)(0x01555500) = 0;

    // Restore state
    restoreMMU(controlRegister);
    enableInterrupts(level);

    // Flush whole D-cache and I-cache again
    invalidateDCache(0x081298BC, 0x4001);
    invalidateICache();

    return 0;
}
