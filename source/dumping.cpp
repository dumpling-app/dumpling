#include "dumping.h"
#include "progress.h"
#include "menu.h"
#include "filesystem.h"
#include "navigation.h"
#include "titles.h"
#include "users.h"

// Dumping Functions

#define BUFFER_SIZE_ALIGNMENT 64
#define BUFFER_SIZE (1024 * BUFFER_SIZE_ALIGNMENT)

bool copyFile(const char* filename, std::string srcPath, std::string destPath) {
    // Check if file is an actual file first
    struct stat fileStat;
    if (stat(srcPath.c_str(), &fileStat) == -1 || !S_ISREG(fileStat.st_mode)) {
        return true;
    }

    // Allocate buffer to copy bytes between
    uint8_t* copyBuffer = (uint8_t*)aligned_alloc(BUFFER_SIZE_ALIGNMENT, BUFFER_SIZE);
    if (copyBuffer == NULL) {
        setErrorPrompt("Couldn't allocate the memory to copy files!");
        return false;
    }

    // Open source file
    FILE* readHandle = fopen(srcPath.c_str(), "rb");
    if (readHandle == NULL) {
        //setErrorPrompt("Couldn't open the file to copy from!");
        free(copyBuffer);
        return true; // Ignore file open errors since symlinks
    }

    // Open the destination file
    FILE* writeHandle = fopen(destPath.c_str(), "wb");
    if (writeHandle == NULL) {
        setErrorPrompt("Couldn't open the file to copy to!");
        fclose(readHandle);
        free(copyBuffer);
        return false;
    }

    // Loop over input to copy
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    setFile(filename, fileStat.st_size);
    
    while((bytesRead = fread(copyBuffer, sizeof(uint8_t), BUFFER_SIZE, readHandle)) > 0) {
        bytesWritten = fwrite(copyBuffer, sizeof(uint8_t), bytesRead, writeHandle);
        // Check if the same amounts of bytes are written
        if (bytesWritten < bytesRead) {
            setErrorPrompt("Something went wrong during the fily copy where not all bytes were written!");
            free(copyBuffer);
            fclose(readHandle);
            fclose(writeHandle);
            return false;
        }
        setFileProgress(bytesWritten);
        showCurrentProgress();
        // Check whether the inputs break
        updateInputs();
        if (pressedBack()) {
            uint8_t selectedChoice = showDialogPrompt("Are you sure that you want to cancel the dumping process?", "Yes", "No");
            if (selectedChoice == 0) {
                setErrorPrompt("Couldn't delete files from SD card, please delete them manually.");
                free(copyBuffer);
                fclose(readHandle);
                fclose(writeHandle);
                return false;
            }
        }
    }

    free(copyBuffer);
    fclose(readHandle);
    fclose(writeHandle);
    return true;
}

bool copyFolder(std::string srcPath, std::string destPath, uint64_t* totalBytes) {
    // Open folder
    DIR* dirHandle;
    if ((dirHandle = opendir(srcPath.c_str())) == NULL) return true; // Ignore folder opening errors since they also happen when you open the special folders.

    // Append slash when last character isn't a slash
    if (totalBytes == nullptr) createPath(destPath.c_str());

    // Loop over directory contents
    struct dirent *dirEntry;
    while((dirEntry = readdir(dirHandle)) != NULL) {
        if (dirEntry->d_type == DT_REG) {
            if (totalBytes != nullptr) {
                // Get the amount of bytes for each file
                struct stat fileStat;
                if (stat(std::string(srcPath+"/"+dirEntry->d_name).c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
                    *totalBytes += fileStat.st_size;
                }
            }
            else {
                // Copy file
                if (!copyFile(dirEntry->d_name, srcPath+"/"+dirEntry->d_name, destPath+"/"+dirEntry->d_name)) {
                    closedir(dirHandle);
                    return false;
                }
            }
        }
        else if (dirEntry->d_type == DT_DIR) {
            // Ignore root and parent folder entries
            if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) continue;

            // Copy all the files in this subdirectory
            if (!copyFolder(srcPath+"/"+dirEntry->d_name, destPath+"/"+dirEntry->d_name, totalBytes)) {
                closedir(dirHandle);
                return false;
            }
        }
    }
    
    closedir(dirHandle);
    return true;
}

