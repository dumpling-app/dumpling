#include "titles.h"
#include "filesystem.h"
#include "users.h"
#include "gui.h"
#include "menu.h"
#include "cfw.h"

std::vector<std::shared_ptr<titleEntry>> installedTitles = {};
std::map<uint32_t, std::vector<MCPTitleListType>> rawTitles = {};
std::map<uint32_t, savePart> rawSaves = {};

bool getTitleMetaXml(titleEntry& title, titlePart& part) {
    // Check if /meta/meta.xml file exists
    std::string metaPath(part.posixPath);
    metaPath.append("/meta/meta.xml");

    // Read meta.xml
    std::ifstream xmlFile(metaPath);
    if (xmlFile.bad()) {
        WHBLogPrint("Couldn't open title xml file stream!");
        WHBLogPrint(metaPath.c_str());
        return false;
    }
    std::string line;
    std::string shortTitleJapan = "";

    // Parse data from it using string matching
    bool foundShortTitle = !title.shortTitle.empty();
    bool foundJapaneseShortTitle = !shortTitleJapan.empty();
    bool foundProductCode = !title.productCode.empty();
    while(getline(xmlFile, line)) {
        if (!foundShortTitle && line.find("shortname_en") != std::string::npos) {
            title.shortTitle = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            trim(title.shortTitle);
            decodeXMLEscapeLine(title.shortTitle);
            if (!title.shortTitle.empty()) foundShortTitle = true;
        }
        else if (!foundJapaneseShortTitle && line.find("shortname_ja") != std::string::npos) {
            shortTitleJapan = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            trim(shortTitleJapan);
            decodeXMLEscapeLine(shortTitleJapan);
            if (!shortTitleJapan.empty()) foundJapaneseShortTitle = true;
        }
        else if (!foundProductCode && line.find("product_code") != std::string::npos) {
            title.productCode = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            foundProductCode = true;
        }
        // Stop when all fields are encountered
        if (foundShortTitle && foundJapaneseShortTitle && foundProductCode) break;
    }
    xmlFile.close();

    if ((foundShortTitle || foundJapaneseShortTitle) && foundProductCode) {
        // Finish up information
        if (!foundShortTitle) {
            title.shortTitle = shortTitleJapan;
        }
        title.folderName = normalizeFolderName(title.shortTitle);
        part.outputPath += title.folderName;
        return true;
    }

    WHBLogPrint("Error while parsing this meta.xml file:");
    WHBLogPrint(metaPath.c_str());
    WHBLogPrint("Problem was that only some information could be found:");
    WHBLogPrintf("shortname_en = %s", title.shortTitle.c_str());
    WHBLogPrintf("shortname_ja = %s", shortTitleJapan.c_str());
    WHBLogPrintf("product_code = %s", title.productCode.c_str());
    WHBLogFreetypeDraw();
    return false;
}

bool getSaveMetaXml(titleEntry& title, savePart& part) {
    // Check if /meta/meta.xml file exists
    std::string metaPath(part.savePath);
    metaPath.append("/meta/meta.xml");

    // Read meta.xml
    std::ifstream xmlFile(metaPath);
    if (xmlFile.bad()) {
        WHBLogPrint("Couldn't open save xml file stream!");
        WHBLogPrint(metaPath.c_str());
        return false;
    }
    std::string line;
    std::string shortTitleJapan = "";

    // Parse data from it using string matching
    bool foundShortTitle = !title.shortTitle.empty();
    bool foundJapaneseShortTitle = !shortTitleJapan.empty();
    bool foundProductCode = !title.productCode.empty();
    while(getline(xmlFile, line)) {
        if (!foundShortTitle && line.find("shortname_en") != std::string::npos) {
            title.shortTitle = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            trim(title.shortTitle);
            decodeXMLEscapeLine(title.shortTitle);
            if (!title.shortTitle.empty()) foundShortTitle = true;
        }
        else if (!foundJapaneseShortTitle && line.find("shortname_ja") != std::string::npos) {
            shortTitleJapan = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            trim(shortTitleJapan);
            decodeXMLEscapeLine(shortTitleJapan);
            if (!shortTitleJapan.empty()) foundJapaneseShortTitle = true;
        }
        else if (!foundProductCode && line.find("product_code") != std::string::npos) {
            title.productCode = line.substr(line.find(">")+1, (line.find_last_of("<"))-(line.find_first_of(">")+1));
            foundProductCode = true;
        }
        // Stop when all fields are encountered
        if (foundShortTitle && foundJapaneseShortTitle && foundProductCode) break;
    }

    if ((foundShortTitle || foundJapaneseShortTitle) && foundProductCode) {
        // Finish up information
        if (!foundShortTitle) {
            title.shortTitle = shortTitleJapan;
        }
        title.folderName = normalizeFolderName(title.shortTitle);
        return true;
    }

    WHBLogPrint("Error while parsing this meta.xml file:");
    WHBLogPrint(metaPath.c_str());
    WHBLogPrint("Problem was that only some information could be found:");
    WHBLogPrintf("shortname_en = %s", title.shortTitle.c_str());
    WHBLogPrintf("shortname_ja = %s", shortTitleJapan.c_str());
    WHBLogPrintf("product_code = %s", title.productCode.c_str());
    WHBLogFreetypeDraw();
    return false;
}

