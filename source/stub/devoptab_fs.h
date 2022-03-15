#include "../app/common.h"
#include "../font/log_freetype.h"

#include <coreinit/filesystem.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

/**
 * Open file struct
 */
typedef struct {
    //! FS handle
    FSFileHandle fd;

    //! Flags used in open(2)
    int flags;

    //! Current file offset
    uint32_t offset;
} __wut_fs_file_t;


/**
 * Open directory struct
 */
typedef struct {
    //! Should be set to FS_DIRITER_MAGIC
    uint32_t magic;

    //! FS handle
    FSDirectoryHandle fd;

    //! Temporary storage for reading entries
    FSDirectoryEntry entry_data;
} __wut_fs_dir_t;

typedef struct devoptab_data {
    bool initialized;
    FSClient* client;
    const char* mount_path;
    const char* virt_path;
} devoptab_data_t;

extern int mount_fs(const char* virt_name, int fsaFd, const char* dev_path, const char* mount_path);
extern int unmount_fs(const char* virt_name);

#define FS_DIRITER_MAGIC 0x77696975

int32_t     __wut_fs_open(struct _reent* r, void* fileStruct, const char* path, int32_t flags, int32_t mode);
int32_t     __wut_fs_close(struct _reent* r, void* fd);
ssize_t     __wut_fs_write(struct _reent* r, void* fd, const char* ptr, size_t len);
ssize_t     __wut_fs_read(struct _reent* r, void* fd, char* ptr, size_t len);
off_t       __wut_fs_seek(struct _reent* r, void* fd, off_t pos, int32_t dir);
int32_t     __wut_fs_fstat(struct _reent* r, void* fd, struct stat* st);
int32_t     __wut_fs_stat(struct _reent* r, const char* file, struct stat* st);
int32_t     __wut_fs_link(struct _reent* r, const char* existing, const char* newLink);
int32_t     __wut_fs_unlink(struct _reent* r, const char* name);
int32_t     __wut_fs_chdir(struct _reent* r, const char* name);
int32_t     __wut_fs_rename(struct _reent* r, const char* oldName, const char* newName);
int32_t     __wut_fs_mkdir(struct _reent* r, const char* path, int32_t mode);
DIR_ITER*   __wut_fs_diropen(struct _reent* r, DIR_ITER* dirState, const char* path);
int32_t     __wut_fs_dirreset(struct _reent* r, DIR_ITER* dirState);
int32_t     __wut_fs_dirnext(struct _reent* r, DIR_ITER* dirState, char* filename, struct stat* filestat);
int32_t     __wut_fs_dirclose(struct _reent* r, DIR_ITER* dirState);
int32_t     __wut_fs_statvfs(struct _reent* r, const char* path, struct statvfs* buf);
int32_t     __wut_fs_ftruncate(struct _reent* r, void* fd, off_t len);
int32_t     __wut_fs_fsync(struct _reent* r, void* fd);
int32_t     __wut_fs_chmod(struct _reent* r, const char* path, mode_t mode);
int32_t     __wut_fs_fchmod(struct _reent* r, void* fd, mode_t mode);
int32_t     __wut_fs_rmdir(struct _reent* r, const char* name);

// devoptab_fs_utils.c
char* __wut_fs_fixpath(struct _reent* r, const char* path);
int32_t __wut_fs_translate_error(FSStatus error);
