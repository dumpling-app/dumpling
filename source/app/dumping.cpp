#include "dumping.h"
#include "progress.h"
#include "menu.h"
#include "filesystem.h"
#include "navigation.h"
#include "titles.h"
#include "users.h"
#include "cfw.h"
#include "gui.h"
#include "http.h"

#include "interfaces/transfer.h"
#include "interfaces/fat32.h"
#include "interfaces/stub.h"

#include <dirent.h>

#include <mocha/mocha.h>
#include <mocha/otp.h>
#include <sys/unistd.h>

// Dumping Functions
#define BUFFER_SIZE_ALIGNMENT 64
#define BUFFER_SIZE (1024 * BUFFER_SIZE_ALIGNMENT * 4)
// not sure which one is better
static_assert(BUFFER_SIZE % 0x8000 == 0, "Buffer size needs to be multiple of 0x8000 to properly read disc sections");
static_assert(BUFFER_SIZE % 0x10000 == 0, "Buffer size needs to be multiple of 0x10000 to properly read disc sections");


enum class WALK_EVENT {
    FILE,
    BUFFER,
    MAKE_PATH,
    DIR_ENTER,
    DIR_LEAVE
};

bool callback_scanFile(uint64_t& totalBytes, const char* filename, const std::string& srcPath) {
    struct stat fileStat {};
    if (lstat(srcPath.c_str(), &fileStat) == -1) {
        std::wstring errorMessage;
        errorMessage += L"Failed to retrieve info from source file!\n";
        if (errno == EIO) errorMessage += L"For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
        errorMessage += L"\nDetails:\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" when getting stats for:\n";
        errorMessage += toWstring(srcPath);
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

bool readFile(const std::string& path, std::vector<uint8_t>& data) {
    data.clear();

    struct stat fileStat {};
    if (lstat(path.c_str(), &fileStat) == -1 || !S_ISREG(fileStat.st_mode)) {
        std::wstring errorMessage;
        errorMessage += L"Failed to retrieve info from source file!\n";
        errorMessage += L"\nDetails:\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" when getting stats for:\n";
        errorMessage += toWstring(path);
        setErrorPrompt(errorMessage);
        return false;
    }

    int handle = open(path.c_str(), O_RDONLY);
    if (handle == -1) {
        std::wstring errorMessage;
        errorMessage += L"Couldn't open the file to read from!\n";
        errorMessage += L"\nDetails:\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" after opening file!\n";
        errorMessage += toWstring(path);
        setErrorPrompt(errorMessage);
        return false;
    }

    data.resize(fileStat.st_size);
    ssize_t bytesRead = read(handle, data.data(), fileStat.st_size);
    if (bytesRead == -1) {
        close(handle);
        std::wstring errorMessage;
        errorMessage += L"Failed to read all data from this file!\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" when reading data from:\n";
        errorMessage += toWstring(path);
        setErrorPrompt(errorMessage);
        return false;
    }

    close(handle);

    return true;
}

#define O_OPEN_UNENCRYPTED 0x4000000
bool callback_copyFile(TransferInterface* interface, bool& cancelledDumping, const char* filename, const std::string& srcPath, const std::string& destPath) {
    // Check if file is an actual file first
    struct stat fileStat {};
    if (lstat(srcPath.c_str(), &fileStat) == -1) {
        std::wstring errorMessage;
        errorMessage += L"Failed to retrieve info from source file!\n";
        if (errno == EIO) errorMessage += L"For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
        errorMessage += L"\nDetails:\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" when getting stats for:\n";
        errorMessage += toWstring(srcPath);
        setErrorPrompt(errorMessage);
        return false;
    }

    int readHandle = open(srcPath.c_str(), srcPath.length() >= 10 && srcPath.ends_with(".nfs") ? O_OPEN_UNENCRYPTED | O_RDONLY : O_RDONLY);
    if (readHandle == -1) {
        std::wstring errorMessage;
        errorMessage += L"Couldn't open the file to copy from!\n";
        if (errno == EIO) errorMessage += L"For discs: Make sure that its clean!\nDumping is very sensitive to tiny errors!\n";
        errorMessage += L"\nDetails:\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" after opening file!\n";
        errorMessage += toWstring(srcPath);
        setErrorPrompt(errorMessage);
        return false;
    }

    setFile(filename, fileStat.st_size);

    while (true) {
        uint8_t* copyBuffer = (uint8_t*)aligned_alloc(BUFFER_SIZE_ALIGNMENT, BUFFER_SIZE);
        if (copyBuffer == nullptr) {
            close(readHandle);
            setErrorPrompt(L"Failed to allocate memory for chunk buffer!");
            return false;
        }

        ssize_t bytesRead = read(readHandle, copyBuffer, BUFFER_SIZE);
        if (bytesRead == -1) {
            free(copyBuffer);
            copyBuffer = nullptr;
            close(readHandle);
            std::wstring errorMessage;
            errorMessage += L"Failed to read all data from this file!\n";
            if (errno == EIO) errorMessage += L"For discs: Make sure that its clean!\nDumping is very sensitive to tiny errors!\n";
            errorMessage += L"Error " + std::to_wstring(errno) + L" when reading data from:\n";
            errorMessage += toWstring(srcPath);
            setErrorPrompt(errorMessage);
            return false;
        }

        bool endOfFile = bytesRead < BUFFER_SIZE;
        if (!interface->submitWriteFile(destPath, fileStat.st_size, copyBuffer, bytesRead, endOfFile)) {
            free(copyBuffer);
            close(readHandle);
            setErrorPrompt(*interface->getStopError());
            return false;
        }

        setFileProgress(bytesRead);
        showCurrentProgress();
        // Check whether the inputs break
        updateInputs();
        if (pressedBack()) {
            uint8_t selectedChoice = showDialogPrompt(L"Are you sure that you want to cancel the dumping process?", L"Yes", L"No");
            if (selectedChoice != 0) continue;
            WHBLogFreetypeClear();
            WHBLogPrint("Quitting dumping process...");
            WHBLogPrint("The app (likely) isn't frozen!");
            WHBLogPrint("This should take a minute at most!");
            WHBLogFreetypeDraw();
            close(readHandle);
            cancelledDumping = true;
            return false;
        }

        if (endOfFile) {
            break;
        }
    }

    close(readHandle);
    return true;
}

bool callback_copyBuffer(TransferInterface* interface, bool& cancelledDumping, const char* filename, uint8_t* srcBuffer, uint64_t bufferSize, const std::string& destPath) {
    setFile(std::filesystem::path(destPath).filename().c_str(), bufferSize);

    // Copy source buffer into aligned copy buffer, then write it in parts
    uint64_t uncopiedBytes = bufferSize;
    while (true) {
        uint8_t* copyBuffer = (uint8_t*)aligned_alloc(BUFFER_SIZE_ALIGNMENT, BUFFER_SIZE);
        if (copyBuffer == nullptr) {
            setErrorPrompt(L"Failed to allocate memory for chunk buffer!");
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
            uint8_t selectedChoice = showDialogPrompt(L"Are you sure that you want to cancel the dumping process?", L"Yes", L"No");
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
    if (dirHandle = opendir((srcPath + "/").c_str()); dirHandle == nullptr) {
        std::wstring errorMessage;
        errorMessage += L"Failed to open folder to read content from!\n";
        if (errno == EIO) errorMessage += L"For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
        errorMessage += L"\nDetails:\n";
        errorMessage += L"Error " + std::to_wstring(errno) + L" while opening folder:\n";
        errorMessage += toWstring(srcPath);
        setErrorPrompt(errorMessage);
        return false;
    }

    // Loop over directory contents
    while (true) {
        errno = 0;
        struct dirent* dirEntry = readdir(dirHandle);
        if (dirEntry != nullptr) {
            std::string entrySrcPath = srcPath + "/" + dirEntry->d_name;
            std::string entryDstPath = destPath + "/" + dirEntry->d_name;
            if (dirEntry->d_type == DT_LNK) {
                continue;
            }
            else if (dirEntry->d_type == DT_REG) {
                // Copy file
                if (!callback(WALK_EVENT::FILE, dirEntry->d_name, entrySrcPath, entryDstPath)) {
                    closedir(dirHandle);
                    return false;
                }
            }
            else if (dirEntry->d_type == DT_DIR) {
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
            std::wstring errorMessage =
                L"The Wii U had an issue while reading the disc(?)!\n"
                L"Make sure that its clean!\nDumping is very sensitive to tiny issues!\n"
                L"Unfortunately, disc requires reinsertion to retry!\nTry restarting Dumpling and trying again.\n\nDetails:\n"
                L"Error " + std::to_wstring(errno) + L" while iterating folder contents:\n"
                + toWstring(srcPath);
            setErrorPrompt(errorMessage);
            return false;
        }
        else if (errno != 0) {
            std::wstring errorMessage =
                L"Unknown error while reading disc contents!\n"
                L"You might want to restart your console and try again.\n\nDetails:\n"
                L"Error " + std::to_wstring(errno) + L" while iterating folder contents:\n"
                + toWstring(srcPath);
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
        if (!Fat32Transfer::deletePath(config.dumpTarget + entry.customFile->outputFolder + "/" + entry.customFile->outputFile, callback_updateGUI)) return false;
    }
    return true;
}

bool dumpQueue(std::vector<std::shared_ptr<titleEntry>>& queue, dumpingConfig& config, std::optional<dumpFileFilter*> filter) {
    // Delete previously dumped files
    if (config.dumpMethod == DUMP_METHOD::FAT && !USE_WUT_DEVOPTAB()) {
        uint64_t deletedCount = 0;
        auto showDeleteProgress = [&deletedCount]() {
            if (deletedCount % 10 == 0) {
                WHBLogFreetypeStartScreen();
                WHBLogPrint("Checking for existing files!");
                WHBLogFreetypePrint(L"");
                WHBLogFreetypePrint(L"This might take a few minutes if there's a lot of files...");
                WHBLogFreetypePrint(L"Please don't turn your Wii U off!");
                WHBLogFreetypePrint(L"");
                WHBLogFreetypePrintf(L"Found and deleted %llu files...", deletedCount);
                WHBLogFreetypeDrawScreen();
            }
            deletedCount++;
            };
        showDeleteProgress();
        for (auto& entry : queue) {
            if (!deleteTitleEntry(*entry, config, showDeleteProgress)) {
                showErrorPrompt(L"Exit to Main Menu");
                return false;
            }
        }
    }

    // Scan title size
    uint64_t totalDumpSize = 0;
    if (config.dumpMethod == DUMP_METHOD::FAT && config.scanTitleSize) {
        // Scan folder to get the full queue size
        for (size_t i = 0; i < queue.size(); i++) {
            // Show message about the scanning process which freezes the GUI
            WHBLogFreetypeStartScreen();
            WHBLogFreetypePrint(L"Scanning the dump size!");
            WHBLogFreetypePrint(L"This might take a few minutes if you selected a lot of titles...");
            WHBLogFreetypePrint(L"Your Wii U isn't frozen in that case!");
            WHBLogFreetypePrint(L"");
            WHBLogPrintf("Scanning %S... (title %lu / %lu)", queue[i]->shortTitle.c_str(), i + 1, queue.size());
            WHBLogFreetypePrint(L"");
            WHBLogFreetypeScreenPrintBottom(L"===============================");
            WHBLogFreetypeScreenPrintBottom(L"\uE001 Button = Cancel scanning and just do dumping");
            WHBLogFreetypeDraw();
            titleEntry& queueEntry = *queue[i];
            bool cancelledDumping = false;
            bool scanSuccess = processTitleEntry(queueEntry, config, [&totalDumpSize, &cancelledDumping, &queueEntry](WALK_EVENT event, const char* filename, const std::string& srcPath, const std::string& destPath) -> bool {
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
                showErrorPrompt(L"Back To Main Menu");
                showDialogPrompt(L"Failed while trying to scan the dump for its size!", L"Exit to Main Menu");
                return false;
            }
        }

        if (totalDumpSize > 0) {
            // Check if there's enough space on the storage location and otherwise give a warning
            uint64_t sizeAvailable = !USE_WUT_DEVOPTAB() ? Fat32Transfer::getDriveSpace(config.dumpTarget) : std::numeric_limits<uint64_t>::max();
            if (sizeAvailable < totalDumpSize) {
                std::wstring freeSpaceWarning;
                freeSpaceWarning += L"You only have " + formatByteSize(sizeAvailable) + L", while you require " + formatByteSize(totalDumpSize) + L" of free space!\n";
                if (sizeAvailable == 0) freeSpaceWarning += L"Make sure that your storage device is still plugged in!";
                else freeSpaceWarning += L"Make enough space, or dump one game at a time.";
                showDialogPrompt(freeSpaceWarning.c_str(), L"Exit to Main Menu");
                return false;
            }
            else {
                WHBLogFreetypeClear();
                WHBLogFreetypePrintf(L"Dump is %S while selected location has %S available!", formatByteSize(totalDumpSize).c_str(), formatByteSize(sizeAvailable).c_str());
                WHBLogFreetypePrint(L"Dumping will start in 10 seconds...");
                WHBLogFreetypeDraw();
                sleep_for(10s);
            }
        }
    }

    std::unique_ptr<TransferInterface> interface = !USE_WUT_DEVOPTAB() ? std::unique_ptr<TransferInterface>{std::make_unique<Fat32Transfer>(config)} : std::make_unique<StubTransfer>(config);

    // Dump files
    startQueue(totalDumpSize);
    for (size_t i = 0; i < queue.size(); i++) {
        std::wstring status(L"Currently dumping ");
        titleEntry& queueEntry = *queue[i];
        status += queueEntry.shortTitle;
        if (queue.size() > 1) {
            if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::GAME)) status += L" (game ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::UPDATE)) status += L" (update ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::DLC)) status += L" (DLC ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SAVES)) status += L" (save ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SYSTEM_APP)) status += L" (app ";
            else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::CUSTOM)) status += L" (custom ";
            else status += L" (title " + std::to_wstring(i + 1) + L"/" + std::to_wstring(queue.size()) + L")";
        }
        status += L"...";
        setDumpingStatus(status);

        bool cancelledDumping = false;
        std::filesystem::path currDir;
        bool dumpSuccess = processTitleEntry(queueEntry, config, [&currDir, &interface, &cancelledDumping, &queueEntry, &filter](WALK_EVENT event, const char* filename, const std::string& srcPath, const std::string& destPath) -> bool {
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
            else if (event == WALK_EVENT::FILE) {

                if (filter.has_value()) {
                    auto& fileFilter = filter.value();
                    bool hasMatched = false;

                    std::string filenameStr(filename);
                    for (auto& ext : fileFilter->extensions) {
                        if (filenameStr.ends_with(ext)) {
                            hasMatched = true;
                            break;
                        }
                    }

                    if (!hasMatched) {
                        for (auto& name : fileFilter->fileNames) {
                            if (filenameStr.compare(name) == 0) {
                                hasMatched = true;
                                break;
                            }
                        }
                    }

                    if (hasMatched)
                        fileFilter->outMatchedFiles.push_back(srcPath);

                }


                return callback_copyFile(interface.get(), cancelledDumping, filename, srcPath, destPath);
            }
            else if (event == WALK_EVENT::BUFFER)
                return callback_copyBuffer(interface.get(), cancelledDumping, filename, queueEntry.customFile->srcBuffer, queueEntry.customFile->srcBufferSize, destPath + "/" + filename);
            return true;
            });
        if (!dumpSuccess) {
            interface.reset();
            if (!cancelledDumping) {
                showErrorPrompt(L"Back To Main Menu");
            }
            uint8_t doDeleteFiles = showDialogPrompt(L"Delete the incomplete files from the currently transferring title?\nThis will make space to retry or dump something else.\nDepending on the size it might take a few minutes.\nAlready completed titles will not be deleted...", L"Yes", L"No");
            if (doDeleteFiles == 0) {
                uint64_t deletedCount = 0;
                auto showDeleteProgress = [&deletedCount]() {
                    if (deletedCount % 10 == 0) {
                        WHBLogFreetypeStartScreen();
                        WHBLogFreetypePrint(L"Deleting incomplete dumping files!");
                        WHBLogFreetypePrint(L"");
                        WHBLogFreetypePrint(L"This might take a few minutes if there's a lot of files...");
                        WHBLogFreetypePrint(L"Please don't turn your Wii U off!");
                        WHBLogFreetypePrint(L"");
                        WHBLogFreetypePrintf(L"Deleted %llu files so far...", deletedCount);
                        WHBLogFreetypeDrawScreen();
                    }
                    deletedCount++;
                    };
                showDeleteProgress();
                if (!deleteTitleEntry(queueEntry, config, showDeleteProgress)) {
                    showErrorPrompt(L"Back To Main Menu");
                }
            }
            return false;
        }
    }
    return true;
}

void dumpMLC() {
    std::vector<std::shared_ptr<titleEntry>> queue = {
            std::make_shared<titleEntry>(titleEntry{.shortTitle = L"MLC Dump", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01"), .outputPath = "/MLC Dump"}})
    };
    dumpingConfig mlcConfig = { .dumpTypes = DUMP_TYPE_FLAGS::CUSTOM };

    uint8_t selectedChoice = showDialogPrompt(L"Dumping the MLC can take 6-20 hours depending on its contents!\nMake sure that you have enough space available.", L"Proceed", L"Cancel");
    if (selectedChoice == 1) return;

    // Show the option screen
    if (!showOptionMenu(mlcConfig, false)) {
        return;
    }

    mlcConfig.scanTitleSize = false;
    if (dumpQueue(queue, mlcConfig)) showDialogPrompt(L"Successfully dumped entire MLC!", L"OK");
}

bool dumpDisc() {
    int32_t mcpHandle = MCP_Open();
    if (mcpHandle < 0) {
        WHBLogPrint("Failed to open MPC to check for inserted disc titles");
        return false;
    }

    if (!checkForDiscTitles(mcpHandle)) {
        // Loop until a disk is found
        while (true) {
            WHBLogFreetypeStartScreen();
            WHBLogPrint("Looking for a game disc...");
            WHBLogPrint("Please insert one if you haven't already!");
            WHBLogPrint("");
            WHBLogPrint("Reinsert the disc if you started Dumpling");
            WHBLogPrint("with a disc already inserted!");
            WHBLogPrint("");
            WHBLogFreetypeScreenPrintBottom(L"===============================");
            WHBLogFreetypeScreenPrintBottom(L"\uE001 Button = Back to Main Menu");
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
        showDialogPrompt(L"Error while mounting disc!", L"OK");
        return false;
    }

    if (!loadTitles(false)) {
        showDialogPrompt(L"Error while scanning disc titles!", L"OK");
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
    WHBLogFreetypePrint(L"Currently inserted disc is:");
    WHBLogFreetypePrint(queue.begin()->get()->shortTitle.c_str());
    WHBLogFreetypePrint(L"");
    WHBLogFreetypePrint(L"Continuing to next step in 5 seconds...");
    WHBLogFreetypeDraw();
    sleep_for(5s);

    // Dump queue
    uint8_t dumpOnlyBase = showDialogPrompt(L"Do you want to dump the update and DLC files?\nIt might be faster when you want to:\n - Dump all updates/DLC at once, without disc swapping!\n - Through methods like Cemu's Online Features, DumpsterU etc.", L"Yes", L"No");
    dumpingConfig config = {
        .filterTypes = DUMP_TYPE_FLAGS::GAME,
        .dumpTypes = (dumpOnlyBase == 0) ? (DUMP_TYPE_FLAGS::GAME | DUMP_TYPE_FLAGS::UPDATE | DUMP_TYPE_FLAGS::DLC | DUMP_TYPE_FLAGS::SAVES) : (DUMP_TYPE_FLAGS::GAME | DUMP_TYPE_FLAGS::SAVES)
    };
    if (!showOptionMenu(config, true)) return true;

    auto startTime = std::chrono::system_clock::now();
    if (dumpQueue(queue, config)) {
        auto endTime = std::chrono::system_clock::now();
        auto elapsedDuration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        std::wstring finishedMsg = L"Successfully dumped disc in\n" + formatElapsedTime(elapsedDuration) + L"!";
        showDialogPrompt(finishedMsg.c_str(), L"OK");
        return true;
    }
    else {
        showDialogPrompt(L"Failed to dump disc files...", L"OK");
        return false;
    }
}

void dumpOnlineFiles() {
    std::vector<std::shared_ptr<titleEntry>> queue;
    dumpingConfig onlineConfig = { .dumpTypes = (DUMP_TYPE_FLAGS::CUSTOM) };

    // Loop until a valid account has been chosen
    if (!showOptionMenu(onlineConfig, true)) return;
    auto userIt = std::find_if(allUsers.begin(), allUsers.end(), [&onlineConfig](userAccount& user) {
        return user.persistentId == onlineConfig.accountId;
        });

    std::string accountIdStr = userIt->persistentIdString;
    std::string accountNameStr = normalizeFolderName(userIt->miiName);
    if (accountNameStr.empty()) accountNameStr = "Non-Ascii Username [" + accountIdStr + "]";

    titleEntry ecAccountInfoEntry{ .shortTitle = L"eShop Key File", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/usr/save/system/nim/ec"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/usr/save/system/nim/ec"} };
    titleEntry miiEntry{ .shortTitle = L"Mii Files", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/sys/title/0005001b/10056000/content"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/sys/title/0005001b/10056000/content"} };
    titleEntry ccertsEntry{ .shortTitle = L"ccerts Files", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/sys/title/0005001b/10054000/content/ccerts"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/sys/title/0005001b/10054000/content/ccerts"} };
    titleEntry scertsEntry{ .shortTitle = L"scerts Files", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/sys/title/0005001b/10054000/content/scerts"), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/sys/title/0005001b/10054000/content/scerts"} };
    titleEntry accountsEntry{ .shortTitle = L"Selected Account", .customFolder = folderPart{.inputPath = convertToPosixPath((std::string("/vol/storage_mlc01/usr/save/system/act/") + accountIdStr).c_str()), .outputPath = "/Online Files/" + accountNameStr + "/mlc01/usr/save/system/act/" + (onlineConfig.dumpAsDefaultUser ? "80000001" : accountIdStr)} };
    queue.emplace_back(std::make_shared<titleEntry>(ecAccountInfoEntry));
    queue.emplace_back(std::make_shared<titleEntry>(miiEntry));
    queue.emplace_back(std::make_shared<titleEntry>(ccertsEntry));
    queue.emplace_back(std::make_shared<titleEntry>(scertsEntry));
    queue.emplace_back(std::make_shared<titleEntry>(accountsEntry));

    // Dump otp.bin and seeprom.bin
    WiiUConsoleOTP otpReturn;
    std::array<uint8_t, 512> seepromBuffer{};

    Mocha_ReadOTP(&otpReturn);
    Mocha_SEEPROMRead(seepromBuffer.data(), 0, seepromBuffer.size());

    titleEntry seepromFile{ .shortTitle = L"seeprom.bin File", .customFile = filePart{.srcBuffer = seepromBuffer.data(), .srcBufferSize = seepromBuffer.size(), .outputFolder = "/Online Files", .outputFile = "seeprom.bin"} };
    titleEntry otpFile{ .shortTitle = L"otp.bin File", .customFile = filePart{.srcBuffer = (uint8_t*)&otpReturn, .srcBufferSize = sizeof(otpReturn), .outputFolder = "/Online Files", .outputFile = "otp.bin"} };
    queue.emplace_back(std::make_shared<titleEntry>(seepromFile));
    queue.emplace_back(std::make_shared<titleEntry>(otpFile));


    if (dumpQueue(queue, onlineConfig)) showDialogPrompt(L"Successfully dumped all of the online files!", L"OK");
    else showDialogPrompt(L"Failed to dump the online files...", L"OK");
}

void dumpSpotpass() {
    std::vector<std::shared_ptr<titleEntry>> queue;
    dumpingConfig spotpassConfig = { .dumpTypes = (DUMP_TYPE_FLAGS::CUSTOM) };

    if (!showOptionMenu(spotpassConfig, false)) return;

    titleEntry slcSpotpassDir{ .shortTitle = L"SpotPass Directory", .customFolder = folderPart{.inputPath = convertToPosixPath("/vol/storage_mlc01/usr/save/system/boss"), .outputPath = "/SpotPass Files"} };
    queue.emplace_back(std::make_shared<titleEntry>(slcSpotpassDir));

    dumpFileFilter filter{ .fileNames = {"task.db"} };
    if (dumpQueue(queue, spotpassConfig, &filter)) showDialogPrompt(L"Successfully dumped all of the SpotPass files!", L"OK");
    else showDialogPrompt(L"Failed to dump the SpotPass files...", L"OK");

    wchar_t promptMessage[256];
    swprintf(promptMessage, 256, L"Do you want to anonymously upload the SpotPass files to an archival server?\nIt will upload %d files of 1MB each!\n", filter.outMatchedFiles.size());
    uint8_t doUploadFiles = showDialogPrompt(promptMessage, L"Yes", L"No");
    if (doUploadFiles == 0) {
        for (auto& taskFilepath : filter.outMatchedFiles) {
            std::vector<uint8_t> data;
            int resCode = -1;
            if (readFile(taskFilepath, data)) {
                if (http_uploadFile("https://bossarchive.raregamingdump.ca/api/upload/wup", data, resCode)) {
                    WHBLogPrintf("Uploaded file to server! %s (%d)", taskFilepath.c_str(), resCode);
                }
                else {
                    WHBLogPrintf("Failed to upload file to server! %s", taskFilepath.c_str());
                }
            }
            else {
                WHBLogPrintf("Failed to read file to upload! %s", taskFilepath.c_str());
            }
        }
    }

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