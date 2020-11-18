#include "users.h"
#include <locale>
#include <codecvt>

std::vector<userAccount> allUsers;

bool loadUsers() {
    WHBLogPrint("Loading users...");
    WHBLogConsoleDraw();

    nn::act::SlotNo currentAccount = nn::act::GetSlotNo();
    nn::act::SlotNo defaultAccount = nn::act::GetDefaultAccount();

    for (nn::act::SlotNo i=1; i<12; i++) {
        if (nn::act::IsSlotOccupied(i) == true) {
            userAccount newAccount;
            
            // Extract account info
            char16_t miiName[nn::act::MiiNameSize+1];
            nn::Result res = nn::act::GetMiiNameEx((int16_t *)miiName, i);
            if (res.IsFailure()) continue;

            // Convert mii name to normal string
            std::u16string newString(miiName);
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
            newAccount.miiName = convert.to_bytes(newString);

            newAccount.persistentId = nn::act::GetPersistentIdEx(i);
            newAccount.networkAccount = nn::act::IsNetworkAccountEx(i);
            newAccount.passwordCached = nn::act::IsPasswordCacheEnabledEx(i);
            newAccount.currAccount = currentAccount == i;
            newAccount.defaultAccount = defaultAccount == i;
            newAccount.slot = i;

            allUsers.emplace_back(newAccount);
        }
    }
    return !allUsers.empty();
}

userAccount* getUserByPersistentID(nn::act::PersistentId id) {
    for (auto& user : allUsers) {
        if (user.persistentId == id) return &user;
    }
    return nullptr;
}