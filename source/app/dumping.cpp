#include "dumping.h"
#include "progress.h"
#include "menu.h"
#include "filesystem.h"
#include "navigation.h"
#include "titles.h"
#include "users.h"
#include "cfw.h"
#include "gui.h"

#include "interfaces/transfer.h"
#include "interfaces/fat32.h"
#include "interfaces/stub.h"

#include <dirent.h>

#include <mocha/mocha.h>
#include <mocha/otp.h>

// Dumping Functions
#define BUFFER_SIZE_ALIGNMENT 64
#define BUFFER_SIZE (1024 * BUFFER_SIZE_ALIGNMENT * 4)
static_assert(BUFFER_SIZE % 0x8000 == 0, "Buffer size needs to be multiple of 0x8000 to properly read disc sections");


enum class WALK_EVENT {
    FILE,
    BUFFER,
    MAKE_PATH,
    DIR_ENTER,
    DIR_LEAVE
};

bool callback_scanFile(uint64_t& totalBytes, const char* filename, const std::string& srcPath) {
    struct stat fileStat{};
    if (lstat(srcPath.c_str(), &fileStat) == -1) {
        std::string errorMessage;
        errorMessage += "Failed to retrieve info from source file!\n";
        if (errno == EIO) errorMessage += "For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
        errorMessage += "\nDetails:\n";
        errorMessage += "Error "+std::to_string(errno)+" when getting stats for:\n";
        errorMessage += srcPath;
        setErrorPrompt(errorMessage);
        return false;
    }

    totalBytes += fileStat.st_size;
    return true;
}

bool callback_scanBuffer(uint64_t& totalBytes, uint64_t bufferSize) {
    totalBytes += bufferSize;
    return true;
}

bool callback_copyFile(TransferInterface* interface, bool& cancelledDumping, const char* filename, const std::string& srcPath, const std::string& destPath) {
    // Check if file is an actual file first
    struct stat fileStat{};
    if (lstat(srcPath.c_str(), &fileStat) == -1) {
        std::string errorMessage;
        errorMessage += "Failed to retrieve info from source file!\n";
        if (errno == EIO) errorMessage += "For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
        errorMessage += "\nDetails:\n";
        errorMessage += "Error "+std::to_string(errno)+" when getting stats for:\n";
        errorMessage += srcPath;
        setErrorPrompt(errorMessage);
        return false;
    }

    FILE* readHandle = fopen(srcPath.c_str(), "rb");
    if (readHandle == nullptr) {
        std::string errorMessage;
        errorMessage += "Couldn't open the file to copy from!\n";
        if (errno == EIO) errorMessage += "For discs: Make sure that its clean!\nDumping is very sensitive to tiny errors!\n";
        errorMessage += "\nDetails:\n";
        errorMessage += "Error "+std::to_string(errno)+" after opening file!\n";
        errorMessage += srcPath;
        setErrorPrompt(errorMessage);
        return false;
    }

    setFile(filename, fileStat.st_size);

    while(true) {
        uint8_t* copyBuffer = (uint8_t*)aligned_alloc(BUFFER_SIZE_ALIGNMENT, BUFFER_SIZE);
        if (copyBuffer == nullptr) {
            fclose(readHandle);
            std::string errorMessage = "Failed to allocate memory for chunk buffer!";
            setErrorPrompt(errorMessage);
            return false;
        }

        size_t bytesRead = fread(copyBuffer, sizeof(uint8_t), BUFFER_SIZE, readHandle);
        if (bytesRead != BUFFER_SIZE) {
            if (int fileError = ferror(readHandle); fileError != 0) {
                free(copyBuffer);
                copyBuffer = nullptr;
                fclose(readHandle);
                std::string errorMessage;
                errorMessage += "Failed to read all data from this file!\n";
                if (errno == EIO) errorMessage += "For discs: Make sure that its clean!\nDumping is very sensitive to tiny errors!\n";
                errorMessage += "Error "+std::to_string(errno)+" when reading data from:\n";
                errorMessage += srcPath;
                setErrorPrompt(errorMessage);
                return false;
            }
        }

        bool endOfFile = feof(readHandle) != 0;
        if (!interface->submitWriteFile(destPath, fileStat.st_size, copyBuffer, bytesRead, endOfFile)) {
            free(copyBuffer);
            fclose(readHandle);
            setErrorPrompt(*interface->getStopError());
            return false;
        }

        setFileProgress(bytesRead);
        showCurrentProgress();
        // Check whether the inputs break
        updateInputs();
        if (pressedBack()) {
            uint8_t selectedChoice = showDialogPrompt("Are you sure that you want to cancel the dumping process?", "Yes", "No");
            if (selectedChoice != 0) continue;
            WHBLogFreetypeClear();
            WHBLogPrint("Quitting dumping process...");
            WHBLogPrint("The app (likely) isn't frozen!");
            WHBLogPrint("This should take a minute at most!");
            WHBLogFreetypeDraw();
            fclose(readHandle);
            cancelledDumping = true;
            return false;
        }

        if (endOfFile) {
            break;
        }
    }

    fclose(readHandle);
    return true;
}

