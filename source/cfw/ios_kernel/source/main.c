#include "main.h"
#include "patches.h"

#include "../../ios_mcp/ios_mcp.h"
#include "../../ios_mcp/ios_mcp_syms.h"
#include "../../ios_usb/ios_usb.h"

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