bool dumpTitle(titleEntry& entry, dumpingConfig& config, uint64_t* totalBytes) {
    if ((config.dumpTypes & dumpTypeFlags::GAME) == dumpTypeFlags::GAME && entry.hasBase) {
        if (!copyFolder(entry.base.path, getRootFromLocation(config.location)+"/dumpling"+entry.base.outputPath, totalBytes)) return false;
    }
    if ((config.dumpTypes & dumpTypeFlags::UPDATE) == dumpTypeFlags::UPDATE && entry.hasUpdate) {
        if (!copyFolder(entry.update.path, getRootFromLocation(config.location)+"/dumpling"+entry.update.outputPath, totalBytes)) return false;
    }
    if ((config.dumpTypes & dumpTypeFlags::DLC) == dumpTypeFlags::DLC && entry.hasDLC) {
        if (!copyFolder(entry.dlc.path, getRootFromLocation(config.location)+"/dumpling"+entry.dlc.outputPath, totalBytes)) return false;
    }
    if ((config.dumpTypes & dumpTypeFlags::COMMONSAVE) == dumpTypeFlags::COMMONSAVE && !entry.commonSave.path.empty()) {
        if (!copyFolder(entry.commonSave.path, getRootFromLocation(config.location)+"/dumpling/Saves/"+entry.normalizedTitle+"/common", totalBytes)) return false;
    }
    if ((config.dumpTypes & dumpTypeFlags::SAVE) == dumpTypeFlags::SAVE && !entry.saves.empty()) {
        for (auto& save : entry.saves) {
            if (save.account->persistentId == config.accountID) {
                if (!copyFolder(save.path, getRootFromLocation(config.location)+"/dumpling/Saves/"+entry.normalizedTitle+"/80000001", totalBytes)) return false;
            }
        }
    }
    if ((config.dumpTypes & dumpTypeFlags::CUSTOM) == dumpTypeFlags::CUSTOM && entry.hasBase) {
        if (!copyFolder(entry.base.path, getRootFromLocation(config.location)+"/dumpling"+entry.base.outputPath, totalBytes)) return false;
    }
    return true;
}

bool dumpQueue(std::vector<std::reference_wrapper<titleEntry>>& queue, dumpingConfig& config) {
    // Show message about the scanning process which freezes the game
    clearScreen();
    WHBLogPrint("Starting the dumping process!");
    WHBLogPrint("This might take a few minutes if you choose a lot of titles...");
    WHBLogConsoleDraw();

    // Scan folder to get the full queue size
    uint64_t totalDumpSize = 0;
    for (titleEntry& entry : queue) {
        if (!dumpTitle(entry, config, &totalDumpSize)) {
            showErrorPrompt("Exit to Main Menu");
            showDialogPrompt("Failed while trying to get the size of a title", "Exit to Main Menu");
            return false;
        }
    }

    // Check if there's enough space on the storage location and otherwise give a warning
    uint64_t sizeAvailable = getFreeSpace(getRootFromLocation(config.location).c_str());
    if (sizeAvailable < totalDumpSize) {
        std::string freeSpaceWarning;
        freeSpaceWarning += "You only have ";
        freeSpaceWarning += formatByteSize(sizeAvailable);
        freeSpaceWarning += ", while you require ";
        freeSpaceWarning += formatByteSize(totalDumpSize);
        freeSpaceWarning += " of free space!\n";
        if (sizeAvailable == 0) freeSpaceWarning += "Make sure that your storage device is still plugged in!";
        else freeSpaceWarning += "Make enough space, or dump one game at a time.";
        showDialogPrompt(freeSpaceWarning.c_str(), "Exit to Main Menu");
        return false;
    }

    // Dump title
    startQueue(totalDumpSize);
    for (size_t i=0; i<queue.size(); i++) {
        std::string status("Currently dumping ");
        status += queue[i].get().shortTitle;
        if (queue.size() > 1) {
            if ((config.filterTypes & dumpTypeFlags::GAME) == dumpTypeFlags::GAME) status += " (game ";
            else if ((config.filterTypes & dumpTypeFlags::UPDATE) == dumpTypeFlags::UPDATE) status += " (update ";
            else if ((config.filterTypes & dumpTypeFlags::DLC) == dumpTypeFlags::DLC) status += " (DLC ";
            else if ((config.filterTypes & dumpTypeFlags::SAVE) == dumpTypeFlags::SAVE) status += " (save ";
            else if ((config.filterTypes & dumpTypeFlags::SYSTEM_APP) == dumpTypeFlags::SYSTEM_APP) status += " (app ";
            else status += " (title ";
            status += std::to_string(i+1);
            status += "/";
            status += std::to_string(queue.size());
            status += ")";
        }
        status += "...";

        setDumpingStatus(status);
        if (!dumpTitle(queue[i], config, nullptr)) {
            showErrorPrompt("Back to Main Menu");
            return false;
        }
    }

    return true;
}

void dumpMLC() {
    dumpingConfig mlcConfig = {.dumpTypes = dumpTypeFlags::CUSTOM};
    titleEntry mlcEntry{.hasBase = true, .base = {.path = "storage_mlc01:/", .outputPath = "/MLC Dump"}};
    
    uint8_t selectedChoice = showDialogPrompt("Dumping the MLC can take up to 6 hours!\nMake sure that you have enough space available.", "Proceed", "Cancel");
    if (selectedChoice == 1) return;

    // Show the option screen
    if (!showOptionMenu(mlcConfig, false)) {
        return;
    }

    setDumpingStatus("Currently dumping the whole MLC...");
    if (!dumpTitle(mlcEntry, mlcConfig, nullptr)) {
        showErrorPrompt("Back to Main Menu");
        return;
    }
}

