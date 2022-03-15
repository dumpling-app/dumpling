#include "devoptab_fs.h"

int32_t __wut_fs_dirreset(struct _reent* r, DIR_ITER* dirState) {
    FSStatus status;
    FSCmdBlock cmd;
    __wut_fs_dir_t* dir;

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return -1;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    if (!dirState) {
        r->_errno = EINVAL;
        return -1;
    }

    FSInitCmdBlock(&cmd);
    dir = (__wut_fs_dir_t*)(dirState->dirStruct);
    status = FSRewindDir(data->client, &cmd, dir->fd, FS_ERROR_FLAG_ALL);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    return 0;
}
