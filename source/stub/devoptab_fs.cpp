#include "devoptab_fs.h"

static devoptab_t __wut_mlc_devoptab = {
    .name = NULL,
    .structSize = sizeof(__wut_fs_file_t),
    .open_r = __wut_fs_open,
    .close_r = __wut_fs_close,
    .write_r = __wut_fs_write,
    .read_r = __wut_fs_read,
    .seek_r = __wut_fs_seek,
    .fstat_r = __wut_fs_fstat,
    .stat_r = __wut_fs_stat,
    .link_r = __wut_fs_link,
    .unlink_r = __wut_fs_unlink,
    .chdir_r = __wut_fs_chdir,
    .rename_r = __wut_fs_rename,
    .mkdir_r = __wut_fs_mkdir,
    .dirStateSize = sizeof(__wut_fs_dir_t),
    .diropen_r = __wut_fs_diropen,
    .dirreset_r = __wut_fs_dirreset,
    .dirnext_r = __wut_fs_dirnext,
    .dirclose_r = __wut_fs_dirclose,
    .statvfs_r = __wut_fs_statvfs,
    .ftruncate_r = __wut_fs_ftruncate,
    .fsync_r = __wut_fs_fsync,
    .deviceData = NULL,
    .chmod_r = __wut_fs_chmod,
    .fchmod_r = __wut_fs_fchmod,
    .rmdir_r = __wut_fs_rmdir,
};

int mount_fs(const char *virt_name, int fsaFd, const char *dev_path, const char *mount_path) {
    FSStatus rc = FS_STATUS_OK;

    if (std::string(virt_name) != "storage_mlc01" || std::string(mount_path) != "/vol/storage_mlc01") {
        return FS_STATUS_ACCESS_ERROR;
    }

    if (__wut_mlc_devoptab.deviceData != NULL && ((devoptab_data_t *)__wut_mlc_devoptab.deviceData)->initialized) {
        return rc;
    }

    devoptab_data_t *data = (devoptab_data_t *)memalign(0x20, sizeof(devoptab_data_t));
    data->client = (FSClient *)memalign(0x20, sizeof(FSClient));
    data->mount_path = mount_path;
    data->virt_path = virt_name;
    __wut_mlc_devoptab.name = virt_name;
    __wut_mlc_devoptab.deviceData = data;

    FSCmdBlock fsCmd;
    FSMountSource mountSource;
    // char mountPath[0x80];
    // char workDir[0x83];

    FSInit();
    rc = FSAddClient(data->client, FS_ERROR_FLAG_ALL);

    if (rc < 0) {
        free(data->client);
        free(data);
        return rc;
    }

    FSInitCmdBlock(&fsCmd);

    if (rc >= 0) {
        int dev = AddDevice(&__wut_mlc_devoptab);

        if (dev != -1) {
            data->initialized = true;
        }
        else {
            FSDelClient(data->client, FS_ERROR_FLAG_ALL);
            free(data->client);
            free(data);
            return dev;
        }
    }

    return rc;
}

int unmount_fs(const char *virt_name) {
    FSStatus rc = FS_STATUS_OK;

    if (__wut_mlc_devoptab.deviceData == NULL || ((devoptab_data_t *)__wut_mlc_devoptab.deviceData)->initialized) {
        return rc;
    }
    devoptab_data_t *data = ((devoptab_data_t *)__wut_mlc_devoptab.deviceData);

    FSDelClient(data->client, FS_ERROR_FLAG_ALL);
    free(data->client);
    free(data);

    RemoveDevice(__wut_mlc_devoptab.name);

    data->client = NULL;
    data->initialized = false;

    return rc;
}