bool getTitleList(bool skipDiscs) {
    WHBLogPrint("Loading titles...");
    WHBLogFreetypeDraw();

    // Open MCP bridge
    int32_t mcpHandle = MCP_Open();
    if (mcpHandle < 0) {
        WHBLogPrint("Failed to open MPC! Is a CFW not cooperating?");
        return false;
    }

    // Get titles from MCP
    uint32_t titleCount = MCP_TitleCount(mcpHandle);
    uint32_t titleListByteSize = titleCount * sizeof(MCPTitleListType);

    std::vector<MCPTitleListType> unparsedTitleList(titleCount);
    MCP_TitleList(mcpHandle, &titleCount, unparsedTitleList.data(), titleListByteSize);
    MCP_Close(mcpHandle);

    // Queue and group parts of each title
    for (auto& title : unparsedTitleList) {
        // Skip discs whenever there's an initial scan
        if (skipDiscs && deviceToLocation(title.indexedDevice) == titleLocation::Disc) {
            continue;
        }

        // Check if it's a supported app type
        if (isBase(title.appType) || isUpdate(title.appType) || isDLC(title.appType) || isSystemApp(title.appType)) {
            std::string posixPath = convertToPosixPath(title.path);
            if (!posixPath.empty() && dirExist(posixPath.c_str())) {
                uint32_t gameId = (uint32_t)title.titleId & 0x00000000FFFFFFFF;
                rawTitles[gameId].emplace_back(title);
            }
            else {
                WHBLogPrint("Couldn't convert the path or find this folder:");
                WHBLogPrint(posixPath.c_str());
                WHBLogFreetypeDraw();
                // sleep_for(2s);
            }
        }
    }
    return true;
}