bool callback_copyBuffer(TransferInterface* interface, bool& cancelledDumping, const char* filename, uint8_t* srcBuffer, uint64_t bufferSize, const std::string& destPath) {
    setFile(std::filesystem::path(destPath).filename().c_str(), bufferSize);

    // Copy source buffer into aligned copy buffer, then write it in parts
    uint64_t uncopiedBytes = bufferSize;
    while (true) {
        uint8_t* copyBuffer = (uint8_t*)aligned_alloc(BUFFER_SIZE_ALIGNMENT, BUFFER_SIZE);
        if (copyBuffer == nullptr) {
            std::string errorMessage = "Failed to allocate memory for chunk buffer!";
            setErrorPrompt(errorMessage);
            return false;
        }

        uint32_t toCopyBytes = uncopiedBytes > BUFFER_SIZE ? BUFFER_SIZE : uncopiedBytes;
        bool endOfBuffer = uncopiedBytes <= BUFFER_SIZE;
        memcpy(copyBuffer, srcBuffer, toCopyBytes);

        if (!interface->submitWriteFile(destPath, bufferSize, copyBuffer, toCopyBytes, endOfBuffer)) {
            free(copyBuffer);
            setErrorPrompt(*interface->getStopError());
            return false;
        }

        setFileProgress(toCopyBytes);
        showCurrentProgress();
        // Check whether the inputs break
        updateInputs();
        if (pressedBack()) {
            uint8_t selectedChoice = showDialogPrompt("Are you sure that you want to cancel the dumping process?", "Yes", "No");
            if (selectedChoice != 0) continue;
            WHBLogFreetypeClear();
            WHBLogPrint("Quitting dumping process...");
            WHBLogPrint("The app (likely) isn't frozen!");
            WHBLogPrint("This should take a minute at most!");
            WHBLogFreetypeDraw();
            cancelledDumping = true;
            return false;
        }

        if (endOfBuffer) {
            break;
        }
    }
    return true;
}

bool callback_makeDir(TransferInterface* interface, const std::string& destPath, bool makeRecursively) {
    if (makeRecursively) {
        std::filesystem::path folderPath(destPath);
        folderPath = folderPath.remove_filename();
        std::string createdPath;
        for (const auto& it : folderPath) {
            if (!it.empty() && it.string() != "/") {
                createdPath += ("/" + it.string());
                if (!interface->submitWriteFolder(createdPath)) {
                    setErrorPrompt(*interface->getStopError());
                    return false;
                }
            }
        }
    }
    if (!interface->submitWriteFolder(destPath)) {
        setErrorPrompt(*interface->getStopError());
        return false;
    }
    return true;
}

bool callback_moveDir(TransferInterface* interface, const std::string& destPath) {
    if (!interface->submitSwitchFolder(destPath)) {
        setErrorPrompt(*interface->getStopError());
        return false;
    }
    return true;
}

