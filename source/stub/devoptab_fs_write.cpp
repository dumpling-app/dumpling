#include "devoptab_fs.h"

ssize_t __wut_fs_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    FSStatus status = FS_STATUS_OK;
    FSCmdBlock cmd;
    uint8_t* alignedWriteBuffer;
    uint32_t bytes, bytesWritten;
    __wut_fs_file_t* file;

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return -1;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    if (!fd || !ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    FSInitCmdBlock(&cmd);
    file = (__wut_fs_file_t*)fd;
    bytesWritten = 0;

    // Check that the file was opened with write access
    if ((file->flags & O_ACCMODE) == O_RDONLY) {
        r->_errno = EBADF;
        return -1;
    }

    if ((((uintptr_t)ptr) & 0x3F) == 0) {
        status = FSWriteFile(data->client, &cmd, (uint8_t*)ptr, 1, len, file->fd, 0, FS_ERROR_FLAG_ALL);
        if (status > 0) {
            bytesWritten = (uint32_t)status;
            file->offset += bytesWritten;
        }
    }
    else {
        // Copy to internal buffer due to alignment requirement and read in chunks.
        // Using a buffer smaller than 128KiB takes a performance hit.
        size_t buffer_size = len < 128 * 1024 ? len : 128 * 1024;
        alignedWriteBuffer = (uint8_t*)memalign(0x40, buffer_size);
        if (!alignedWriteBuffer) {
            r->_errno = ENOMEM;
            return -1;
        }
        while (len > 0) {
            size_t toWrite = len > buffer_size ? buffer_size : len;

            // Copy to internal buffer
            memcpy(alignedWriteBuffer, ptr, toWrite);

            // Write the data
            status = FSWriteFile(data->client, &cmd, alignedWriteBuffer, 1, toWrite, file->fd, 0, FS_ERROR_FLAG_ALL);
            if (status <= 0) {
                break;
            }

            bytes = (uint32_t)status;
            file->offset += bytes;
            bytesWritten += bytes;
            ptr += bytes;
            len -= bytes;

            if (bytes < toWrite) {
                break;
            }
        }
        free(alignedWriteBuffer);
    }

    // Return partial write
    if (bytesWritten > 0) {
        return bytesWritten;
    }

    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    return 0;
}
