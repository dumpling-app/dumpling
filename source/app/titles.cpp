#include "titles.h"
#include "filesystem.h"
#include "users.h"
#include "gui.h"

std::vector<titleEntry> installedTitles = {};
std::vector<MCPTitleListType> unparsedTitleList = {};

bool readInfoFromXML(titleEntry& title, titlePart& part) {
    // Check if /meta/meta.xml file exists
    std::string metaPath(part.path);
    metaPath.append("/meta/meta.xml");
    if (!fileExist(metaPath.c_str())) {
        WHBLogPrint("The meta.xml file doesn't seem to exist at the given path:");
        WHBLogPrint(metaPath.c_str());
        return false;
    }

    // Read meta.xml
    std::ifstream xmlFile(metaPath);
    std::string line;

    // Parse data from it using string matching
    bool foundShortTitle = !title.shortTitle.empty();
    bool foundProductCode = !title.productCode.empty();
    while(getline(xmlFile, line)) {
        if (!foundShortTitle && line.find("shortname_en") != std::string::npos) {
            title.shortTitle = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            title.normalizedTitle = normalizeTitle(title.shortTitle);
            
            // Create generic names for games that have non-standard characters (like kanji)
            if (title.normalizedTitle.empty() && !title.productCode.empty()) {
                title.shortTitle = std::string("Undisplayable Game Name [") + title.productCode + "]";
                title.normalizedTitle = title.productCode;
            }
            else foundShortTitle = true;
        }
        else if (!foundProductCode && line.find("product_code") != std::string::npos) {
            title.productCode = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            foundProductCode = true;
        }
        if (foundShortTitle && foundProductCode) {
            // Finish up information
            part.outputPath += title.normalizedTitle;
            return true;
        }
    }

    WHBLogPrintf("Could only partially parse %lu information bits:");
    WHBLogPrintf("shortname_en = %s", title.shortTitle.c_str());
    WHBLogPrintf("product_code = %s", title.productCode.c_str());
    WHBLogConsoleDraw();
    return false;
}

bool getSaves(std::string savesPath, std::vector<titleSave>& saves, titleSaveCommon& common) {
    // Open folder
    DIR* dirHandle;
    if ((dirHandle = opendir(savesPath.c_str())) == NULL) {
        //WHBLogPrint("Can't open the folder to read the saves from the following path:");
        //WHBLogPrint(savesPath.c_str());
        //WHBLogConsoleDraw();
        //OSSleepTicks(OSMillisecondsToTicks(500));
        return false;
    }

    struct dirent *dirEntry;
    while((dirEntry = readdir(dirHandle)) != NULL) {
        // Check if there's a common folder
        if (dirEntry->d_type == DT_DIR) {
            // Ignore root and parent folder
            if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) continue;
            
            // Don't save the directory is it's empty
            std::string path = savesPath + "/" + dirEntry->d_name;
            if (isDirEmpty(path.c_str())) continue;

            if (strcmp(dirEntry->d_name, "common") == 0) {
                common.path = path;
            }
            else if (strlen(dirEntry->d_name) == sizeof("80000001")-1) {
                // Extract information about current user
                nn::act::PersistentId saveID = std::stoul(std::string(dirEntry->d_name), 0, 16);
                userAccount* userAccount = getUserByPersistentID(saveID);
                if (userAccount == nullptr) {
                    WHBLogPrint("Couldn't find the user by the persistence ID:");
                    WHBLogPrintf("saveID: %s", dirEntry->d_name);
                    WHBLogPrint(path.c_str());
                    WHBLogConsoleDraw();
                    //OSSleepTicks(OSMillisecondsToTicks(5000));
                    continue;
                }

                saves.emplace_back(titleSave{
                    .account = userAccount,
                    .path = path
                });
            }
        }
    }

    closedir(dirHandle);
    return true;
}

bool getTitles() {
    WHBLogPrint("Loading titles...");
    WHBLogConsoleDraw();
    
    // Clear unparsed titles
    unparsedTitleList.clear();

    // Open MCP tunnel
    int32_t mcpHandle = MCP_Open();
    if (mcpHandle < 0) {
        WHBLogPrint("Failed to open MPC to load all the games and updates");
        return false;
    }

    // Get titles from MCP
    int32_t titleCount = MCP_TitleCount(mcpHandle);
    uint32_t titleByteSize = titleCount * sizeof(struct MCPTitleListType);

    unparsedTitleList.resize(titleCount);

    uint32_t titlesListed;
    MCP_TitleList(mcpHandle, &titlesListed, unparsedTitleList.data(), titleByteSize);

    // Close MCP
    MCP_Close(mcpHandle);
    return true;
}