template <typename F>
bool walkDirectoryRecursive(const std::string& srcPath, const std::string& destPath, F&& callback) {
    // Call callback that e.g. makes directories first
    if (!callback(WALK_EVENT::DIR_ENTER, nullptr, srcPath, destPath)) {
        return false;
    }

    // Open folder
    DIR* dirHandle;
    if ((dirHandle = opendir((srcPath+"/").c_str())) == nullptr) {
        std::string errorMessage;
        errorMessage += "Failed to open folder to read content from!\n";
        if (errno == EIO) errorMessage += "For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
        errorMessage += "\nDetails:\n";
        errorMessage += "Error "+std::to_string(errno)+" while opening folder:\n";
        errorMessage += srcPath;
        setErrorPrompt(errorMessage);
        setErrorPrompt("Failed to open the directory to read files from");
        return false;
    }

    // Loop over directory contents
    while (true) {
        errno = 0;
        struct dirent* dirEntry = readdir(dirHandle);
        if (dirEntry != nullptr) {
            std::string entrySrcPath = srcPath + "/" + dirEntry->d_name;
            std::string entryDstPath = destPath + "/" + dirEntry->d_name;
            // Use lstat since readdir returns DT_REG for symlinks
            struct stat fileStat{};
            if (lstat(entrySrcPath.c_str(), &fileStat) != 0) {
                setErrorPrompt("Unknown Error "+std::to_string(errno)+":\nCouldn't check type of file/folder!\n"+entrySrcPath);
                return false;
            }
            if (S_ISLNK(fileStat.st_mode)) {
                continue;
            }
            else if (S_ISREG(fileStat.st_mode)) {
                // Copy file
                if (!callback(WALK_EVENT::FILE, dirEntry->d_name, entrySrcPath, entryDstPath)) {
                    closedir(dirHandle);
                    return false;
                }
            }
            else if (S_ISDIR(fileStat.st_mode)) {
                // Ignore root and parent folder entries
                if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) continue;

                // Copy all the files in this subdirectory
                if (!walkDirectoryRecursive(entrySrcPath, entryDstPath, callback)) {
                    closedir(dirHandle);
                    return false;
                }
            }
        }
        else if (errno == EIO) {
            std::string errorMessage =
                    "The Wii U had an issue while reading the disc(?)!\n"
                    "Make sure that its clean!\nDumping is very sensitive to tiny issues!\n"
                    "Unfortunately, disc requires reinsertion to retry!\nTry restarting Dumpling and trying again.\n\nDetails:\n"
                    "Error "+std::to_string(errno)+" while iterating folder contents:\n"
                    +srcPath;
            setErrorPrompt(errorMessage);
            return false;
        }
        else if (errno != 0) {
            std::string errorMessage =
                    "Unknown error while reading disc contents!\n"
                    "You might want to restart your console and try again.\n\nDetails:\n"
                    "Error "+std::to_string(errno)+" while iterating folder contents:\n"
                    +srcPath;
            setErrorPrompt(errorMessage);
            return false;
        }
        else {
            break;
        }
    }

    // Call callback that e.g. deletes directories after it deletes all files
    closedir(dirHandle);
    return callback(WALK_EVENT::DIR_LEAVE, nullptr, srcPath, destPath);
}

