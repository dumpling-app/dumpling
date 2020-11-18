#include <stdint.h>

typedef uint32_t (*odmCreateObject_t)(void *, int32_t, int32_t);
typedef void * (*odmDecrypt_t)(int32_t, void *, uint32_t, const void *, uint32_t, void *, uint32_t);
typedef void * (*odmMemcpy_t)(void *, const void *, uint32_t);
typedef void * (*odmMemset_t)(void *, int32_t, uint32_t);

odmCreateObject_t odmCreateObject = (odmCreateObject_t)0x107F20DC;
odmDecrypt_t odmDecrypt = (odmDecrypt_t)0x107F3FE0;
odmMemcpy_t odmMemcpy = (odmMemcpy_t)0x107F4F7C;
odmMemset_t odmMemset = (odmMemset_t)0x107F5018;