bool getSaveList(std::string saveDirPath) {
    WHBLogPrintf("Loading saves...", saveDirPath.c_str());
    WHBLogFreetypeDraw();

    // Loop over high title id folders e.g. storage_mlc01:/usr/save/[iterated]
    DIR* highDirHandle;
    if ((highDirHandle = opendir((saveDirPath+"/").c_str())) == nullptr) return false;

    struct dirent* highDirEntry;
    while ((highDirEntry = readdir(highDirHandle)) != nullptr) {
        if ((highDirEntry->d_type & DT_DIR) != DT_DIR) continue;
        if (strlen(highDirEntry->d_name) != 8) continue;

        uint32_t highTitleId = strtoul((const char*)highDirEntry->d_name, nullptr, 16);
        if (highTitleId == 0) continue;
        std::string highDirPath = saveDirPath + highDirEntry->d_name + "/";

        // Loop over low title id folders e.g. storage_mlc01:/usr/save/00050000/[iterated]
        DIR* lowDirHandle;
        if ((lowDirHandle = opendir(highDirPath.c_str())) == nullptr) continue;

        struct dirent* lowDirEntry;
        while ((lowDirEntry = readdir(lowDirHandle)) != nullptr) {
            if ((lowDirEntry->d_type & DT_DIR) != DT_DIR) continue;
            if (strlen(lowDirEntry->d_name) != 8) continue;

            uint32_t lowTitleId = strtoul((const char*)&lowDirEntry->d_name, nullptr, 16);
            if (lowTitleId == 0) continue;
            std::string lowDirPath = highDirPath + lowDirEntry->d_name + "/";

            const auto& currSavePart = rawSaves.try_emplace(lowTitleId, savePart{.savePath = lowDirPath.c_str(), .titleHighId = highTitleId});

            // Loop over user saves folders e.g. storage_mlc01:/usr/save/00050000/101b0500/user/[iterated]
            DIR* userDirHandle;
            if ((userDirHandle = opendir((lowDirPath + "user/").c_str())) == nullptr) continue;

            struct dirent* userDirEntry;
            while ((userDirEntry = readdir(userDirHandle)) != nullptr) {
                if ((userDirEntry->d_type & DT_DIR) != DT_DIR) continue;

                std::string userDirPath = lowDirPath + "user/" + userDirEntry->d_name;

                // Check whether the common or user save has any contents
                bool hasContents = false;
                DIR* contentDirHandle;
                if ((contentDirHandle = opendir(userDirPath.c_str())) == nullptr) continue;
                
                struct dirent* contentDirEntry;
                while ((contentDirEntry = readdir(contentDirHandle)) != nullptr) {
                    if ((contentDirEntry->d_type & DT_DIR) == DT_DIR || (contentDirEntry->d_type & DT_REG) == DT_REG) {
                        // WHBLogPrint((userDirPath + contentDirEntry->d_name).c_str());
                        // WHBLogFreetypeDraw();
                        // sleep_for(500ms);
                        hasContents = true;
                        break;
                    }
                }
                closedir(contentDirHandle);

                if (!hasContents) continue;

                if (strcmp(userDirEntry->d_name, "common") == 0) {
                    currSavePart.first->second.commonSave = titleCommonSave{.path = userDirPath, .location = pathToLocation(userDirPath.c_str())};
                }
                else if (strlen(userDirEntry->d_name) == 8) {
                    nn::act::PersistentId persistentId = strtoul((const char*)&userDirEntry->d_name, nullptr, 16);
                    if (lowTitleId == 0) continue;
                    currSavePart.first->second.userSaves.emplace_back(titleUserSave{.userId = persistentId, .path = userDirPath, .location = pathToLocation(userDirPath.c_str())});
                }
            }
            closedir(userDirHandle);
        }
        closedir(lowDirHandle);
    }
    closedir(highDirHandle);
    return true;
}

