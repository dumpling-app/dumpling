#include "devoptab_fs.h"

int32_t __wut_fs_chmod(struct _reent* r, const char* path, mode_t mode) {
    FSStatus status;
    FSCmdBlock cmd;

    if (!path) {
        r->_errno = EINVAL;
        return -1;
    }

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return -1;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    char* fixedPath = __wut_fs_fixpath(r, path);
    if (!fixedPath) {
        return -1;
    }

    FSInitCmdBlock(&cmd);
    status = FSChangeMode(data->client, &cmd, fixedPath, __wut_fs_convert_mode(mode), (FSMode)0x777, FS_ERROR_FLAG_ALL);
    free(fixedPath);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    return 0;
}
