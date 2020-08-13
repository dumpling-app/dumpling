#include "filesystem.h"
#include <sys/stat.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>
#include <fat.h>
#include <sys/statvfs.h>

#include "iosuhax.h"

bool mlcMounted = false;
bool usbMounted = false;
bool discMounted = false;
bool sdfatMounted = false;
bool usbfatMounted = false;

bool mountDevices() {
    // Mount internal storage devices
    if (mount_fs("storage_mlc01", getFSAHandle(), NULL, "/vol/storage_mlc01") == 0) mlcMounted = true;
    if (mount_fs("storage_usb01", getFSAHandle(), NULL, "/vol/storage_usb01") == 0) usbMounted = true;
    // Mount SD card and USB using libfat
    if (fatMountSimple("sdfat", &IOSUHAX_sdio_disc_interface)) sdfatMounted = true;
    if (fatMountSimple("usbfat", &IOSUHAX_usb_disc_interface)) usbfatMounted = true;
    //WHBLogPrintf("SD MOUNTED: %d", sdfatMounted);
    //WHBLogPrintf("USB MOUNTED: %d", usbfatMounted);
    //WHBLogConsoleDraw();
    //OSSleepTicks(OSMillisecondsToTicks(2000));
    return mlcMounted && sdfatMounted; // Require both the NAND and SD card to be mounted
}

bool mountDisc() {
    if (mount_fs("storage_odd01", getFSAHandle(), "/dev/odd01", "/vol/storage_odd_tickets") == 0) discMounted = true; 
    if (mount_fs("storage_odd02", getFSAHandle(), "/dev/odd02", "/vol/storage_odd_updates") == 0) discMounted = true;
    if (mount_fs("storage_odd03", getFSAHandle(), "/dev/odd03", "/vol/storage_odd_content") == 0) discMounted = true;
    if (mount_fs("storage_odd04", getFSAHandle(), "/dev/odd04", "/vol/storage_odd_content2") == 0) discMounted = true;
    return discMounted;
}

bool unmountDevices() {
    // Unmount all of the devices
    if (mlcMounted && unmount_fs("storage_mlc01") == 0) mlcMounted = false;
    if (usbMounted && unmount_fs("storage_usb01") == 0) usbMounted = false;
    if (sdfatMounted) fatUnmount("sdfat");
    if (usbfatMounted) fatUnmount("usbfat");
    sdfatMounted = false;
    usbfatMounted = false;
    return (!mlcMounted && !usbMounted);
}

bool unmountDisc() {
    if (unmount_fs("storage_odd01") == 0) discMounted = false;
    if (unmount_fs("storage_odd02") == 0) discMounted = false;
    if (unmount_fs("storage_odd03") == 0) discMounted = false;
    if (unmount_fs("storage_odd04") == 0) discMounted = false;
    return !discMounted;
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
    if (drivePathEnd == NULL) {
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
    
    WHBLogPrint("Unknown device type detected:");
    WHBLogPrint(device);
    WHBLogConsoleDraw();
    OSSleepTicks(OSSecondsToTicks(2));

    // TODO: Check what happens with multiple USB drives. Do you get usb2 or something?
    return titleLocation::Unknown;
}