bool loadTitles(bool skipDiscs) {
    WHBLogPrint("Searching for games...");
    WHBLogFreetypeDraw();

    rawSaves.clear();
    rawTitles.clear();

    // Get sorted lists
    if (!getTitleList(skipDiscs) || !getSaveList(convertToPosixPath("/vol/storage_mlc01/usr/save/")) || !(!testStorage(titleLocation::USB) || getSaveList(convertToPosixPath("/vol/storage_usb01/usr/save/")))) {
        WHBLogPrint("Error while getting the titles/saves, can't continue!");
        WHBLogPrint("Please report the issue on Dumpling's Github!");
        WHBLogFreetypeDraw();
        sleep_for(5s);
        return false;
    }

    // Parse title meta files and create title entries for each title
    installedTitles.clear();
    for (auto& sortedTitle : rawTitles) {
        std::shared_ptr<titleEntry>& title = installedTitles.emplace_back(std::make_shared<titleEntry>());
        title->titleLowId = sortedTitle.first;

        // Loop over each part of a title
        for (MCPTitleListType& part : sortedTitle.second) {
            if (isBase(part.appType) || isSystemApp(part.appType)) {
                title->base = titlePart{};
                title->base->posixPath = convertToPosixPath(part.path);
                title->base->version = part.titleVersion;
                title->base->type = part.appType;
                title->base->titleHighId = (uint32_t)((part.titleId & 0xFFFFFFFF00000000) >> 32);
                title->base->location = deviceToLocation(part.indexedDevice);
                
                // Change the output path
                if (isBase(part.appType)) title->base->outputPath = "/Games/";
                if (isSystemApp(part.appType)) title->base->outputPath = "/System Applications/";
                
                if (!getTitleMetaXml(*title, *title->base)) {
                    title->base.reset();
                    WHBLogPrint("Failed to read meta from game!");
                    WHBLogFreetypeDraw();
                    sleep_for(2s);
                }
            }
            else if (isUpdate(part.appType)) { // TODO: Log cases where maybe two updates are found (one on disc and one later via the title system)
                title->update = titlePart{};
                title->update->posixPath = convertToPosixPath(part.path);
                title->update->version = part.titleVersion;
                title->update->type = part.appType;
                title->update->titleHighId = (uint32_t)((part.titleId & 0xFFFFFFFF00000000) >> 32);
                title->update->location = deviceToLocation(part.indexedDevice);
                title->update->outputPath = "/Updates/";
                if (!getTitleMetaXml(*title, *title->update)) {
                    title->update.reset();
                    WHBLogPrint("Failed to read meta from update!");
                    WHBLogFreetypeDraw();
                    sleep_for(2s);
                }
            }
            else if (isDLC(part.appType)) {
                title->dlc = titlePart{};
                title->dlc->posixPath = convertToPosixPath(part.path);
                title->dlc->version = part.titleVersion;
                title->dlc->type = part.appType;
                title->dlc->titleHighId = (uint32_t)((part.titleId & 0xFFFFFFFF00000000) >> 32);
                title->dlc->location = deviceToLocation(part.indexedDevice);
                title->dlc->outputPath = "/DLCs/";
                if (!getTitleMetaXml(*title, *title->dlc)) {
                    title->dlc.reset();
                    WHBLogPrint("Failed to read meta from dlc!");
                    WHBLogFreetypeDraw();
                    sleep_for(2s);
                }
            }
        }

        // Remove element from list again if it contains nothing
        if (!(title->base || title->update || title->dlc)) {
            installedTitles.pop_back();
        }
    }

    // Try to append or create new title entries from save list data
    for (auto& sortedSave : rawSaves) {
        uint32_t saveTitleId = sortedSave.first;
        const auto& existingEntry = std::find_if(installedTitles.begin(), installedTitles.end(), [saveTitleId](std::shared_ptr<titleEntry>& entry){ return entry->titleLowId == saveTitleId; });
        if (existingEntry != installedTitles.end()) {
            (*existingEntry)->saves = sortedSave.second;
        }
        else {
            auto& newEntry = installedTitles.emplace_back(std::make_shared<titleEntry>(titleEntry{
                .titleLowId = sortedSave.first,
                .saves = sortedSave.second
            }));
            
            if (!getSaveMetaXml(*newEntry, *newEntry->saves)) {
                newEntry->saves.reset();
                WHBLogPrint("Failed to read meta from saves!");
                WHBLogFreetypeDraw();
                sleep_for(250ms);
            }
        }
    }

    rawSaves.clear();
    rawTitles.clear();

    return true;
}

bool checkForDiscTitles(int32_t mcpHandle) {
    int32_t titleCount = MCP_TitleCount(mcpHandle);
    uint32_t titleByteSize = titleCount * sizeof(MCPTitleListType);

    std::vector<MCPTitleListType> titles(titleCount);

    uint32_t titlesListed = 0;
    MCP_TitleList(mcpHandle, &titlesListed, titles.data(), titleByteSize);

    for (auto& title : titles) {
        if (isBase(title.appType) && deviceToLocation(title.indexedDevice) == titleLocation::Disc) {
            return true;
        }
    }
    return false;
}

