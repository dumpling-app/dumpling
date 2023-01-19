#include "transfer.h"
#include "./../gui.h"
#include "./../filesystem.h"

TransferInterface::TransferInterface(const dumpingConfig& config) {
    OSInitSemaphore(&this->countSemaphore, 0);
    OSInitSemaphore(&this->maxSemaphore, (int32_t)this->maxQueueSize);
    this->chunks = std::queue<TransferCommands>();
    OSMemoryBarrier();
    this->transferThread = std::thread([this, config]() {
        this->transferThreadLoop(config);
    });
}

TransferInterface::~TransferInterface() = default;


template <class T, class... Args>
bool TransferInterface::submitCommand(Args&&... args) {
    while (true) {
        if (this->hasStopped())
            return false;

        int32_t obtainedLock = OSTryWaitSemaphore(&this->maxSemaphore);
        if (obtainedLock > 0)
            break;

        sleep_for(1ms);
    }
    OSMemoryBarrier();
    std::unique_lock<std::mutex> lck(this->mutex);

    this->chunks.emplace(std::in_place_type<T>, std::forward<Args>(args)...);
    OSMemoryBarrier();
    lck.unlock();
    OSSignalSemaphore(&this->countSemaphore);
    return true;
}

bool TransferInterface::submitSwitchFolder(const std::string& dirPath) {
    return this->submitCommand<CommandSwitchDir>(dirPath);
}

bool TransferInterface::submitWriteFolder(const std::string& dirPath) {
    return this->submitCommand<CommandMakeDir>(dirPath);
}

bool TransferInterface::submitWriteFile(const std::string& filePath, size_t fileSize, uint8_t* buffer, uint32_t bufferSize, bool closeFileAtEnd) {
    return this->submitCommand<CommandWrite>(filePath, fileSize, buffer, bufferSize, closeFileAtEnd);
}

bool TransferInterface::submitStopThread() {
    return this->submitCommand<CommandStopThread>();
}

bool TransferInterface::hasStopped() {
    return this->threadStopped;
}

std::optional<std::wstring> TransferInterface::getStopError() {
    return !this->threadStopped ? std::nullopt : this->threadStoppedError;
}