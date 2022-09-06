#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <malloc.h>

void memoryInitialize(void);
void memoryRelease(void);

void* MEM2_alloc(unsigned int size, unsigned int align);
void MEM2_free(void *ptr);

void* MEM1_alloc(unsigned int size, unsigned int align);
void MEM1_free(void *ptr);

void* MEMBucket_alloc(unsigned int size, unsigned int align);
void MEMBucket_free(void *ptr);

#ifdef __cplusplus
}
#endif