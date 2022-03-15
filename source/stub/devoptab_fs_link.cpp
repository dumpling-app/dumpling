#include "devoptab_fs.h"

int32_t __wut_fs_link(struct _reent *r, const char *existing, const char *newLink) {
   r->_errno = ENOSYS;
   return -1;
}