bool loadTitles(bool skipDiscs) {
    WHBLogPrint("Searching for games...");
    WHBLogConsoleDraw();

    // Delete previous titles
    installedTitles.clear();

    // Queue and group parts of each title
    std::map<uint32_t, std::vector<std::reference_wrapper<MCPTitleListType>>> sortedQueue;
    for (auto& title : unparsedTitleList) {
        // Skip discs whenever there's an initial scan
        if (skipDiscs && deviceToLocation(title.indexedDevice) == titleLocation::Disc) continue;

        // Check if it's a supported app type
        if (isBase(title.appType) || isUpdate(title.appType) || isDLC(title.appType) || isSystemApp(title.appType)) {
            std::string posixPath = convertToPosixPath(title.path);
            if (!posixPath.empty() && dirExist(posixPath.c_str())) {
                uint32_t gameId = (uint32_t)title.titleId & 0x00000000FFFFFFFF;
                sortedQueue[gameId].emplace_back(std::ref(title));
            }
            else {
                WHBLogPrint("Couldn't convert the path or find this folder:");
                WHBLogPrint(posixPath.c_str());
                WHBLogConsoleDraw();
                OSSleepTicks(OSSecondsToTicks(2));
            }
        }
    }

    // Parse meta files to get name, title etc.
    for (auto& sortedTitle : sortedQueue) {
        installedTitles.emplace_back(titleEntry{});
        titleEntry& title = installedTitles.back();
        title.titleLowID = sortedTitle.first;
        title.base = titlePart{};
        title.update = titlePart{};
        title.dlc = titlePart{};

        // Loop over each part of a title
        for (MCPTitleListType& part : sortedTitle.second) {
            if (isBase(part.appType) || isSystemApp(part.appType)) {
                title.base.path = convertToPosixPath(part.path);
                title.base.version = part.titleVersion;
                title.base.type = part.appType;
                title.base.partHighID = (uint32_t)((part.titleId & 0xFFFFFFFF00000000) >> 32);
                title.base.location = deviceToLocation(part.indexedDevice);
                
                // Change the output path
                if (isBase(part.appType)) title.base.outputPath = "/Games/";
                if (isSystemApp(part.appType)) title.base.outputPath = "/System Applications/";
                
                if (readInfoFromXML(title, title.base)) title.hasBase = true;
                else {
                    WHBLogPrint("Failed to read meta from game!");
                    WHBLogConsoleDraw();
                    OSSleepTicks(OSSecondsToTicks(10));
                }
            }
            else if (isUpdate(part.appType)) { // TODO: Log cases where maybe two updates are found (one on disc and one later via the title system)
                title.update.path = convertToPosixPath(part.path);
                title.update.version = part.titleVersion;
                title.update.type = part.appType;
                title.update.partHighID = (uint32_t)((part.titleId & 0xFFFFFFFF00000000) >> 32);
                title.update.location = deviceToLocation(part.indexedDevice);
                title.update.outputPath = "/Updates/";
                if (readInfoFromXML(title, title.update)) title.hasUpdate = true;
                else {
                    WHBLogPrint("Failed to read meta from update!");
                    WHBLogConsoleDraw();
                    OSSleepTicks(OSSecondsToTicks(10));
                }
            }
            else if (isDLC(part.appType)) {
                title.dlc.path = convertToPosixPath(part.path);
                title.dlc.version = part.titleVersion;
                title.dlc.type = part.appType;
                title.dlc.partHighID = (uint32_t)((part.titleId & 0xFFFFFFFF00000000) >> 32);
                title.dlc.location = deviceToLocation(part.indexedDevice);
                title.dlc.outputPath = "/DLCs/";
                if (readInfoFromXML(title, title.dlc)) title.hasDLC = true;
                else {
                    WHBLogPrint("Failed to read meta from dlc!");
                    WHBLogConsoleDraw();
                    OSSleepTicks(OSSecondsToTicks(10));
                }
            }
        }

        // Check for save files
        if (title.hasBase || title.hasUpdate || title.hasDLC) {
            // Check if path would support save file
            std::ostringstream savePath;
            savePath << "/usr/save/";
            savePath << std::nouppercase << std::right << std::setw(8) << std::setfill('0') << std::hex << title.base.partHighID;
            savePath << "/";
            savePath << std::nouppercase << std::right << std::setw(8) << std::setfill('0') << std::hex << title.titleLowID;
            savePath << "/user";
            if (isExternalStorageInserted()) getSaves((std::string("storage_usb01:") + savePath.str()), title.saves, title.commonSave);
            getSaves((std::string("storage_mlc01:") + savePath.str()), title.saves, title.commonSave);
        }
    }

    // Clear unparsed titles
    unparsedTitleList.clear();
    return true;
}

titleEntry emptyEntry;
std::reference_wrapper<titleEntry> getTitleWithName(std::string nameOfTitle) {
    for (auto& title : installedTitles) {
        if (title.shortTitle == nameOfTitle) return std::ref(title);
    }
    return std::ref(emptyEntry);
}

// Helper functions

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::string normalizeTitle(std::string& unsafeTitle) {
    std::string retTitle;
    uint32_t actualCharacters = 0;
    for (char& chr : unsafeTitle) {
        if (isalnum(chr)) {
            actualCharacters++;
            retTitle += chr;
        }
        else if (chr == ' ') {
            retTitle += chr;
        }
    }
    if (actualCharacters <= 2) return "";

    rtrim(retTitle);
    ltrim(retTitle);

    return retTitle;
}

bool isBase(MCPAppType type) {
    return type == MCP_APP_TYPE_GAME;
}

bool isUpdate(MCPAppType type) {
    return type == MCP_APP_TYPE_GAME_UPDATE;
}

bool isDLC(MCPAppType type) {
    return type == MCP_APP_TYPE_GAME_DLC;
}

bool isSystemApp(MCPAppType type) {
    return type == MCP_APP_TYPE_BROWSER || type == MCP_APP_TYPE_ESHOP || type == MCP_APP_TYPE_FRIEND_LIST || type == MCP_APP_TYPE_AMIIBO_SETTINGS || type == MCP_APP_TYPE_DOWNLOAD_MANAGEMENT || type == MCP_APP_TYPE_HOME_MENU || type == MCP_APP_TYPE_EMANUAL || type == MCP_APP_TYPE_SYSTEM_APPS || type == MCP_APP_TYPE_SYSTEM_SETTINGS;
}
