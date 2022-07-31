#include "filesystem.h"
#include "cfw.h"

#include <mocha/disc_interface.h>
#include <mocha/fsa.h>

#ifdef __cplusplus
extern "C" {
#endif
// Define libfat header methods manually to prevent including any libiosuhax.h header due to collisions
extern bool fatMountSimple (const char* name, const DISC_INTERFACE* interface);
extern bool fatMount (const char* name, const DISC_INTERFACE* interface, sec_t startSector, uint32_t cacheSize, uint32_t SectorsPerPage);
extern void fatUnmount (const char* name);
#ifdef __cplusplus
}
#endif

#include <sys/statvfs.h>
#include "gui.h"

static bool systemMLCMounted = false;
static bool systemUSBMounted = false;
static bool discMounted = false;
static bool SDMounted = false;
static bool USBMounted = false;

int32_t sdHandle = 0;

bool unlockedFSClient = false;
extern "C" FSClient* __wut_devoptab_fs_client;

bool mountFilesystemCemu(const char* source, const char* target) {
    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);

    FSStatus res = FSBindMount(__wut_devoptab_fs_client, &cmd, source, target, FS_ERROR_FLAG_ALL);
    if (res != FS_STATUS_OK) {
        WHBLogPrintf("Couldn't mount %s drive! Error = %X", source, res);
        WHBLogFreetypeDraw();
        return false;
    }
    return true;
}

bool unmountFilesystemCemu(const char* target) {
    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);

    FSStatus res = FSBindUnmount(__wut_devoptab_fs_client, &cmd, target, FS_ERROR_FLAG_ALL);
    if (res != FS_STATUS_OK) {
        WHBLogPrintf("Couldn't unmount %s drive! Error = %X", target, res);
        WHBLogFreetypeDraw();
        return false;
    }
    return true;
}

#define O_OPEN_UNENCRYPTED 0x4000000

typedef int (*devoptab_open)(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);

devoptab_open mlc_open = nullptr;

int hooked_devoptab_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    std::string strPath(path);
    int newFlags = flags;
    if (strPath.length() >= 10 && strPath.ends_with(".nfs")) {
        newFlags = newFlags | O_OPEN_UNENCRYPTED;
    }
    return mlc_open(r, fileStruct, path, newFlags, mode);
}

bool installOpenHook(const char* dev_name) {
    devoptab_t* dev = (devoptab_t*)GetDeviceOpTab(dev_name);
    if (dev == NULL) return false;
    mlc_open = dev->open_r;
    dev->open_r = hooked_devoptab_open;
    return true;
}

bool mountSystemDrives() {
    WHBLogPrint("Mounting system drives...");
    WHBLogFreetypeDraw();
#ifdef USING_CEMU
    //if (mountFilesystemCemu("/dev/mlc01", "/vol/storage_mlc01")) systemMLCMounted = true;
    //if (mountFilesystemCemu("/dev/usb01", "/vol/storage_usb01")) systemUSBMounted = true;
    systemMLCMounted = true;
    systemUSBMounted = true;
#else
    if (Mocha_MountFS("storage_mlc01", nullptr, "/vol/storage_mlc01") == MOCHA_RESULT_SUCCESS && installOpenHook("storage_mlc01:")) systemMLCMounted = true;
    if (Mocha_MountFS("storage_usb01", nullptr, "/vol/storage_usb01") == MOCHA_RESULT_SUCCESS && installOpenHook("storage_usb01:")) systemUSBMounted = true;
#endif
    if (systemMLCMounted) WHBLogPrint("Successfully mounted the internal Wii U storage!");
    if (systemUSBMounted) WHBLogPrint("Successfully mounted the external Wii U storage!");
    WHBLogFreetypeDraw();
    return systemMLCMounted; // Require only the MLC to be mounted for this function to be successful
}

bool mountSD() {
#ifdef USE_LIBFAT
    Mocha_sdio_disc_interface.startup();
    if (fatMountSimple("sdfat", &Mocha_sdio_disc_interface)) SDMounted = true;
#else
    SDMounted = true; // initialization of wut's devoptab is done automatically
#endif
    if (SDMounted) WHBLogPrint("Successfully mounted the SD card!");
    return SDMounted;
}

bool mountUSBDrive() {
#ifdef USE_LIBFAT
    Mocha_usb_disc_interface.startup();
    if (fatMountSimple("usbfat", &Mocha_usb_disc_interface)) USBMounted = true;
#else
    WHBLogPrint("USB sticks aren't supported without libfat!");
#endif
    if (USBMounted) WHBLogPrint("Successfully mounted an USB stick!");
    return USBMounted;
}

