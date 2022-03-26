#include "common.h"

extern std::vector<userAccount> allUsers;

bool loadUsers();
userAccount* getUserByPersistentId(nn::act::PersistentId id);