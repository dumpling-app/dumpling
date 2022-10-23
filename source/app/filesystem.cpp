#include "filesystem.h"
#include "cfw.h"
#include "gui.h"
#include "progress.h"

#include <dirent.h>
#include <sys/unistd.h>

static bool systemMLCMounted = false;
static bool systemUSBMounted = false;
static bool discMounted = false;

// unmount wii hook
extern "C" FSClient* __wut_devoptab_fs_client;
bool unmountDefaultDevoptab() {
    // Get FS mount path using current directory
    char mountPath[PATH_MAX];
    getcwd(mountPath, sizeof(mountPath));
    memmove(mountPath, mountPath + 3, PATH_MAX - 3);

    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);
    if (FSStatus res = FSUnmount(__wut_devoptab_fs_client, &cmd, "/vol/external01", FS_ERROR_FLAG_ALL); res != FS_STATUS_OK) {
        WHBLogPrintf("Couldn't unmount default devoptab with path %s! Error = %X", mountPath, res);
        WHBLogFreetypeDraw();
        return false;
    }

    if (FSStatus res = FSDelClient(__wut_devoptab_fs_client, FS_ERROR_FLAG_ALL); res != FS_STATUS_OK) {
        WHBLogPrintf("Couldn't delete wut devoptab with path %s! Error = %X", mountPath, res);
        WHBLogFreetypeDraw();
        return false;
    }
    return true;
}

