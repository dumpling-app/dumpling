#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ARM_B(addr, func)   (0xEA000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))
#define ARM_BL(addr, func)  (0xEB000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))
#define THUMB_B(addr, func)     ((0xE000 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x7FF)))
#define THUMB_BL(addr, func)    ((0xF000F800 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x0FFF)) | ((((uint32_t)(func) - (uint32_t)(addr) - 4) << 4) & 0x7FFF000))


#define KERNEL_RUN_ADDR(addr) (void*)(addr - 0x05100000 + 0x13D80000)
#define KERNEL_SRC_ADDR(addr) (void*)(addr - 0x05000000 + 0x081C0000)

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