template <class F>
bool processTitleEntry(titleEntry& entry, dumpingConfig& config, F&& callback) {
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::GAME) && entry.base) {
        if (!callback(WALK_EVENT::MAKE_PATH, nullptr, entry.base->posixPath, entry.base->outputPath)) return false;
        if (!walkDirectoryRecursive(entry.base->posixPath, entry.base->outputPath, callback)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::UPDATE) && entry.update) {
        if (!callback(WALK_EVENT::MAKE_PATH, nullptr, entry.update->posixPath, entry.update->outputPath)) return false;
        if (!walkDirectoryRecursive(entry.update->posixPath, entry.update->outputPath, callback)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::DLC) && entry.dlc) {
        if (!callback(WALK_EVENT::MAKE_PATH, nullptr, entry.dlc->posixPath, entry.dlc->outputPath)) return false;
        if (!walkDirectoryRecursive(entry.dlc->posixPath, entry.dlc->outputPath, callback)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::SAVES) && entry.saves && !entry.saves->userSaves.empty()) {
        for (auto& save : entry.saves->userSaves) {
            if (save.userId == config.accountId) {
                if (!callback(WALK_EVENT::MAKE_PATH, nullptr, save.path, entry.saves->outputPath + "/" + (config.dumpAsDefaultUser ? "80000001" : getUserByPersistentId(save.userId)->persistentIdString))) return false;
                if (!walkDirectoryRecursive(save.path, entry.saves->outputPath + "/" + (config.dumpAsDefaultUser ? "80000001" : getUserByPersistentId(save.userId)->persistentIdString), callback)) return false;
            }
        }
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::SAVES) && entry.saves && entry.saves->commonSave) {
        if (!callback(WALK_EVENT::MAKE_PATH, nullptr, entry.saves->commonSave->path, entry.saves->outputPath + "/common")) return false;
        if (!walkDirectoryRecursive(entry.saves->commonSave->path, entry.saves->outputPath + "/common", callback)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::CUSTOM) && entry.customFolder) {
        if (!callback(WALK_EVENT::MAKE_PATH, nullptr, entry.customFolder->inputPath, entry.customFolder->outputPath)) return false;
        if (!walkDirectoryRecursive(entry.customFolder->inputPath, entry.customFolder->outputPath, callback)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::CUSTOM) && entry.customFile) {
        if (!callback(WALK_EVENT::MAKE_PATH, nullptr, "", entry.customFile->outputFolder)) return false;
        if (!callback(WALK_EVENT::DIR_ENTER, entry.customFile->outputFile.c_str(), "", entry.customFile->outputFolder)) return false;
        if (!callback(WALK_EVENT::BUFFER, entry.customFile->outputFile.c_str(), "", entry.customFile->outputFolder)) return false;
        if (!callback(WALK_EVENT::DIR_LEAVE, entry.customFile->outputFile.c_str(), "", entry.customFile->outputFolder)) return false;
    }
    return true;
}

bool deleteTitleEntry(titleEntry& entry, dumpingConfig& config, const std::function<void()>& callback_updateGUI) {
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::GAME) && entry.base) {
        if (!Fat32Transfer::deletePath(config.dumpTarget + entry.base->outputPath, callback_updateGUI)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::UPDATE) && entry.update) {
        if (!Fat32Transfer::deletePath(config.dumpTarget + entry.update->outputPath, callback_updateGUI)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::DLC) && entry.dlc) {
        if (!Fat32Transfer::deletePath(config.dumpTarget + entry.dlc->outputPath, callback_updateGUI)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::SAVES) && entry.saves && !entry.saves->userSaves.empty()) {
        for (auto& save : entry.saves->userSaves) {
            if (save.userId == config.accountId) {
                std::string userSavePath = config.dumpAsDefaultUser ? "80000001" : getUserByPersistentId(save.userId)->persistentIdString;
                if (!Fat32Transfer::deletePath(config.dumpTarget + entry.saves->outputPath + "/" + userSavePath, callback_updateGUI)) return false;
            }
        }
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::SAVES) && entry.saves && entry.saves->commonSave) {
        if (!Fat32Transfer::deletePath(config.dumpTarget + entry.saves->outputPath + "/common", callback_updateGUI)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::CUSTOM) && entry.customFolder) {
        if (!Fat32Transfer::deletePath(config.dumpTarget + entry.customFolder->outputPath, callback_updateGUI)) return false;
    }
    if (HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::CUSTOM) && entry.customFile) {
        if (!Fat32Transfer::deletePath(config.dumpTarget+entry.customFile->outputFolder+"/"+entry.customFile->outputFile, callback_updateGUI)) return false;
    }
    return true;
}

