#include <sys/unistd.h>
#include "stub.h"
#include "./../menu.h"
#include "./../filesystem.h"

StubTransfer::StubTransfer(const dumpingConfig& config) : TransferInterface(config) {
}

StubTransfer::~StubTransfer() {
    this->submitStopThread();
    while (!this->hasStopped()) {
        sleep_for(50ms);
    }
    if (this->transferThread.joinable()) {
        this->transferThread.join();
    }
}

std::vector<std::pair<std::string, std::string>> StubTransfer::getDrives() {
    std::vector<std::pair<std::string, std::string>> targets;
    targets.emplace_back("fs:/vol/external01/", "SD Card (through WUT)");
    return targets;
}

void StubTransfer::transferThreadLoop(dumpingConfig config) {
    while(runThreadLoop) {
        OSWaitSemaphore(&this->countSemaphore);
        std::unique_lock<std::mutex> lck(this->mutex);

        TransferCommands chunk = this->chunks.front();
        this->chunks.pop();
        lck.unlock();
        OSSignalSemaphore(&this->maxSemaphore);

        std::visit(overloaded{
                [this](CommandSwitchDir& arg) {
                    OSReport("Switch Directory: path=%s\n", arg.dirPath.c_str());
                    chdir((this->sdPath+arg.dirPath).c_str());
                },
                [this](CommandMakeDir& arg) {
                    OSReport("Make Directory: path=%s\n", arg.dirPath.c_str());
                    mkdir((this->sdPath+arg.dirPath).c_str(), ACCESSPERMS);
                },
                [this](CommandWrite& arg) {
                    std::string limitedFilename = arg.filePath;
                    if (limitedFilename.length() >= 50) {
                        limitedFilename = limitedFilename.substr(0, 49)+"\n";
                    }
                    OSReport("Write File: path=%.50s size=%u closeFileAtEnd=%s\n", limitedFilename.c_str(), (uint32_t)arg.chunkSize, arg.closeFileAtEnd ? "true" : "false");

                    FILE* destFileHandle = this->getFileHandle(arg.filePath);

                    if (destFileHandle == nullptr) {
                        this->closeFileHandle(arg.filePath);
                        free(arg.chunkBuffer);
                        this->error += L"Couldn't open the file to copy to!\n";
                        this->error += L"For SD cards: Make sure it isn't locked\nby the write-switch on the side!\n\nDetails:\n";
                        this->error += L"Error "+std::to_wstring(errno)+L" after creating SD card file:\n";
                        this->error += toWstring(arg.filePath);
                        this->runThreadLoop = false;
                        return;
                    }

                    if (size_t bytesWritten = fwrite(arg.chunkBuffer, sizeof(uint8_t), arg.chunkSize, destFileHandle); bytesWritten != arg.chunkSize) {
                        this->closeFileHandle(arg.filePath);
                        free(arg.chunkBuffer);
                        this->error += L"Failed to write data to dumping device!\n";
                        if (errno == ENOSPC) this->error += L"There's no space available on the USB/SD card!\n";
                        if (errno == EIO) this->error += L"For discs: Make sure that its clean!\nDumping is very sensitive to tiny issues!\n";
                        this->error += L"\nDetails:\n";
                        this->error += L"Error "+std::to_wstring(errno)+L" when writing data from:\n";
                        this->error += L"to:\n";
                        this->error += toWstring(arg.filePath);
                        this->runThreadLoop = false;
                        return;
                    }

                    if (arg.closeFileAtEnd)
                        this->closeFileHandle(arg.filePath);
                    free(arg.chunkBuffer);
                },
                [this](CommandStopThread& arg) {
                    OSReport("Stop Thread\n");
                    this->runThreadLoop = false;
                }
        }, chunk);
    }

    OSReport("Cleaning up the thread...\n");

    // Finish queue and close all file handles that are still left
    auto openHandleIt = this->fileHandles.begin();
    while (openHandleIt != this->fileHandles.end()) {
        fclose(openHandleIt->second);
        this->fileHandles.erase(openHandleIt);
        openHandleIt++;
    }
    if (!this->error.empty())
        this->threadStoppedError = this->error;
    this->threadStopped = true;
}

// Should only be called from transfer thread
FILE* StubTransfer::getFileHandle(const std::string& path) {
    auto fileHandle = this->fileHandles.find(path);
    if (fileHandle != this->fileHandles.end()) {
        return fileHandle->second;
    }
    return this->fileHandles.emplace(path, fopen((this->sdPath+path).c_str(), "wb")).first->second;
}

// Should only be called from transfer thread
void StubTransfer::closeFileHandle(const std::string& path) {
    FILE* fileHandle = this->getFileHandle(path);
    this->fileHandles.erase(path);
    fclose(fileHandle);
}