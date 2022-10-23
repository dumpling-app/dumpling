#include "fat32.h"

#include "./../menu.h"
#include "./../filesystem.h"
#include "./../../utils/fatfs/ff.h"
#include "./../../utils/fatfs/diskio.h"
#include "../../utils/log_freetype.h"
#include "../../utils/fatfs/ffcache.h"

struct Fat32Transfer::FATFSPtr: public FATFS {};
struct Fat32Transfer::FILPtr: public FIL {};
struct Fat32Transfer::DIRPtr: public DIR {};

size_t Fat32Transfer::filePtrAlignment = 0x40 - (offsetof(FIL, buf) % 0x40);

std::vector<std::pair<std::string, std::string>> Fat32Transfer::getDrives() {
    std::vector<std::pair<std::string, std::string>> targets;
    for (BYTE i=0; i<FF_VOLUMES; i++) {
        std::string driveIdPath = std::to_string(i)+":/";

        FATFS fs;
        if (FRESULT res = f_mount(&fs, driveIdPath.c_str(), 1); res != FR_OK) {
            OSReport("[Fat32Transfer] GetDrives: Error %d while mounting %s\n", res, driveIdPath.c_str());
            disk_shutdown(i);
            f_unmount(driveIdPath.c_str());
            continue;
        }
        char volumeLabel[34];
        DWORD volumeSerial = 0;
        if (FRESULT res = f_getlabel(driveIdPath.c_str(), volumeLabel, &volumeSerial); res != FR_OK) {
            OSReport("[Fat32Transfer] GetDrives: Error %d while getting label %s\n", res, driveIdPath.c_str());
            disk_shutdown(i);
            f_unmount(driveIdPath.c_str());
            continue;
        }
        disk_shutdown(i);
        f_unmount(driveIdPath.c_str());

        std::string volumeName;
        if (i == DEV_SD_REF) volumeName = "SD";
        else if (i == DEV_USB01_REF) volumeName = "USB 1";
        else if (i == DEV_USB02_REF) volumeName = "USB 2";
        if (volumeLabel[0]) volumeName += std::string(" - ") + volumeLabel;
        else volumeName += " - Not Named";

        targets.emplace_back(std::make_pair((driveIdPath+Fat32Transfer::targetDirectoryName).c_str(), volumeName));
    }
    return targets;
}

uint64_t Fat32Transfer::getDriveSpace(const std::string& drivePath) {
    BYTE driveIdx = drivePath[0] - '0';
    FATFS* freeFS = (FATFS*)aligned_alloc(0x40, sizeof(FATFS));
    if (FRESULT res = f_mount(freeFS, drivePath.c_str(), 1); res != FR_OK) {
        OSReport("[Fat32Transfer] GetDriveSpace: Error %d while mounting %s\n", res, drivePath.c_str());
        disk_shutdown(driveIdx);
        free(freeFS);
        return 0;
    }

    DWORD freeClusters = 0;
    if (FRESULT res = f_getfree(drivePath.c_str(), &freeClusters, (FATFS**)&freeFS); res != FR_OK) {
        OSReport("[Fat32Transfer] GetDriveSpace: Error %d while getting free space %s\n", res, drivePath.c_str());
        disk_shutdown(driveIdx);
        f_unmount(drivePath.c_str());
        free(freeFS);
        return 0;
    }
    disk_shutdown(driveIdx);
    f_unmount(drivePath.c_str());
    free(freeFS);
    uint64_t clusterSize = freeFS->csize * 512;
    return freeClusters * clusterSize;
}