bool dumpQueue(std::vector<std::shared_ptr<titleEntry>>& queue, dumpingConfig& config) {
    // Delete previously dumped files
    if (config.dumpMethod == DUMP_METHOD::FAT && !USE_WUT_DEVOPTAB()) {
        uint64_t deletedCount = 0;
        auto showDeleteProgress = [&deletedCount]() {
            if (deletedCount % 10 == 0) {
                WHBLogFreetypeStartScreen();
                WHBLogPrint("Checking for existing files!");
                WHBLogFreetypePrint("");
                WHBLogFreetypePrint("This might take a few minutes if there's a lot of files...");
                WHBLogFreetypePrint("Please don't turn your Wii U off!");
                WHBLogFreetypePrint("");
                WHBLogFreetypePrintf("Found and deleted %llu files...", deletedCount);
                WHBLogFreetypeDrawScreen();
            }
            deletedCount++;
        };
        showDeleteProgress();
        for (auto& entry : queue) {
            if (!deleteTitleEntry(*entry, config, showDeleteProgress)) {
                showErrorPrompt("Exit to Main Menu");
                return false;
            }
        }
    }

    // Scan title size
    uint64_t totalDumpSize = 0;
    if (config.dumpMethod == DUMP_METHOD::FAT && config.scanTitleSize) {
        // Scan folder to get the full queue size
        for (size_t i=0; i<queue.size(); i++) {
            // Show message about the scanning process which freezes the GUI
            WHBLogFreetypeStartScreen();
            WHBLogFreetypePrint("Scanning the dump size!");
            WHBLogFreetypePrint("This might take a few minutes if you selected a lot of titles...");
            WHBLogFreetypePrint("Your Wii U isn't frozen in that case!");
            WHBLogFreetypePrint("");
            WHBLogPrintf("Scanning %s... (title %lu / %lu)", queue[i]->shortTitle.c_str(), i+1, queue.size());
            WHBLogFreetypePrint("");
            WHBLogFreetypeScreenPrintBottom("===============================");
            WHBLogFreetypeScreenPrintBottom("\uE001 Button = Cancel scanning and just do dumping");
            WHBLogFreetypeDraw();
            titleEntry& queueEntry = *queue[i];
            bool cancelledDumping = false;
            bool scanSuccess = processTitleEntry(queueEntry, config, [&totalDumpSize, &cancelledDumping, &queueEntry](WALK_EVENT event, const char *filename, const std::string &srcPath, const std::string &destPath) -> bool {
                updateInputs();
                if (pressedBack()) {
                    cancelledDumping = true;
                    return false;
                }
                else if (event == WALK_EVENT::FILE)
                    return callback_scanFile(totalDumpSize, filename, srcPath);
                else if (event == WALK_EVENT::BUFFER)
                    return callback_scanBuffer(totalDumpSize, queueEntry.customFile->srcBufferSize);
                return true;
            });
            if (cancelledDumping) {
                totalDumpSize = 0;
                sleep_for(1s);
                break;
            }
            else if (!scanSuccess) {
                showErrorPrompt("Back To Main Menu");
                showDialogPrompt("Failed while trying to scan the dump for its size!", "Exit to Main Menu");
                return false;
            }
        }

        if (totalDumpSize > 0) {
            // Check if there's enough space on the storage location and otherwise give a warning
            uint64_t sizeAvailable = !USE_WUT_DEVOPTAB() ? Fat32Transfer::getDriveSpace(config.dumpTarget) : std::numeric_limits<uint64_t>::max();
            if (sizeAvailable < totalDumpSize) {
                std::string freeSpaceWarning;
                freeSpaceWarning += "You only have " + formatByteSize(sizeAvailable) + ", while you require " + formatByteSize(totalDumpSize) + " of free space!\n";
                if (sizeAvailable == 0) freeSpaceWarning += "Make sure that your storage device is still plugged in!";
                else freeSpaceWarning += "Make enough space, or dump one game at a time.";
                showDialogPrompt(freeSpaceWarning.c_str(), "Exit to Main Menu");
                return false;
            }
            else {
                WHBLogFreetypeClear();
                WHBLogPrintf("Dump is %s while selected location has %s available!", formatByteSize(totalDumpSize).c_str(), formatByteSize(sizeAvailable).c_str());
                WHBLogFreetypePrint("Dumping will start in 10 seconds...");
                WHBLogFreetypeDraw();
                sleep_for(10s);
            }
        }
    }

    std::unique_ptr<TransferInterface> interface = !USE_WUT_DEVOPTAB() ? std::unique_ptr<TransferInterface>{std::make_unique<Fat32Transfer>(config)} : std::make_unique<StubTransfer>(config);

    // Dump files
    startQueue(totalDumpSize);
    for (size_t i=0; i<queue.size(); i++) {
        std::string status("Currently dumping ");
        titleEntry& queueEntry = *queue[i];
        status += queueEntry.shortTitle;
        if (queue.size() > 1) {
            if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::GAME)) status += " (game ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::UPDATE)) status += " (update ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::DLC)) status += " (DLC ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SAVES)) status += " (save ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SYSTEM_APP)) status += " (app ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::CUSTOM)) status += " (custom ";
            else status += " (title "+std::to_string(i+1)+"/"+std::to_string(queue.size())+")";
        }
        status += "...";
        setDumpingStatus(status);

        bool cancelledDumping = false;
        std::filesystem::path currDir;
        bool dumpSuccess = processTitleEntry(queueEntry, config, [&currDir, &interface, &cancelledDumping, &queueEntry](WALK_EVENT event, const char *filename, const std::string &srcPath, const std::string &destPath) -> bool {
            if (event == WALK_EVENT::MAKE_PATH)
                return callback_makeDir(interface.get(), destPath, true);
            else if (event == WALK_EVENT::DIR_ENTER) {
                currDir = destPath;
                if (!callback_makeDir(interface.get(), destPath, false)) return false;
                if (!callback_moveDir(interface.get(), currDir.string())) return false;
            }
            else if (event == WALK_EVENT::DIR_LEAVE) {
                currDir = currDir.parent_path();
                return callback_moveDir(interface.get(), currDir.string());
            }
            else if (event == WALK_EVENT::FILE)
                return callback_copyFile(interface.get(), cancelledDumping, filename, srcPath, destPath);
            else if (event == WALK_EVENT::BUFFER)
                return callback_copyBuffer(interface.get(), cancelledDumping, filename, queueEntry.customFile->srcBuffer, queueEntry.customFile->srcBufferSize, destPath+"/"+filename);
            return true;
        });
        if (!dumpSuccess) {
            interface.reset();
            if (!cancelledDumping) {
                showErrorPrompt("Back To Main Menu");
            }
            uint8_t doDeleteFiles = showDialogPrompt("Delete the incomplete files from the currently transferring title?\nThis will make space to retry or dump something else.\nDepending on the size it might take a few minutes.\nAlready completed titles will not be deleted...", "Yes", "No");
            if (doDeleteFiles == 0) {
                uint64_t deletedCount = 0;
                auto showDeleteProgress = [&deletedCount]() {
                    if (deletedCount % 10 == 0) {
                        WHBLogFreetypeStartScreen();
                        WHBLogFreetypePrint("Deleting incomplete dumping files!");
                        WHBLogFreetypePrint("");
                        WHBLogFreetypePrint("This might take a few minutes if there's a lot of files...");
                        WHBLogFreetypePrint("Please don't turn your Wii U off!");
                        WHBLogFreetypePrint("");
                        WHBLogFreetypePrintf("Deleted %llu files so far...", deletedCount);
                        WHBLogFreetypeDrawScreen();
                    }
                    deletedCount++;
                };
                showDeleteProgress();
                if (!deleteTitleEntry(queueEntry, config, showDeleteProgress)) {
                    showErrorPrompt("Back To Main Menu");
                }
            }
            return false;
        }
    }
    return true;
}

