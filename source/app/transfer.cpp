#include "transfer.h"
#include "progress.h"
#include "gui.h"

// todo: is this constructor really necessary...
TransferInterface::TransferInterface() {
    this->chunks = std::queue<TransferChunk>();
    this->stopTransferLoop = false;
    // this->transferThread = std::thread(); // todo: see if I can make a thread out of the perhaps not yet constructed fat32Interface::threadloop using a lambda
}

void TransferInterface::addChunk(std::string destFilePath, uint8_t* buffer, size_t byteSize, bool closeFileAfterWriting) {
    while(this->currChunkSize >= this->maxChunks) {
        //OSSleepTicks(OSMillisecondsToTicks(5));
        showCurrentProgress();
    }

    TransferChunk newChunk;
    newChunk.chunkBuffer = (uint8_t*)memalign(BUFFER_SIZE_ALIGNMENT, byteSize);
    memcpy(newChunk.chunkBuffer, buffer, byteSize);
    newChunk.chunkSize = byteSize;
    newChunk.destFile = destFilePath;
    newChunk.closeFileAtEnd = closeFileAfterWriting;

    currChunkSize++;
    this->lock.lock();
    this->chunks.emplace(newChunk);
    this->lock.unlock();
}

// void TransferInterface::startThread() {
//     std::thread(&(this->transferThreadLoop));
// }

// TransferInterface::~TransferInterface() {
//     WHBLogPrint("Called base deconstructor and stopping thread!");
//     this->stopTransfer = true;
//     this->transferThread.join();
// }

// void startTransfer(TransferInterface* interface) {
//     interface->dataChunks = std::stack<TransferChunk>(); // todo: Shouldn't be necessary
//     interface->transferThread = std::thread(fat32TransferLoop, (void*)interface);
//     interface->transferThread.detach();
// }

// void stopTransfer(TransferInterface* interface) {
//     interface->stopTransfer = true;
//     interface->transferThread.join();
// }