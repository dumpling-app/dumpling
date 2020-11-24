#define IOS_FS_IOSC_CREATEOBJ ((int (*)(void*, int, int))0x107F20DC)
#define IOS_FS_IOSC_DECRYPT ((void (*)(int, void*, unsigned int, const void*, unsigned int, void*, unsigned int))0x107F3FE0)
#define IOS_FS_MEMCPY ((void* (*)(void*, const void*, unsigned int))0x107F4F7C)
#define IOS_FS_MEMSET ((void* (*)(void*, int, unsigned int))0x107F5018)

int odmReadKey(void *obj, void *key, unsigned int base) {
	if(*(volatile int*)(base + 0x409AC) == 0)
	{
		char iv[0x10];
		IOS_FS_MEMSET(iv, 0, 0x10);
		//session key fd, used to decrypt drive data
		int fd = *(volatile int*)(base + 0x409CC);
		//decrypt disc key with the session key
		IOS_FS_IOSC_DECRYPT(fd, iv, 0x10, key, 0x10, (void*)0x01E10C00, 0x10);
	}
	else
	{
		//no session key decrypt needed
		IOS_FS_MEMCPY((void*)0x01E10C00, key, 0x10);
	}
	//original code
	return IOS_FS_IOSC_CREATEOBJ(obj, 0, 0);
}