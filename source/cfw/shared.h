#pragma once

#define KERNEL_READ32         1
#define KERNEL_WRITE32        2
#define KERNEL_MEMCPY         3
#define KERNEL_GET_CFW_CONFIG 4
#define KERNEL_READ_OTP       5

#define IOSUHAX_MAGIC_WORD           0x4E696365
#define IOS_ERROR_UNKNOWN_VALUE      0xFFFFFFD6
#define IOS_ERROR_INVALID_ARG        0xFFFFFFE3
#define IOS_ERROR_INVALID_SIZE       0xFFFFFFE9
#define IOS_ERROR_UNKNOWN            0xFFFFFFF7
#define IOS_ERROR_NOEXISTS           0xFFFFFFFA

#define IOS_COMMAND_INVALID          0x00
#define IOS_OPEN                     0x01
#define IOS_CLOSE                    0x02
#define IOS_READ                     0x03
#define IOS_WRITE                    0x04
#define IOS_SEEK                     0x05
#define IOS_IOCTL                    0x06
#define IOS_IOCTLV                   0x07
#define IOS_REPLY                    0x08
#define IOS_IPC_MSG0                 0x09
#define IOS_IPC_MSG1                 0x0A
#define IOS_IPC_MSG2                 0x0B
#define IOS_SUSPEND                  0x0C
#define IOS_RESUME                   0x0D
#define IOS_SVCMSG                   0x0E

#define IOCTL_MEM_WRITE              0x00
#define IOCTL_MEM_READ               0x01
#define IOCTL_SVC                    0x02
#define IOCTL_KILL_SERVER            0x03
#define IOCTL_MEMCPY                 0x04
#define IOCTL_REPEATED_WRITE         0x05
#define IOCTL_KERN_READ32            0x06
#define IOCTL_KERN_WRITE32           0x07
#define IOCTL_READ_OTP               0x08

#define IOCTL_FSA_OPEN               0x40
#define IOCTL_FSA_CLOSE              0x41
#define IOCTL_FSA_MOUNT              0x42
#define IOCTL_FSA_UNMOUNT            0x43
#define IOCTL_FSA_GETINFO            0x44
#define IOCTL_FSA_OPENDIR            0x45
#define IOCTL_FSA_READDIR            0x46
#define IOCTL_FSA_CLOSEDIR           0x47
#define IOCTL_FSA_MAKEDIR            0x48
#define IOCTL_FSA_OPENFILE           0x49
#define IOCTL_FSA_READFILE           0x4A
#define IOCTL_FSA_WRITEFILE          0x4B
#define IOCTL_FSA_GETSTATFILE        0x4C
#define IOCTL_FSA_CLOSEFILE          0x4D
#define IOCTL_FSA_SETPOSFILE         0x4E
#define IOCTL_FSA_GETSTAT            0x4F
#define IOCTL_FSA_REMOVE             0x50
#define IOCTL_FSA_REWINDDIR          0x51
#define IOCTL_FSA_CHDIR              0x52
#define IOCTL_FSA_RENAME             0x53
#define IOCTL_FSA_RAW_OPEN           0x54
#define IOCTL_FSA_RAW_READ           0x55
#define IOCTL_FSA_RAW_WRITE          0x56
#define IOCTL_FSA_RAW_CLOSE          0x57
#define IOCTL_FSA_CHANGEMODE         0x58
#define IOCTL_FSA_FLUSHVOLUME        0x59
#define IOCTL_CHECK_IF_IOSUHAX       0x5B

// Extended mode
#define IOCTL_FSA_CHANGEOWNER        0x5C
#define IOCTL_FSA_OPENFILEEX         0x5D
#define IOCTL_FSA_READFILEWITHPOS    0x5E
#define IOCTL_FSA_WRITEFILEWITHPOS   0x5F
#define IOCTL_FSA_APPENDFILE         0x60
#define IOCTL_FSA_APPENDFILEEX       0x61
#define IOCTL_FSA_FLUSHFILE          0x62
#define IOCTL_FSA_TRUNCATEFILE       0x63
#define IOCTL_FSA_GETPOSFILE         0x64
#define IOCTL_FSA_ISEOF              0x65
#define IOCTL_FSA_ROLLBACKVOLUME     0x66
#define IOCTL_FSA_GETCWD             0x67
#define IOCTL_FSA_MAKEQUOTA          0x68
#define IOCTL_FSA_FLUSHQUOTA         0x69
#define IOCTL_FSA_ROLLBACKQUOTA      0x6A
#define IOCTL_FSA_ROLLBACKQUOTAFORCE 0x6B
#define IOCTL_FSA_CHANGEMODEEX       0x6C

// Old bindings that are now renamed
#define IOCTL_FSA_GETDEVICEINFO      IOCTL_FSA_GETINFO
#define IOCTL_FSA_SETFILEPOS         IOCTL_FSA_SETPOSFILE