void dumpMLC() {
    std::vector<std::shared_ptr<titleEntry>> queue = {
            std::make_shared<titleEntry>(titleEntry{.shortTitle = "MLC Dump", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01"), .outputPath = "/MLC Dump"}})
    };
    dumpingConfig mlcConfig = {.dumpTypes = DUMP_TYPE_FLAGS::CUSTOM};

    uint8_t selectedChoice = showDialogPrompt("Dumping the MLC can take 6-20 hours depending on its contents!\nMake sure that you have enough space available.", "Proceed", "Cancel");
    if (selectedChoice == 1) return;

    // Show the option screen
    if (!showOptionMenu(mlcConfig, false)) {
        return;
    }

    mlcConfig.scanTitleSize = false;
    if (dumpQueue(queue, mlcConfig)) showDialogPrompt("Successfully dumped entire MLC!", "OK");
}

bool dumpDisc() {
    int32_t mcpHandle = MCP_Open();
    if (mcpHandle < 0) {
        WHBLogPrint("Failed to open MPC to check for inserted disc titles");
        return false;
    }

    if (!checkForDiscTitles(mcpHandle)) {
        // Loop until a disk is found
        while(true) {
            WHBLogFreetypeStartScreen();
            WHBLogPrint("Looking for a game disc...");
            WHBLogPrint("Please insert one if you haven't already!");
            WHBLogPrint("");
            WHBLogPrint("Reinsert the disc if you started Dumpling");
            WHBLogPrint("with a disc already inserted!");
            WHBLogPrint("");
            WHBLogFreetypeScreenPrintBottom("===============================");
            WHBLogFreetypeScreenPrintBottom("\uE001 Button = Back to Main Menu");
            WHBLogFreetypeDrawScreen();
            sleep_for(100ms);

            updateInputs();
            if (pressedBack()) {
                MCP_Close(mcpHandle);
                return true;
            }
            if (checkForDiscTitles(mcpHandle)) {
                break;
            }
        }
    }
    MCP_Close(mcpHandle);

    // Scan disc titles this time
    WHBLogFreetypeClear();
    WHBLogPrint("Refreshing games list:");
    WHBLogPrint("");
    WHBLogFreetypeDraw();

    if (!mountDisc()) {
        showDialogPrompt("Error while mounting disc!", "OK");
        return false;
    }

    if (!loadTitles(false)) {
        showDialogPrompt("Error while scanning disc titles!", "OK");
        return false;
    }

    // Make a queue from game disc
    std::vector<std::shared_ptr<titleEntry>> queue;
    for (auto& title : installedTitles) {
        if (title->base && title->base->location == TITLE_LOCATION::DISC) {
            queue.emplace_back(std::ref(title));
            break;
        }
    }

    WHBLogFreetypeClear();
    WHBLogPrint("Currently inserted disc is:");
    WHBLogPrint(queue.begin()->get()->shortTitle.c_str());
    WHBLogFreetypePrint("");
    WHBLogFreetypePrint("Continuing to next step in 5 seconds...");
    WHBLogFreetypeDraw();
    sleep_for(5s);

    // Dump queue
    uint8_t dumpOnlyBase = showDialogPrompt("Do you want to dump the update and DLC files?\nIt might be faster when you want to:\n - Dump all updates/DLC at once, without disc swapping!\n - Through methods like Cemu's Online Features, DumpsterU etc.", "Yes", "No");
    dumpingConfig config = {
        .filterTypes = DUMP_TYPE_FLAGS::GAME,
        .dumpTypes = (dumpOnlyBase == 0) ? (DUMP_TYPE_FLAGS::GAME | DUMP_TYPE_FLAGS::UPDATE | DUMP_TYPE_FLAGS::DLC | DUMP_TYPE_FLAGS::SAVES) : (DUMP_TYPE_FLAGS::GAME | DUMP_TYPE_FLAGS::SAVES)
    };
    if (!showOptionMenu(config, true)) return true;

    auto startTime = std::chrono::system_clock::now();
    if (dumpQueue(queue, config)) {
        auto endTime = std::chrono::system_clock::now();
        auto elapsedDuration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        std::string finishedMsg = "Successfully dumped disc in\n"+formatElapsedTime(elapsedDuration)+"!";
        showDialogPrompt(finishedMsg.c_str(), "OK");
        return true;
    }
    else {
        showDialogPrompt("Failed to dump disc files...", "OK");
        return false;
    }
}

void dumpOnlineFiles() {
    std::vector<std::shared_ptr<titleEntry>> queue;
    dumpingConfig onlineConfig = {.dumpTypes = (DUMP_TYPE_FLAGS::CUSTOM)};

    // Loop until a valid account has been chosen
    if (!showOptionMenu(onlineConfig, true)) return;
    auto userIt = std::find_if(allUsers.begin(), allUsers.end(), [&onlineConfig](userAccount& user) {
        return user.persistentId == onlineConfig.accountId;
    });

    std::string accountIdStr = userIt->persistentIdString;
    std::string accountNameStr = normalizeFolderName(userIt->miiName);
    if (accountNameStr.empty()) accountNameStr = "Non-Ascii Username ["+accountIdStr+"]";

    titleEntry ecAccountInfoEntry{.shortTitle = "eShop Key File", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/usr/save/system/nim/ec"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/usr/save/system/nim/ec"}};
    titleEntry miiEntry{.shortTitle = "Mii Files", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/sys/title/0005001b/10056000/content"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/sys/title/0005001b/10056000/content"}};
    titleEntry ccertsEntry{.shortTitle = "ccerts Files", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/sys/title/0005001b/10054000/content/ccerts"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/sys/title/0005001b/10054000/content/ccerts"}};
    titleEntry scertsEntry{.shortTitle = "scerts Files", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/sys/title/0005001b/10054000/content/scerts"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/sys/title/0005001b/10054000/content/scerts"}};
    titleEntry accountsEntry{.shortTitle = "Selected Account", .customFolder = folderPart{.inputPath = convertToPosixPath((std::string("/vol/storage_mlc01/usr/save/system/act/") + accountIdStr).c_str()), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/usr/save/system/act/" + (onlineConfig.dumpAsDefaultUser ? "80000001" : accountIdStr)}};

    // Dump otp.bin and seeprom.bin
    WiiUConsoleOTP otpReturn;
    std::array<uint8_t, 512> seepromBuffer{};

    Mocha_ReadOTP(&otpReturn);
    Mocha_SEEPROMRead(seepromBuffer.data(), 0, seepromBuffer.size());

    titleEntry seepromFile{.shortTitle = "seeprom.bin File", .customFile = filePart{.srcBuffer = seepromBuffer.data(), .srcBufferSize = seepromBuffer.size(), .outputFolder = "/Online Files", .outputFile = "seeprom.bin"}};
    titleEntry otpFile{.shortTitle = "otp.bin File", .customFile = filePart{.srcBuffer = (uint8_t*)&otpReturn, .srcBufferSize = sizeof(otpReturn), .outputFolder = "/Online Files", .outputFile = "otp.bin"}};

    // Add custom title entries to queue
    queue.emplace_back(std::make_shared<titleEntry>(ecAccountInfoEntry));
    queue.emplace_back(std::make_shared<titleEntry>(miiEntry));
    queue.emplace_back(std::make_shared<titleEntry>(ccertsEntry));
    queue.emplace_back(std::make_shared<titleEntry>(scertsEntry));
    queue.emplace_back(std::make_shared<titleEntry>(accountsEntry));
    queue.emplace_back(std::make_shared<titleEntry>(seepromFile));
    queue.emplace_back(std::make_shared<titleEntry>(otpFile));

    if (dumpQueue(queue, onlineConfig)) showDialogPrompt("Successfully dumped all of the online files!", "OK");
    else showDialogPrompt("Failed to dump the online files...", "OK");
}

void cleanDumpingProcess() {
    WHBLogPrint("Cleaning up after dumping...");
    WHBLogFreetypeDraw();
    sleep_for(200ms);
    if (isDiscMounted() && !loadTitles(true)) {
        WHBLogPrint("Error while reloading titles after disc dumping...");
        WHBLogFreetypeDraw();
        sleep_for(2s);
    }
    unmountDisc();
}