bool Fat32Transfer::deletePath(const std::string &path, const std::function<void()>& callback_updateGUI) {
    BYTE driveIdx = path[0] - '0';
    FATFS* deleteFS = (FATFS*)aligned_alloc(0x40, sizeof(FATFS));
    if (FRESULT res = f_mount(deleteFS, path.c_str(), 1); res != FR_OK) {
        OSReport("[Fat32Transfer] DeleteFolders: Error %d while mounting %s\n", res, path.c_str());
        disk_shutdown(driveIdx);
        free(deleteFS);
        return false;
    }

    FILINFO info;
    if (FRESULT res = f_stat(path.c_str(), &info); res != FR_OK) {
        disk_shutdown(driveIdx);
        f_unmount(path.c_str());
        free(deleteFS);
        if (res == FR_NO_FILE || res == FR_NO_PATH) {
            return true;
        }
        OSReport("[Fat32Transfer] DeleteFolders: Error %d while checking for folder to delete at %s\n", res, path.c_str());
        return false;
    }

    if (info.fattrib & AM_ARC) {
        FRESULT res = f_unlink(path.c_str());
        disk_shutdown(driveIdx);
        f_unmount(path.c_str());
        free(deleteFS);
        if (res != FR_OK) {
            std::string deleteError = "Error "+std::to_string(res)+" while deleting the following file:\n" + path + "\n";
            setErrorPrompt(deleteError);
            return false;
        }
        return true;
    }

    if (!(info.fattrib & AM_DIR)) {
        OSReport("[Fat32Transfer] DeleteFolders: Tried deleting %s but found non-folder\n", path.c_str());
        return false;
    }

    std::function<FRESULT(const std::filesystem::path& dirPath)> recursiveDeletion;
    recursiveDeletion = [driveIdx, &recursiveDeletion, callback_updateGUI](const std::filesystem::path& dirPath) -> FRESULT {
        DIR dir;
        if (FRESULT res = f_opendir(&dir, dirPath.c_str()); res != FR_OK) {
            std::string deleteError = "Failed while deleting folder entry in recursive loop:\n";
            deleteError += dirPath.string()+"\n";
            setErrorPrompt(deleteError);
            return res;
        }

        while (true) {
            FILINFO fno = {};
            if (FRESULT res = f_readdir(&dir, &fno); res != FR_OK || fno.fname[0] == 0) break;

            callback_updateGUI();

            if (fno.fattrib & AM_DIR) {
                if (FRESULT res = recursiveDeletion(dirPath / fno.fname); res != FR_OK) {
                    f_closedir(&dir);
                    return res;
                }
            }
            else {
                if (FRESULT res = f_unlink((dirPath / fno.fname).c_str()); res != FR_OK) {
                    std::string deleteError = "Failed to delete file:\n";
                    deleteError += (dirPath / fno.fname).string()+"\n";
                    setErrorPrompt(deleteError);
                    f_closedir(&dir);
                    return res;
                }
                disk_ioctl(driveIdx, CTRL_FORCE_SYNC, nullptr);
            }
        }

        f_closedir(&dir);
        if (FRESULT res = f_unlink((dirPath).c_str()); res != FR_OK) {
            std::string deleteError = "Couldn't delete folder!\n";
            deleteError += dirPath.string()+"\n";
            setErrorPrompt(deleteError);
            return res;
        }
        disk_ioctl(driveIdx, CTRL_FORCE_SYNC, nullptr);
        return FR_OK;
    };

    if (FRESULT res = recursiveDeletion(path); res != FR_OK) {
        OSReport("[Fat32Transfer] DeleteFolders: Error %d while deleting folder contents\n", res);
        disk_shutdown(driveIdx);
        f_unmount(path.c_str());
        free(deleteFS);
        return false;
    }
    disk_shutdown(driveIdx);
    f_unmount(path.c_str());
    free(deleteFS);
    return true;
}

Fat32Transfer::Fat32Transfer(const dumpingConfig& config) : TransferInterface(config) {
    this->fatTarget = config.dumpTarget;
}

Fat32Transfer::~Fat32Transfer() {
    this->submitStopThread();
    while (!this->hasStopped()) {
        sleep_for(50ms);
    }
    if (this->transferThread.joinable()) {
        this->transferThread.join();
    }
};