/*
std::filesystem::recursive_directory_iterator dirIterator(deviceSavePath, std::filesystem::directory_options::skip_permission_denied);
try {
    for (const auto& dirEntry : dirIterator) {
        if (dirEntry.is_directory()) {
            if ((dirIterator.depth() == 0 || dirIterator.depth() == 1) && dirEntry.path().filename().string().length() != 8) dirIterator.disable_recursion_pending(); // Only go into subdirectories of title id folders
            else if (dirIterator.depth() == 2 && dirEntry.path().filename() != "user") dirIterator.disable_recursion_pending(); // Only go into the user folder
            else if (dirIterator.depth() == 3 && !(dirEntry.path().filename().string().length() == 8 || dirEntry.path().filename() == "common")) dirIterator.disable_recursion_pending(); // Only go into common or user id folders
        }
        if (dirIterator.depth() == 4 && (dirEntry.is_regular_file() || dirEntry.is_directory())) {
            // Split current path up in usable elements, should be /usr/save/00050000/101b0500/user/800000XX/foo.bar or /usr/save/00050000/101b0500/user/common/foo.bar
            std::string titleIdHighStr = dirEntry.path().parent_path().parent_path().parent_path().parent_path().filename().string();
            std::string titleIdLowStr = dirEntry.path().parent_path().parent_path().parent_path().filename().string();
            uint32_t titleIdHigh = std::stoul(titleIdHighStr);
            uint32_t titleIdLow = std::stoul(titleIdLowStr);
            // uint64_t titleId = (((uint64_t)titleIdHigh) << 32) | (((uint64_t)titleIdLow) << 0);
            const auto& currSavePart = rawSaves.try_emplace(titleIdLow, savePart{.savePath = dirEntry.path().string(), .titleHighId = titleIdHigh});
            if (dirEntry.path().parent_path().filename() == "common") {
                currSavePart.first->second.commonSave = titleCommonSave{.path = dirEntry.path().parent_path().string(), .location = pathToLocation(dirEntry.path().string().c_str())};
            }
            else {
                nn::act::PersistentId persistentId = std::stoul(dirEntry.path().parent_path().filename().string());
                currSavePart.first->second.userSaves.emplace_back(titleUserSave{.userId = persistentId, .path = dirEntry.path().parent_path().string(), .location = pathToLocation(dirEntry.path().string().c_str())});
            }
            dirIterator.pop();
        }
    }
}
catch(const std::filesystem::filesystem_error& error) {
    WHBLogPrint("Failed while recursively finding saves! Error:");
    WHBLogPrintf("Value: %d", error.code().value());
    WHBLogPrintf("Message: %s", error.code().message().c_str());
    WHBLogPrintf("Path 1: %s", error.path1().c_str());
    WHBLogPrintf("Path 2: %s", error.path2().c_str());
    WHBLogFreetypeDraw();
    sleep_for(8s);
    return false;
}

enum ACPDeviceTypeEnum {
    UnknownType = 0,
    InternalDeviceType = 1,
    USBDeviceType = 3, // seems to be /storage_mlc/ at the very least
    UnknownDeviceType = 4, // seen in system settings
    ACPUSBDeviceType = 8, // "/vol/acp_usb/usr/save/"
};

// Taken from Cemu
struct ACPSaveDirInfo {
	uint32_t ukn00;
	uint32_t ukn04;
	uint32_t persistentId;
	uint32_t ukn0C;
	//uint32_t ukn10; // ukn10/ukn14 are part of size
	//uint32_t ukn14;
	//uint32_t ukn18; // ukn18/ukn1C are part of size
	//uint32_t ukn1C;
	uint64_t sizeA;
	uint64_t sizeB;
	// path starts at 0x20, length unknown?
	char path[0x40]; // %susr/save/%08x/%08x/meta/			// /vol/storage_mlc01/
	// +0x60  uint64_t time;
	// +0x68  uint8_t padding[0x80 - 0x68];
	// size is 0x80, but actual content size is only 0x60 and padded to 0x80?
};

static_assert(sizeof(ACPSaveDirInfo) == 0x80, "ACPSaveDirInfo has invalid size");

#ifdef __cplusplus
extern "C" {
#endif
    ACPResult ACPGetSaveDataTitleIdList(ACPDeviceType storageDevice, ACPTitleId* titleIdList, uint32_t getCount, uint32_t* maxCount);
    ACPResult ACPGetTitleSaveDirEx(uint32_t titleIdPart1, uint32_t titleIdPart2, ACPDeviceTypeEnum storageDevice, uint32_t unknown2, ACPSaveDirInfo* saveDirInfoList, uint32_t getCount, uint32_t* maxCount);
#ifdef __cplusplus
}
#endif

bool getSaveListThroughACP(bool skipDiscs) {
    ACPInitialize();
    WHBLogPrint("Loading saves...");
    WHBLogFreetypeDraw();
    ACPResult result = ACP_RESULT_SUCCESS;

    uint32_t getCount = 512;
    uint32_t saveListCount = 0;
    ACPTitleId* saveList = (ACPTitleId*)aligned_alloc(64, sizeof(ACPTitleId)*getCount);
    if (saveList == NULL || (result = ACPGetSaveDataTitleIdList(ACPDeviceTypeEnum::USBDeviceType, saveList, sizeof(ACPTitleId)*getCount, &saveListCount)) != ACP_RESULT_SUCCESS) {
        WHBLogPrintf("Error %d returned by ACPGetSaveDataTitleIdList2", result);
        free(saveList);
        return false;
    }

    WHBLogPrintf("Found %u games with save data", saveListCount);
    WHBLogFreetypeDraw();
    sleep_for(2s);

    // for (uint32_t i=0; i<saveListCount; i++) {
    //     WHBLogPrintf("Parsing saves for title %016llX", saveList[i]);
    //     uint32_t getSaveDirCount = 32;
    //     uint32_t saveDirCount = 0;
    //     ACPSaveDirInfo* saveDirInfoList = (ACPSaveDirInfo*)aligned_alloc(64, sizeof(ACPSaveDirInfo)*getSaveDirCount);
    //     if ((result = ACPGetTitleSaveDirEx((uint32_t)((saveList[i] & 0xFFFFFFFF00000000LL) >> 32), (uint32_t)((saveList[i] & 0x00000000FFFFFFFFLL) >> 0), ACPDeviceTypeEnum::USBDeviceType, 0, saveDirInfoList, getSaveDirCount, &saveDirCount)) != ACP_RESULT_SUCCESS) {
    //         WHBLogPrintf("Error %d returned by ACPGetTitleSaveDirEx", result);
    //         free(saveList);
    //         free(saveDirInfoList);
    //         return false;
    //     }

    //     for (uint32_t j=0; j<saveDirCount; j++) {
    //         WHBLogPrintf(" |-> %08X = %s", saveDirInfoList[j].persistentId, saveDirInfoList[j].path);
    //     }
    //     WHBLogFreetypeDraw();
    //     sleep_for(1s);
    //     free(saveDirInfoList);
    // }

    free(saveList);
    ACPFinalize();
    return true;
}
*/

std::optional<std::shared_ptr<titleEntry>> getTitleWithName(std::string& nameOfTitle) {
    for (auto& title : installedTitles) {
        if (title->shortTitle == nameOfTitle) return title;
    }
    return std::nullopt;
}

// Helper functions
std::array nonValidFilenames{'\0', '\\', '/', ':', '*', '?', '\"', '<', '>', '|', '.', '%', '$', '#'};

void decodeXMLEscapeLine(std::string& xmlString) {
    replaceAll(xmlString, "&quot;", "\"");
    replaceAll(xmlString, "&apos;", "'");
    replaceAll(xmlString, "&lt;", "<");
    replaceAll(xmlString, "&gt;", ">");
    replaceAll(xmlString, "&amp;", "&");
}

std::string normalizeFolderName(std::string& unsafeTitle) {
    std::string retTitle = "";
    for (char& chr : unsafeTitle) {
        if (std::find(nonValidFilenames.begin(), nonValidFilenames.end(), chr) == nonValidFilenames.end()) {
            retTitle += chr;
        }
    }
    trim(retTitle);
    return retTitle;
}

bool isBase(MCPAppType type) {
    return type == MCP_APP_TYPE_GAME || type == MCP_APP_TYPE_GAME_WII;
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
