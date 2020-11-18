#include "main.h"

void restoreHandle() {
	uint32_t savedHandle = *(volatile uint32_t*)0x01E10000;
	if (IOSReply(savedHandle, 0) != 0)
		IOSShutdown(1);

	// stack pointer will be 0x1016AE30
	// link register will be 0x1012EACC
	asm("LDR SP, newsp\n"
		"LDR R0, newr0\n"
		"LDR LR, newlr\n"
		"LDR PC, newpc\n"
		"newsp: .word 0x1016AE30\n"
		"newlr: .word 0x1012EACC\n"
		"newr0: .word 0x10146080\n"
		"newpc: .word 0x10111164\n");
}