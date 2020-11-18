#include <stdint.h>

typedef void (*IOSShutdown_t)(int32_t);
typedef int32_t (*IOSReply_t)(int32_t, int32_t);

IOSShutdown_t IOSShutdown = (IOSShutdown_t)0x1012EE4C;
IOSReply_t IOSReply = (IOSReply_t)0x1012ED04;

void restoreHandle();
