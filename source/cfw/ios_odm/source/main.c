#include "main.h"

int odmReadKey(void *obj, void *key, uint32_t base) {
	if (*(volatile int32_t*)(base + 0x409AC) == 0) {
        // Create new empty buffer to hold key
		uint8_t keyBuffer[0x10];
		odmMemset(keyBuffer, 0, 0x10);

		// Get the session key
		int32_t sessionKeyHandle = *(volatile int32_t*)(base + 0x409CC);

		// Decrypt the disc key with the session key
		odmDecrypt(sessionKeyHandle, keyBuffer, 0x10, key, 0x10, (void*)0x1E10C00, 0x10);
	}
	else {
		// No session key that encrypted the disc key, so just copy 
		odmMemcpy((void*)0x1E10C00, key, 0x10);
	}
	//original code
	return odmCreateObject(obj, 0, 0);
}