#include "common.h"

extern std::vector<userAccount> allUsers;

bool loadUsers();
userAccount* getUserByPersistentID(nn::act::PersistentId id);