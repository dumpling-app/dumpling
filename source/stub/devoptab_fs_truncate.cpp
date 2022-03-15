#include "devoptab_fs.h"

int32_t __wut_fs_ftruncate(struct _reent* r, void* fd, off_t len) {
    FSStatus status;
    FSCmdBlock cmd;
    __wut_fs_file_t* file;

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return -1;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    // Make sure length is non-negative
    if (!fd || len < 0) {
        r->_errno = EINVAL;
        return -1;
    }

    // Set the new file size
    FSInitCmdBlock(&cmd);
    file = (__wut_fs_file_t*)fd;
    status = FSSetPosFile(data->client, &cmd, file->fd, len, FS_ERROR_FLAG_ALL);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    status = FSTruncateFile(data->client, &cmd, file->fd, FS_ERROR_FLAG_ALL);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    return 0;
}
