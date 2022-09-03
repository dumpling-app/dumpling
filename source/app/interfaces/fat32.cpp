#include "fat32.h"
#include "./../gui.h"
#include "./../menu.h"
#include "./../filesystem.h"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

// Should only be called from transfer thread
FILE* Fat32Transfer::getFileHandle(std::string& path) {
    auto fileHandle = this->fileHandles.find(path);
    if (fileHandle != this->fileHandles.end()) {
        return fileHandle->second;
    }
    return this->fileHandles.emplace(path, fopen(path.c_str(), "wb")).first->second;
}

// Should only be called from transfer thread
void Fat32Transfer::closeFileHandle(std::string& path) {
    FILE* fileHandle = this->getFileHandle(path);
    this->fileHandles.erase(path);
    fclose(fileHandle);
}

std::string Fat32Transfer::transferThreadLoop(dumpingConfig config) {
    while(!breakThreadLoop) {
        std::unique_lock<std::mutex> lck(this->mutex);
        this->condVariable.wait(lck, [this]{ return !this->chunks.empty(); });

        if (this->chunks.empty()) {
            WHBLogPrint("Woken up without chunk!");
            OSMemoryBarrier();
            continue;
        }
        TransferCommands chunk = this->chunks.front();
        this->chunks.pop();
        OSMemoryBarrier();
        lck.unlock();
        this->condVariable.notify_all();

        std::visit(overloaded{
            [this](CommandMakeDir& arg) {
                WHBLogPrintf("Make directory: path=%s", arg.dirPath.c_str());
                createPath(arg.dirPath.c_str());
            },
            [this](CommandWrite& arg) {
                WHBLogPrintf("Write File: path=%s size=%zu closeFileAtEnd=%s", arg.filePath.c_str(), arg.chunkSize, arg.closeFileAtEnd == true ? "true" : "false");

                FILE* destFileHandle = this->getFileHandle(arg.filePath);

                if (destFileHandle == nullptr) {
                    this->closeFileHandle(arg.filePath);
                    free(arg.chunkBuffer);

                    this->error += "Couldn't open the file to copy to!\n";
                    this->error += "For SD cards: Make sure it isn't locked\nby the write-switch on the side!\n\nDetails:\n";
                    this->error += "Error "+std::to_string(errno)+" after creating SD card file:\n";
                    this->error += arg.filePath;
                    this->breakThreadLoop = true;
                    return;
                }
                
                if (uint32_t bytesWritten = fwrite(arg.chunkBuffer, sizeof(uint8_t), arg.chunkSize, destFileHandle); bytesWritten != arg.chunkSize) {
                    this->closeFileHandle(arg.filePath);
                    free(arg.chunkBuffer);

                    error += "Failed to write data to dumping device!\n";
                    if (errno == ENOSPC) error += "There's no space available on the USB/SD card!\n";
                    if (errno == EIO) error += "For discs: Make sure that it's clean!\nDumping is very sensitive to tiny issues!\n";
                    error += "\nDetails:\n";
                    error += "Error "+std::to_string(errno)+" when writing data from:\n";
                    error += "to:\n";
                    error += arg.filePath;
                    this->breakThreadLoop = true;
                    return;
                }

                if (arg.closeFileAtEnd)
                    this->closeFileHandle(arg.filePath);

                free(arg.chunkBuffer);
            },
            [this](CommandStopThread& arg) {
                WHBLogPrintf("Stop Thread: ");
                this->breakThreadLoop = true;
            }
        }, chunk);
    }

    WHBLogPrint("Cleaning up the thread...");
    WHBLogFreetypeDraw();

    // Finish queue and close all file handles that are still left
    auto openHandleIt = this->fileHandles.begin();
    while (openHandleIt != this->fileHandles.end()) {
        fclose(openHandleIt->second);
        this->fileHandles.erase(openHandleIt);
        openHandleIt++;
    }
    // todo: clean up the remaining chunks too by freeing the remaining memory and popping em off. or change above to use while loop
    return this->error;
}