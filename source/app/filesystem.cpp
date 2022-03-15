#include "filesystem.h"
#include <iosuhax.h>
#ifdef CEMU_STUBS
#include "../stub/devoptab_fs.h"
#else
#include <iosuhax_devoptab.h>
#endif
#include <iosuhax_disc_interface.h>
#include <fat.h>
#include <sys/statvfs.h>
#include "gui.h"
#include "iosuhax.h"

static bool systemMLCMounted = false;
static bool systemUSBMounted = false;
static bool discMounted = false;
static bool SDMounted = false;
static bool USBMounted = false;

int32_t sdHandle = 0;

bool mountSystemDrives() {
    if (mount_fs("storage_mlc01", getFSAHandle(), NULL, "/vol/storage_mlc01") == 0) systemMLCMounted = true;
    if (mount_fs("storage_usb01", getFSAHandle(), NULL, "/vol/storage_usb01") == 0) systemUSBMounted = true;
    if (systemMLCMounted) WHBLogPrint("Successfully mounted the internal Wii U storage!");
    if (systemUSBMounted) WHBLogPrint("Successfully mounted the external Wii U storage!");
    WHBLogFreetypeDraw();
    return systemMLCMounted; // Require only the MLC to be mounted for this function to be successful
}

bool mountSD() {
    if (fatMountSimple("sdfat", &IOSUHAX_sdio_disc_interface)) SDMounted = true;
    if (SDMounted) WHBLogPrint("Successfully mounted the SD card!");
    return SDMounted;
}

bool mountUSBDrive() {
    if (fatMountSimple("usbfat", &IOSUHAX_usb_disc_interface)) USBMounted = true;
    if (USBMounted) WHBLogPrint("Successfully mounted an USB stick!");
    return USBMounted;
}

bool mountDisc() {
    if (mount_fs("storage_odd01", getFSAHandle(), "/dev/odd01", "/vol/storage_odd_tickets") == 0) discMounted = true; 
    if (mount_fs("storage_odd02", getFSAHandle(), "/dev/odd02", "/vol/storage_odd_updates") == 0) discMounted = true;
    if (mount_fs("storage_odd03", getFSAHandle(), "/dev/odd03", "/vol/storage_odd_content") == 0) discMounted = true;
    if (mount_fs("storage_odd04", getFSAHandle(), "/dev/odd04", "/vol/storage_odd_content2") == 0) discMounted = true;
    if (discMounted) WHBLogPrint("Successfully mounted the disc!");
    WHBLogFreetypeDraw();
    return discMounted;
}

bool unmountSystemDrives() {
    // Unmount all of the devices
    if (systemMLCMounted && unmount_fs("storage_mlc01") == 0) systemMLCMounted = false;
    if (systemUSBMounted && unmount_fs("storage_usb01") == 0) systemUSBMounted = false;
    return (!systemMLCMounted && !systemUSBMounted);
}

void unmountSD() {
    if (SDMounted) fatUnmount("sdfat");
    SDMounted = false;
}

void unmountUSBDrive() {
    if (USBMounted) fatUnmount("usbfat");
    USBMounted = false;
}

bool unmountDisc() {
    if (!discMounted) return false;
    if (unmount_fs("storage_odd01") == 0) discMounted = false;
    if (unmount_fs("storage_odd02") == 0) discMounted = false;
    if (unmount_fs("storage_odd03") == 0) discMounted = false;
    if (unmount_fs("storage_odd04") == 0) discMounted = false;
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

// Wii U libraries will give us paths that use /vol/storage_mlc01/file.txt, but posix uses the mounted drive paths like storage_mlc01:/file.txt
// Converts a Wii U device path to a posix path
std::string convertToPosixPath(const char* volPath) {
    std::string posixPath;
    
    // volPath has to start with /vol/
    if (strncmp("/vol/", volPath, 5) != 0) return "";

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
bool fileExist(const char* path) {
    if (stat(path, &existStat) == 0 && S_ISREG(existStat.st_mode)) return true;
    return false;
}

bool dirExist(const char* path) {
    if (strlen(path) >= 2 && path[strlen(path)-1] == ':') return true;
    if (stat(path, &existStat) == 0 && S_ISDIR(existStat.st_mode)) return true;
    return false;
}

bool isDirEmpty(const char* path) {
    DIR* dirHandle;
    if ((dirHandle = opendir(path)) == NULL) return true;

    struct dirent *dirEntry;
    while((dirEntry = readdir(dirHandle)) != NULL) {
        if (dirEntry->d_type == DT_DIR && (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)) continue;
        
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
            mkdir(tmp, ACCESSPERMS);
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
    if (location == dumpLocation::SDFat) return "sdfat:";
    else if (location == dumpLocation::USBFat) return "usbfat:";
    else if (location == dumpLocation::USBExFAT) return "usbexfat:";
    else if (location == dumpLocation::USBNTFS) return "usbntfs:";
    return "";
}

dumpLocation getLocationFromRoot(std::string rootPath) {
    if (rootPath.compare("sdfat:") == 0) return dumpLocation::SDFat;
    else if (rootPath.compare("usbfat:") == 0) return dumpLocation::USBFat;
    else if (rootPath.compare("usbexfat:") == 0) return dumpLocation::USBExFAT;
    else if (rootPath.compare("usbntfs:") == 0) return dumpLocation::USBNTFS;
    return dumpLocation::Unknown;
}

titleLocation deviceToLocation(char* device) {
    if (memcmp(device, "odd", 3) == 0) return titleLocation::Disc;
    if (memcmp(device, "usb", 3) == 0) return titleLocation::USB;
    if (memcmp(device, "mlc", 3) == 0) return titleLocation::Nand;
    return titleLocation::Unknown;
}