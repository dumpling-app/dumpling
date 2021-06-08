#include "fat32.h"
#include "gui.h"

void Fat32Transfer::initializeTransfer() {
    WHBLogPrint("Started transfer!");
    WHBLogConsoleDraw();
    this->transferThread = std::thread(&Fat32Transfer::transferThreadLoop, this);
}

void Fat32Transfer::stopTransfer() {
    WHBLogPrint("Stopped transfer!");
    WHBLogConsoleDraw();
    this->stopTransferLoop = true;
    this->transferThread.join();
}

FILE* Fat32Transfer::getFileHandle(std::string& path) {
    auto fileHandle = this->fileHandles.find(path);
    if (fileHandle != this->fileHandles.end()) {
        return fileHandle->second;
    }
    return this->fileHandles.emplace(path, fopen(path.c_str(), "wb")).first->second;
}

void Fat32Transfer::transferThreadLoop() {
    while(!this->stopTransferLoop) {
        while (!this->chunks.empty()) {
            this->lock.lock();
            TransferChunk chunk = this->chunks.back();
            this->chunks.pop();
            this->lock.unlock();
            currChunkSize--;

            FILE* destFileHandle = getFileHandle(chunk.destFile);
            
            uint32_t bytesWritten = 0;
            if ((bytesWritten = fwrite(chunk.chunkBuffer, sizeof(uint8_t), chunk.chunkSize, destFileHandle)) < chunk.chunkSize) {
                for (uint32_t i=0; i<30; i++) {
                    WHBLogPrintf("Tried writing %u bytes, but wrote %u", chunk.chunkSize, bytesWritten);
                    WHBLogConsoleDraw();
                }
            }

            if (chunk.closeFileAtEnd) {
                fclose(destFileHandle);
                fileHandles.erase(chunk.destFile);
            }

            free(chunk.chunkBuffer);
        }
    }

    // Finish queue and close all file handles that are still left
    this->lock.lock();
    auto openHandleIt = this->fileHandles.begin();
    while (openHandleIt != this->fileHandles.end()) {
        fclose(openHandleIt->second);
        this->fileHandles.erase(openHandleIt);
        openHandleIt++;
    }
    // todo: clean up the remaining chunks too by freeing the remaining memory and popping em off. or change above to use while loop
    this->lock.unlock();

    WHBLogPrint("Finished the whole queue (in dumping thread)!");
    WHBLogConsoleDraw();
}