void Fat32Transfer::transferThreadLoop(dumpingConfig config) {
    // Initialize fatfs thread
    OSReport("Starting fat32 transfer thread...!\n");

    DWORD cacheSize = config.debugCacheSize;
    disk_ioctl((BYTE)(this->fatTarget[0] - '0'), SET_CACHE_COUNT, (void*)&cacheSize);

    this->fs = (FATFSPtr*)malloc(sizeof(FATFS));
    if (FRESULT res = f_mount(this->fs, this->fatTarget.c_str(), 1); res != FR_OK) {
        this->runThreadLoop = false;
        this->error = "Drive "+this->fatTarget+" couldn't be mounted! Error #"+std::to_string(res)+"\n";
    }

    if (this->runThreadLoop) {
        // Create /Dumpling/ folder
        f_mkdir(this->fatTarget.c_str());
    }

    // Loop thread
    while(runThreadLoop) {
        OSWaitSemaphore(&this->countSemaphore);
        std::unique_lock<std::mutex> lck(this->mutex);

        TransferCommands chunk = this->chunks.front();
        this->chunks.pop();
        OSMemoryBarrier();
        lck.unlock();
        OSSignalSemaphore(&this->maxSemaphore);

        std::visit(overloaded{
            [this](CommandSwitchDir& arg) {
                DEBUG_OSReport("Switch Directory: path=%s", arg.dirPath.c_str());
                if (FRESULT res = f_chdir((this->fatTarget+arg.dirPath).c_str()); res != FR_OK) {
                    this->error += "Failed to switch to the given directory!\n";
                    this->error += "Error "+std::to_string(res)+" after changing directory:\n";
                    this->error += this->fatTarget+arg.dirPath;
                    this->runThreadLoop = false;
                }
            },
            [this](CommandMakeDir& arg) {
                DEBUG_OSReport("Make Directory: path=%s", arg.dirPath.c_str());
                if (FRESULT res = f_mkdir((this->fatTarget+arg.dirPath).c_str()); res != FR_OK && res != FR_EXIST/* && res != FR_NO_PATH*/) {
                    this->error += "Couldn't make the directory!\n";
                    this->error += "For SD cards: Make sure it isn't locked\nby the write-switch on the side!\n\nDetails:\n";
                    this->error += "Error "+std::to_string(res)+" after creating directory:\n";
                    this->error += this->fatTarget+arg.dirPath;
                    this->runThreadLoop = false;
                }
            },
            [this](CommandWrite& arg) {
                DEBUG_OSReport("Write File: path=%.75s size=%u closeFileAtEnd=%s", arg.filePath.c_str(), (uint32_t)arg.chunkSize, arg.closeFileAtEnd ? "true" : "false");

                if (FRESULT res = (FRESULT) this->openFile(this->fatTarget + arg.filePath, arg.fileSize); res != FR_OK) {
                    free(arg.chunkBuffer);
                    this->closeFile(this->fatTarget + arg.filePath);
                    this->error += "Couldn't open the file to copy to!\n";
                    this->error += "For SD cards: Make sure it isn't locked\nby the write-switch on the side!\n\nDetails:\n";
                    this->error += "Error "+std::to_string(res)+" after creating SD card file:\n";
                    this->error += this->fatTarget+arg.filePath;
                    this->runThreadLoop = false;
                    return;
                }

                UINT bytesWritten = 0;
                if (FRESULT res = f_write(this->currFileHandle, arg.chunkBuffer, arg.chunkSize, &bytesWritten); res != FR_OK || bytesWritten != arg.chunkSize) {
                    free(arg.chunkBuffer);
                    this->closeFile(this->fatTarget + arg.filePath);
                    this->error += "Failed to write data to dumping device!\n";
                    if (res == FR_DENIED) this->error += "There's no space available on the USB/SD card!\n";
                    this->error += "\nDetails:\n";
                    this->error += "Error "+std::to_string(res)+" when writing data to:\n";
                    this->error += this->fatTarget+arg.filePath;
                    this->runThreadLoop = false;
                    return;
                }
                free(arg.chunkBuffer);

                if (arg.closeFileAtEnd)
                    this->closeFile(this->fatTarget + arg.filePath);
            },
            [this](CommandStopThread& arg) {
                DEBUG_OSReport("Stop Thread");
                this->runThreadLoop = false;
            }
        }, chunk);
    }

    OSReport("Cleaning up the thread...\n");
    OSReport(this->error.c_str());

    // Shutdown fatfs thread
    if (this->currFileHandle != nullptr) {
        if (FRESULT res = f_close(this->currFileHandle); res != FR_OK) {
            OSReport("[fat32] Error %d while closing fatfs thread!\n", res);
        }
        free(((uint8_t*)this->currFileHandle)-filePtrAlignment);
    }

    // Unmount and free fatfs
    BYTE driveIdx = this->fatTarget[0] - '0';
    disk_shutdown(driveIdx);
    f_unmount(this->fatTarget.c_str());
    free(this->fs);

    if (!this->error.empty())
        this->threadStoppedError = this->error;
    this->threadStopped = true;
    OSMemoryBarrier();
}



// Should only be called from transfer thread
uint8_t Fat32Transfer::openFile(const std::string &path, size_t prepareSize) {
    if (this->currFilePath == path)
        return FR_OK;
    else if (this->currFileHandle != nullptr) {
        OSReport("[fat32] Expected a matching path from the given file handle!\n");
        return FR_INT_ERR;
    }

    this->currFilePath = path;
    this->currFileHandle = (FILPtr*)((uint8_t*)aligned_alloc(0x40, sizeof(FIL)+filePtrAlignment) + filePtrAlignment);
    if (this->currFileHandle == nullptr) {
        OSReport("[fat32] Couldn't allocate memory for file handle!\n");
        return FR_NOT_ENOUGH_CORE;
    }
    DEBUG_profile_startSegment(total);
    std::string filename = path.substr(0, 2)+std::filesystem::path(path).filename().string();
    if (FRESULT res = f_open(this->currFileHandle, filename.c_str(), FA_CREATE_ALWAYS | FA_WRITE); res != FR_OK) {
        DEBUG_profile_endSegment(total);
        OSReport("[fat32] Error %d occurred while opening %s!\n", res, path.c_str());
        free(((uint8_t*)this->currFileHandle)-filePtrAlignment);
        return res;
    }
    DEBUG_profile_endSegment(total);
    DEBUG_profile_incrementCounter(files);

    if (FRESULT res = f_expand(this->currFileHandle, prepareSize, 1); res != FR_OK && res != FR_DENIED) {
        OSReport("[fat32] Error %d occurred while allocating %s!\n", res, path.c_str());
        f_close(this->currFileHandle);
        free(((uint8_t*)this->currFileHandle)-filePtrAlignment);
        return res;
    }
    return FR_OK;
}

// Should only be called from transfer thread
uint8_t Fat32Transfer::closeFile(const std::string& path) {
    // Debug assertion when stored path doesn't match given path
    if (this->currFilePath != path)
        return FR_INT_ERR;

    FRESULT res = f_close(this->currFileHandle);
    if (res != FR_OK) {
        OSReport("[fat32] Error %d occurred while closing %s!\n", res, path.c_str());
    }
    free(((uint8_t*)this->currFileHandle)-filePtrAlignment);
    this->currFileHandle = nullptr;
    this->currFilePath = "";
    return res;
}
