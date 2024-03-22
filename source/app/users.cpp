#include "users.h"
#include "gui.h"

std::vector<userAccount> allUsers;

bool loadUsers() {
    WHBLogPrint("Loading users...");
    WHBLogFreetypeDraw();

    nn::act::SlotNo currentAccount = nn::act::GetSlotNo();
    nn::act::SlotNo defaultAccount = nn::act::GetDefaultAccount();

    for (nn::act::SlotNo i = 1; i < 13; i++) {
        if (nn::act::IsSlotOccupied(i) == true) {
            userAccount newAccount;

            // Extract account info
            char16_t miiName[nn::act::MiiNameSize + 1];
            nn::Result res = nn::act::GetMiiNameEx((int16_t*)miiName, i);
            if (res.IsFailure()) continue;

            if (nn::act::IsNetworkAccountEx(i)) {
                char accountId[nn::act::AccountIdSize + 1];
                res = nn::act::GetAccountIdEx(accountId, i);
                if (res.IsFailure()) continue;
                newAccount.accountId = accountId;
            }

            // Convert mii name to normal string
            std::wstring_convert<std::codecvt_utf16<wchar_t>> convert;
            newAccount.miiName = convert.from_bytes(reinterpret_cast<const char*>(miiName), reinterpret_cast<const char*>(miiName + std::char_traits<char16_t>::length(miiName)));

            // Convert persistent id into string
            std::ostringstream stream;
            stream << std::hex << nn::act::GetPersistentIdEx(i);
            newAccount.persistentIdString = stream.str();

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

userAccount* getUserByPersistentId(nn::act::PersistentId id) {
    for (auto& user : allUsers) {
        if (user.persistentId == id) return &user;
    }
    return nullptr;
}