bool mountDisc() {
#ifdef USE_LIBMOCHA
    if (Mocha_MountFS("storage_odd01", "/dev/odd01", "/vol/storage_odd_tickets") == MOCHA_RESULT_SUCCESS) discMounted = true;
    if (Mocha_MountFS("storage_odd02", "/dev/odd02", "/vol/storage_odd_updates") == MOCHA_RESULT_SUCCESS) discMounted = true;
    if (Mocha_MountFS("storage_odd03", "/dev/odd03", "/vol/storage_odd_content") == MOCHA_RESULT_SUCCESS) discMounted = true;
    if (Mocha_MountFS("storage_odd04", "/dev/odd04", "/vol/storage_odd_content2") == MOCHA_RESULT_SUCCESS) discMounted = true;
#else
    if (mountFilesystemCemu("/dev/odd01", "/vol/storage_odd01")) discMounted = true;
    if (mountFilesystemCemu("/dev/odd02", "/vol/storage_odd02")) discMounted = true;
    if (mountFilesystemCemu("/dev/odd03", "/vol/storage_odd03")) discMounted = true;
    if (mountFilesystemCemu("/dev/odd04", "/vol/storage_odd04")) discMounted = true;
#endif
    if (discMounted) WHBLogPrint("Successfully mounted the disc!");
    WHBLogFreetypeDraw();
    return discMounted;
}

bool unmountSystemDrives() {
#ifdef USE_LIBMOCHA
    if (systemMLCMounted && Mocha_UnmountFS("storage_mlc01") == MOCHA_RESULT_SUCCESS) systemMLCMounted = false;
    if (systemUSBMounted && Mocha_UnmountFS("storage_usb01") == MOCHA_RESULT_SUCCESS) systemUSBMounted = false;
#else
    systemMLCMounted = false;
    systemUSBMounted = false;
    // if (systemMLCMounted && unmountRootFilesystem("/vol/storage_mlc01")) systemMLCMounted = false;
    // if (systemUSBMounted && unmountRootFilesystem("/vol/storage_usb01")) systemUSBMounted = false;
#endif
    return (!systemMLCMounted && !systemUSBMounted);
}

void unmountSD() {
#ifdef USE_LIBFAT
    if (SDMounted) fatUnmount("sdfat");
#endif
    SDMounted = false;
}

void unmountUSBDrive() {
#ifdef USE_LIBFAT
    if (USBMounted) fatUnmount("usbfat");
#endif
    USBMounted = false;
}

bool unmountDisc() {
    if (!discMounted) return false;
#ifdef USE_LIBMOCHA
    if (Mocha_UnmountFS("storage_odd01") == MOCHA_RESULT_SUCCESS) discMounted = false;
    if (Mocha_UnmountFS("storage_odd02") == MOCHA_RESULT_SUCCESS) discMounted = false;
    if (Mocha_UnmountFS("storage_odd03") == MOCHA_RESULT_SUCCESS) discMounted = false;
    if (Mocha_UnmountFS("storage_odd04") == MOCHA_RESULT_SUCCESS) discMounted = false;
#else
    if (unmountFilesystemCemu("/vol/storage_odd01")) discMounted = false;
    if (unmountFilesystemCemu("/vol/storage_odd02")) discMounted = false;
    if (unmountFilesystemCemu("/vol/storage_odd03")) discMounted = false;
    if (unmountFilesystemCemu("/vol/storage_odd04")) discMounted = false;
#endif
    return !discMounted;
}

bool isSDMounted() {
    return SDMounted;
}

bool isUSBDriveMounted() {
    return USBMounted;
}

bool isDiscMounted() {
    return discMounted;
}

bool isExternalStorageMounted() {
    return systemUSBMounted;
}

bool testStorage(titleLocation location) {
    if (location == titleLocation::Nand) return dirExist(convertToPosixPath("/vol/storage_mlc01/usr/").c_str());
    if (location == titleLocation::USB) return dirExist(convertToPosixPath("/vol/storage_usb01/usr/").c_str());
    //if (location == titleLocation::Disc) return dirExist("storage_odd01:/usr/");
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

bool isSDInserted() {
    bool sdInserted = mountSD();
    if (sdInserted) unmountSD();
    return sdInserted;
}

bool isUSBDriveInserted() {
    bool usbInserted = mountUSBDrive();
    if (usbInserted) unmountUSBDrive();
    return usbInserted;
}

// Filesystem Helper Functions

// Wii U libraries will give us paths that use /vol/storage_mlc01/file.txt, but libiosuhax uses the mounted drive paths like storage_mlc01:/file.txt (and wut uses fs:/vol/sys_mlc01/file.txt)
// Converts a Wii U device path to a posix path
std::string convertToPosixPath(const char* volPath) {
    std::string posixPath;

    // volPath has to start with /vol/
    if (strncmp("/vol/", volPath, 5) != 0) return "";

#ifdef USE_LIBMOCHA
    // Get and append the mount path
    const char* drivePathEnd = strchr(volPath+5, '/');
    if (drivePathEnd == nullptr) {
        // Return just the mount path
        posixPath.append(volPath+5);
        posixPath.append(":");
    }
    else {
        // Return mount path + the path after it
        posixPath.append(volPath, 5, drivePathEnd-(volPath+5));
        posixPath.append(":/");
        posixPath.append(drivePathEnd+1);
    }
    return posixPath;
#else
    return std::string("fs:") + volPath;
#endif
}

// Converts a posix path to a Wii U device path
std::string convertToDevicePath(const char* volPath) {
    std::string devicePath = "/vol/";

    const char* drivePath = strchr(volPath, ':');
    const char* pathRemains = strchr(volPath, '/');
    
    if (drivePath == nullptr) return "";
    
    if (pathRemains == nullptr) {
        devicePath.append(volPath, drivePath-volPath);
        devicePath.append("/");
    }
    else {
        // Test if '/' is before ':', which is invalid
        if (drivePath-pathRemains > 0) return "";
    
        devicePath.append(volPath, drivePath-volPath);
        devicePath.append(pathRemains);
    }
    return devicePath;
}

struct stat existStat;
const std::regex rootFilesystem("^fs:\\/vol\\/[^\\/:]+\\/?$");
bool isRoot(const char* path) {
    std::string newPath(path);
    if (newPath.size() >= 2 && newPath.rbegin()[0] == ':') return true;
    if (newPath.size() >= 3 && newPath.rbegin()[1] == ':' && newPath.rbegin()[0] == '/') return true;
#ifndef USE_LIBMOCHA
    if (std::regex_match(newPath, rootFilesystem)) return true;
#endif
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
    if ((dirHandle = opendir(path)) == NULL) return true;

    struct dirent *dirEntry;
    while((dirEntry = readdir(dirHandle)) != NULL) {
        if ((dirEntry->d_type & DT_DIR) == DT_DIR && (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)) continue;
        
        // An entry other then the root and parent directory was found
        closedir(dirHandle);
        return false;
    }
    
    closedir(dirHandle);
    return true;
}

// From http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
void createPath(const char* path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len-1] == '/') tmp[len-1] = 0;
    for(p = tmp+1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (!dirExist(tmp)) mkdir(tmp, ACCESSPERMS);
            *p = '/';
        }
    }
    mkdir(tmp, ACCESSPERMS);
}

