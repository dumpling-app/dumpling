#include "devoptab_fs.h"

int __wut_fs_chdir(struct _reent *r, const char *path)
{
   FSStatus status;
   FSCmdBlock cmd;

   if (!path) {
      r->_errno = EINVAL;
      return -1;
   }

   if (r->deviceData == NULL || !((devoptab_data_t *)r->deviceData)->initialized) {
      r->_errno = EINVAL;
      return -1;
   }
   devoptab_data_t *data = ((devoptab_data_t *)r->deviceData);

   char *fixedPath = __wut_fs_fixpath(r, path);
   if (!fixedPath) {
      return -1;
   }

   FSInitCmdBlock(&cmd);
   status = FSChangeDir(data->client, &cmd, fixedPath, FS_ERROR_FLAG_ALL);
   free(fixedPath);
   if (status < 0) {
      r->_errno = __wut_fs_translate_error(status);
      return -1;
   }

   return 0;
}