bool dumpDisc() {
    // Loop until a disk is found
    std::vector<std::reference_wrapper<titleEntry>> queue;
    while(queue.empty()) {
        mountDisc();
        // Reload titles
        if (!loadTitles(false)) {
            showDialogPrompt("A fatal error occured while trying to check for game discs!", "OK");
            showDialogPrompt("Dumpling will now forcefully be closed...", "OK");
            return false;
        }

        // Check if there's a disc title
        for (auto& title : installedTitles) {
            if (title.hasBase && title.base.location == titleLocation::Disc) queue.emplace_back(std::ref(title));
        }

        // Print menu
        clearScreen();
        WHBLogPrint("Looking for a game disc...");
        WHBLogPrint("Please insert one if you haven't already!");
        WHBLogPrint("");
        WHBLogPrint("===============================");
        WHBLogPrint("B Button = Back to Main Menu");
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(20));

        updateInputs();
        if (pressedBack()) {
            unmountDisc();
            return true;
        }
    }

    // Dump queue
    dumpingConfig config = {.dumpTypes = (dumpTypeFlags::GAME | dumpTypeFlags::UPDATE | dumpTypeFlags::DLC | dumpTypeFlags::SAVE)};
    dumpQueue(queue, config);
    unmountDisc();
    return true;
}

void dumpOnlineFiles() {
    std::vector<std::reference_wrapper<titleEntry>> queue;
    dumpingConfig onlineConfig = {.dumpTypes = (dumpTypeFlags::CUSTOM)};
    titleEntry ccertsEntry{.shortTitle = "ccerts folder", .hasBase = true, .base = {.path = "storage_mlc01:/sys/title/0005001b/10054000/content/ccerts", .outputPath = "/Online Files/mlc01/sys/title/0005001b/10054000/content/ccerts"}};
    titleEntry scertsEntry{.shortTitle = "scerts folder", .hasBase = true, .base = {.path = "storage_mlc01:/sys/title/0005001b/10054000/content/scerts", .outputPath = "/Online Files/mlc01/sys/title/0005001b/10054000/content/scerts"}};
    titleEntry accountsEntry{.shortTitle = "Selected Account", .hasBase = true, .base = {.path = "", .outputPath = "/Online Files/mlc01/usr/save/system/act/80000001"}};

    // Loop until a valid account has been chosen, or when 
    while(accountsEntry.base.path.empty()) {
        if (!showOptionMenu(onlineConfig, true)) return;

        // Check if the selected user has 
        for (auto& user : allUsers) {
            if (user.persistentId == onlineConfig.accountID) {
                if (!user.networkAccount) showDialogPrompt("This account doesn't have a NNID connected to it!\n\nSteps on how to connect/create a NNID to your Account:\n - Click the Mii icon on the Wii U's homescreen.\n - Click on the Link a Nintendo Network ID option.\n - Return to Dumpling.", "OK");
                else if (!user.passwordCached) showDialogPrompt("Your password isn't saved!\n\nSteps on how to save your password in your account:\n - Click your Mii icon on the Wii U's homescreen.\n - Enable the Save Password option.\n - Return to Dumpling.", "OK");
                else {
                    // Create path to account folder
                    std::ostringstream stream;
                    stream << "storage_mlc01:/usr/save/system/act/";
                    stream << std::hex << user.persistentId;
                    accountsEntry.base.path = stream.str();
                }
                break;
            }
        }
    }

    // Add (custom) title entries to queue
    queue.emplace_back(getTitleWithName("Friend List"));
    queue.emplace_back(std::ref(ccertsEntry));
    queue.emplace_back(std::ref(scertsEntry));
    queue.emplace_back(std::ref(accountsEntry));

    if (!dumpQueue(queue, onlineConfig)) showDialogPrompt("Failed to dump the online files...", "OK");
    else showDialogPrompt("Successfully dumped all of the online files!", "OK");
}

void dumpCompatibilityFiles() {
    std::vector<std::reference_wrapper<titleEntry>> queue;
    dumpingConfig compatConfig = {.dumpTypes = (dumpTypeFlags::CUSTOM)};
    titleEntry miiEntry{.shortTitle = "Mii Files", .hasBase = true, .base = {.path = "storage_mlc01:/sys/title/0005001b/10056000/content", .outputPath = "/Compatibility Files/mlc01/sys/title/0005001b/10056000/content"}};
    titleEntry keyboardEntry{.shortTitle = "Keyboard Files", .hasBase = true, .base = {.path = "storage_mlc01:/sys/title/0005001b/1004f000", .outputPath = "/Compatibility Files/mlc01/sys/title/0005001b/1004f000"}};
    titleEntry rplEntry{.shortTitle = "RPL Files", .hasBase = true, .base = {.path = "storage_mlc01:/sys/title/00050010/1000400a", .outputPath = "/Compatibility Files/mlc01/sys/title/00050010/1000400a"}};

    if (!showOptionMenu(compatConfig, false)) return;

    queue.emplace_back(std::ref(miiEntry));
    queue.emplace_back(std::ref(keyboardEntry));
    queue.emplace_back(std::ref(rplEntry));

    if (!dumpQueue(queue, compatConfig)) showDialogPrompt("Failed to dump the compatibility files...", "OK");
    else showDialogPrompt("Successfully dumped all of the compatibility files!", "OK");
}
