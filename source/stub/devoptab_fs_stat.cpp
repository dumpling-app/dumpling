#include "devoptab_fs.h"

int32_t __wut_fs_stat(struct _reent* r, const char* path, struct stat* st) {
    int32_t fd;
    FSStatus status;
    FSCmdBlock cmd;

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return -1;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    if (!path || !st) {
        r->_errno = EINVAL;
        return -1;
    }

    char* fixedPath = __wut_fs_fixpath(r, path);
    if (!fixedPath) {
        return -1;
    }

    FSInitCmdBlock(&cmd);

    // First try open as file
    status = FSOpenFile(data->client, &cmd, fixedPath, "r", (FSFileHandle*)&fd, FS_ERROR_FLAG_ALL);
    if (status >= 0) {
        WHBLogPrint(fixedPath);
        WHBLogFreetypeDraw();
        __wut_fs_file_t tmpfd = { .fd = fd };
        status = (FSStatus)__wut_fs_fstat(r, &tmpfd, st);
        FSCloseFile(data->client, &cmd, fd, FS_ERROR_FLAG_ALL);
        free(fixedPath);
        return status;
    }

    // File failed, so lets try open as directory
    status = FSOpenDir(data->client, &cmd, fixedPath, (FSDirectoryHandle*)&fd, FS_ERROR_FLAG_ALL);
    free(fixedPath);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_nlink = 1;
    st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    FSCloseDir(data->client, &cmd, fd, FS_ERROR_FLAG_ALL);
    return 0;
}
