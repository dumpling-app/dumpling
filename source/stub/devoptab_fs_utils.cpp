#include "devoptab_fs.h"

char* __wut_fs_fixpath(struct _reent* r, const char* path) {
    // Sanity check
    if (!path)
        return NULL;

    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    // Move the path pointer to the start of the actual path
    if (strchr(path, ':') != NULL) {
        path = strchr(path, ':') + 1;
    }

    int mount_len = strlen(data->mount_path);

    char* new_name = (char*)malloc(mount_len + strlen(path) + 1);
    if (new_name) {
        strcpy(new_name, data->mount_path);
        strcpy(new_name + mount_len, path);
        return new_name;
    }
    return new_name;
}

int32_t __wut_fs_translate_error(FSStatus error) {
    switch ((int32_t)error) {
        case FS_STATUS_END:
            return ENOENT;
        case FS_STATUS_CANCELLED:
            return EINVAL;
        case FS_STATUS_EXISTS:
            return EEXIST;
        case FS_STATUS_MEDIA_ERROR:
            return EIO;
        case FS_STATUS_NOT_FOUND:
            return ENOENT;
        case FS_STATUS_PERMISSION_ERROR:
            return EPERM;
        case FS_STATUS_STORAGE_FULL:
            return ENOSPC;
        case FS_ERROR_ALREADY_EXISTS:
            return EEXIST;
        case FS_ERROR_BUSY:
            return EBUSY;
        case FS_ERROR_CANCELLED:
            return ECANCELED;
        case FS_STATUS_FILE_TOO_BIG:
            return EFBIG;
        case FS_ERROR_INVALID_PATH:
            return ENAMETOOLONG;
        case FS_ERROR_NOT_DIR:
            return ENOTDIR;
        case FS_ERROR_NOT_FILE:
            return EISDIR;
        case FS_ERROR_OUT_OF_RANGE:
            return ESPIPE;
        case FS_ERROR_UNSUPPORTED_COMMAND:
            return ENOTSUP;
        case FS_ERROR_WRITE_PROTECTED:
            return EROFS;
        default:
            return (int32_t)error;
    }
}