std::string formatByteSize(uint64_t bytes) {
    if (bytes < 1000) return std::to_string(bytes) + " Bytes";
    if (bytes < 1000000) return std::to_string(bytes/1000) + " KB";
    if (bytes < 1000000000) return std::to_string(bytes/1000000) + " MB";
    else return std::to_string(bytes/1000000000) + " GB";
}

std::string formatByteSizes(uint64_t dominantBytes, uint64_t otherBytes) {
    if (dominantBytes < 1000) return std::to_string(otherBytes) + "/" + std::to_string(dominantBytes) + " Bytes";
    if (dominantBytes < 1000000) return std::to_string(otherBytes/1000) + "/" + std::to_string(dominantBytes/1000) + " KB";
    if (dominantBytes < 1000000000) return std::to_string(otherBytes/1000000) + "/" + std::to_string(dominantBytes/1000000) + " MB";
    else return std::to_string(otherBytes/1000000000) + "/" + std::to_string(dominantBytes/1000000000) + " GB";
}

struct statvfs filesystemStruct;
uint64_t getFreeSpace(const char* path) {
    if (statvfs(path, &filesystemStruct) == 0) return filesystemStruct.f_frsize * filesystemStruct.f_bavail;
    else return 0;
}

uint64_t getTotalSpace(const char* path) {
    if (statvfs(path, &filesystemStruct) == 0) return filesystemStruct.f_frsize * filesystemStruct.f_blocks;
    else return 0;
}

std::string getRootFromLocation(dumpLocation location) {
#ifdef USE_LIBFAT
    if (location == dumpLocation::SDFat) return "sdfat:";
#else
    if (location == dumpLocation::SDFat) return "fs:/vol/external01";
#endif
    else if (location == dumpLocation::USBFat) return "usbfat:";
    else if (location == dumpLocation::USBExFAT) return "usbexfat:";
    else if (location == dumpLocation::USBNTFS) return "usbntfs:";
    return "";
}

dumpLocation getLocationFromRoot(std::string rootPath) {
#ifdef USE_LIBFAT
    if (rootPath.compare("sdfat:") == 0) return dumpLocation::SDFat;
#else
    if (rootPath.compare("fs:") == 0) return dumpLocation::SDFat;
#endif
    else if (rootPath.compare("usbfat:") == 0) return dumpLocation::USBFat;
    else if (rootPath.compare("usbexfat:") == 0) return dumpLocation::USBExFAT;
    else if (rootPath.compare("usbntfs:") == 0) return dumpLocation::USBNTFS;
    return dumpLocation::Unknown;
}

titleLocation deviceToLocation(const char* device) {
    if (memcmp(device, "mlc", 3) == 0) return titleLocation::Nand;
    if (memcmp(device, "usb", 3) == 0) return titleLocation::USB;
    if (memcmp(device, "odd", 3) == 0) return titleLocation::Disc;
    return titleLocation::Unknown;
}

titleLocation pathToLocation(const char* device) {
    if (memcmp(device, "storage_mlc01", strlen("storage_mlc01")) == 0) return titleLocation::Nand;
    if (memcmp(device, "storage_usb01", strlen("storage_usb01")) == 0) return titleLocation::USB;
    if (memcmp(device, "storage_odd0", strlen("storage_odd0")) == 0) return titleLocation::Disc;
    return titleLocation::Unknown;
}