// Wii files hook
typedef int (*devoptab_open)(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
devoptab_open mlc_open = nullptr;
#define O_OPEN_UNENCRYPTED 0x4000000
int hook_openWiiFiles(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    std::string strPath(path);
    int newFlags = flags;
    if (strPath.length() >= 10 && strPath.ends_with(".nfs")) {
        newFlags = newFlags | O_OPEN_UNENCRYPTED;
    }
    return mlc_open(r, fileStruct, path, newFlags, mode);
}

bool installWiiFilesHook(const char* dev_name) {
    auto* dev = const_cast<devoptab_t*>(GetDeviceOpTab(dev_name));
    if (dev == nullptr) return false;
    mlc_open = dev->open_r;
    dev->open_r = hook_openWiiFiles;
    return true;
}

bool mountSystemDrives() {
    WHBLogPrint("Mounting system drives...");
    WHBLogFreetypeDraw();
    if (USE_LIBMOCHA()) {
        //unmountDefaultDevoptab();
        if (Mocha_MountFS("storage_mlc01", nullptr, "/vol/storage_mlc01") == MOCHA_RESULT_SUCCESS && installWiiFilesHook("storage_mlc01:")) systemMLCMounted = true;
        if (Mocha_MountFS("storage_usb01", nullptr, "/vol/storage_usb01") == MOCHA_RESULT_SUCCESS && installWiiFilesHook("storage_usb01:")) systemUSBMounted = true;
    }
    else {
        systemMLCMounted = true;
        systemUSBMounted = false;
    }

    if (systemMLCMounted) WHBLogPrint("Successfully mounted the internal Wii U storage!");
    if (systemUSBMounted) WHBLogPrint("Successfully mounted the external Wii U storage!");
    WHBLogFreetypeDraw();
    return systemMLCMounted; // Require only the MLC to be mounted for this function to be successful
}

bool mountDisc() {
    if (USE_LIBMOCHA()) {
        if (Mocha_MountFS("storage_odd01", "/dev/odd01", "/vol/storage_odd_tickets") == MOCHA_RESULT_SUCCESS) discMounted = true;
        if (Mocha_MountFS("storage_odd02", "/dev/odd02", "/vol/storage_odd_updates") == MOCHA_RESULT_SUCCESS) discMounted = true;
        if (Mocha_MountFS("storage_odd03", "/dev/odd03", "/vol/storage_odd_content") == MOCHA_RESULT_SUCCESS) discMounted = true;
        if (Mocha_MountFS("storage_odd04", "/dev/odd04", "/vol/storage_odd_content2") == MOCHA_RESULT_SUCCESS) discMounted = true;
    }
    if (discMounted) WHBLogPrint("Successfully mounted the disc!");
    WHBLogFreetypeDraw();
    return discMounted;
}

bool unmountSystemDrives() {
    if (USE_LIBMOCHA()) {
        if (systemMLCMounted && Mocha_UnmountFS("storage_mlc01") == MOCHA_RESULT_SUCCESS) systemMLCMounted = false;
        if (systemUSBMounted && Mocha_UnmountFS("storage_usb01") == MOCHA_RESULT_SUCCESS) systemUSBMounted = false;
    }
    else {
        systemMLCMounted = false;
        systemUSBMounted = false;
    }
    return (!systemMLCMounted && !systemUSBMounted);
}

bool unmountDisc() {
    if (!discMounted) return false;
    if (USE_LIBMOCHA()) {
        if (Mocha_UnmountFS("storage_odd01") == MOCHA_RESULT_SUCCESS) discMounted = false;
        if (Mocha_UnmountFS("storage_odd02") == MOCHA_RESULT_SUCCESS) discMounted = false;
        if (Mocha_UnmountFS("storage_odd03") == MOCHA_RESULT_SUCCESS) discMounted = false;
        if (Mocha_UnmountFS("storage_odd04") == MOCHA_RESULT_SUCCESS) discMounted = false;
    }
    return !discMounted;
}

bool isDiscMounted() {
    return discMounted;
}

bool testStorage(TITLE_LOCATION location) {
    if (location == TITLE_LOCATION::NAND) return dirExist(convertToPosixPath("/vol/storage_mlc01/usr/").c_str());
    if (location == TITLE_LOCATION::USB) return dirExist(convertToPosixPath("/vol/storage_usb01/usr/").c_str());
    //if (location == TITLE_LOCATION::Disc) return dirExist("storage_odd01:/usr/");
    return false;
}

bool isDiscInserted() {
    return false;
    // if (getCFWVersion() == TIRAMISU_RPX) {
    //     // Get the disc key via Tiramisu's CFW
    //     std::array<uint8_t, 16> discKey = {0};
    //     discKey.fill(0);
    //     int32_t result = IOSUHAX_ODM_GetDiscKey(discKey.data());
    //     if (result == 0) {
    //         // WHBLogPrintf("%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", discKey.at(0), discKey.at(1), discKey.at(2), discKey.at(3), discKey.at(4), discKey.at(5), discKey.at(6), discKey.at(7), discKey.at(8), discKey.at(9), discKey.at(10), discKey.at(11), discKey.at(12), discKey.at(13), discKey.at(14), discKey.at(15));
    //         // WHBLogFreetypeDraw();
    //         // sleep_for(3s);
    //         return !(std::all_of(discKey.begin(), discKey.end(), [](uint8_t i) {return i==0;}));
    //     }
    //     else return false;
    // }
    // else {
    //     // Use the older method of detecting discs
    //     // The ios_odm module writes the disc key whenever a disc is inserted
    //     DCInvalidateRange((void*)0xF5E10C00, 32);
    //     return *(volatile uint32_t*)0xF5E10C00 != 0;
    // }
}

// Filesystem Helper Functions

// Wii U libraries will give us paths that use /vol/storage_mlc01/file.txt, but libiosuhax uses the mounted drive paths like storage_mlc01:/file.txt (and wut uses fs:/vol/sys_mlc01/file.txt)
// Converts a Wii U device path to a posix path
std::string convertToPosixPath(const char* volPath) {
    std::string posixPath;

    // volPath has to start with /vol/
    if (strncmp("/vol/", volPath, 5) != 0) return "";

    if (USE_LIBMOCHA()) {
        // Get and append the mount path
        const char *drivePathEnd = strchr(volPath + 5, '/');
        if (drivePathEnd == nullptr) {
            // Return just the mount path
            posixPath.append(volPath + 5);
            posixPath.append(":");
        } else {
            // Return mount path + the path after it
            posixPath.append(volPath, 5, drivePathEnd - (volPath + 5));
            posixPath.append(":/");
            posixPath.append(drivePathEnd + 1);
        }
        return posixPath;
    }
    else return std::string("fs:") + volPath;
}


struct stat existStat;
const std::regex rootFilesystem(R"(^fs:\/vol\/[^\/:]+\/?$)");
bool isRoot(const char* path) {
    std::string newPath(path);
    if (newPath.size() >= 2 && newPath.rbegin()[0] == ':') return true;
    if (newPath.size() >= 3 && newPath.rbegin()[1] == ':' && newPath.rbegin()[0] == '/') return true;
    if (true/*!IS_CEMU_PRESENT()*/) {
        if (std::regex_match(newPath, rootFilesystem)) return true;
    }
    return false;
}

bool fileExist(const char* path) {
    if (isRoot(path)) return true;
    if (lstat(path, &existStat) == 0 && S_ISREG(existStat.st_mode)) return true;
    return false;
}

bool dirExist(const char* path) {
    if (isRoot(path)) return true;
    if (lstat(path, &existStat) == 0 && S_ISDIR(existStat.st_mode)) return true;
    return false;
}

bool isDirEmpty(const char* path) {
    DIR* dirHandle;
    if ((dirHandle = opendir(path)) == nullptr) return true;

    struct dirent *dirEntry;
    while((dirEntry = readdir(dirHandle)) != nullptr) {
        if ((dirEntry->d_type & DT_DIR) == DT_DIR && (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)) continue;
        
        // An entry other than the root and parent directory was found
        closedir(dirHandle);
        return false;
    }
    
    closedir(dirHandle);
    return true;
}

TITLE_LOCATION deviceToLocation(const char* device) {
    if (memcmp(device, "mlc", 3) == 0) return TITLE_LOCATION::NAND;
    if (memcmp(device, "usb", 3) == 0) return TITLE_LOCATION::USB;
    if (memcmp(device, "odd", 3) == 0) return TITLE_LOCATION::DISC;
    return TITLE_LOCATION::UNKNOWN;
}

TITLE_LOCATION pathToLocation(const char* device) {
    if (memcmp(device, "storage_mlc01", strlen("storage_mlc01")) == 0) return TITLE_LOCATION::NAND;
    if (memcmp(device, "storage_usb01", strlen("storage_usb01")) == 0) return TITLE_LOCATION::USB;
    if (memcmp(device, "storage_odd0", strlen("storage_odd0")) == 0) return TITLE_LOCATION::DISC;
    return TITLE_LOCATION::UNKNOWN;
}
