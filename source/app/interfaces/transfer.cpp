#include "transfer.h"
#include "./../gui.h"
#include "./../filesystem.h"

#include <coreinit/debug.h>

TransferInterface::TransferInterface(dumpingConfig config) {
    this->chunks = std::queue<TransferCommands>();
    std::packaged_task<std::string(dumpingConfig)> threadTask(std::bind(&TransferInterface::transferThreadLoop, this, config));
    this->transferError = threadTask.get_future();
    this->transferThread = std::thread(std::move(threadTask), config);
    OSInitSemaphore(&this->countSemaphore, 0);
    OSInitSemaphore(&this->maxSemaphore, this->maxQueueSize);
}

TransferInterface::~TransferInterface() {
    this->submitStopThread();
    this->transferThread.join();
}

template <typename T, typename... Args>
void TransferInterface::submitCommand(Args&&... args) {
    OSWaitSemaphore(&this->maxSemaphore);
    OSMemoryBarrier();
    std::unique_lock<std::mutex> lck(this->mutex);

    TransferCommands& command = this->chunks.emplace(std::in_place_type<T>, std::forward<Args>(args)...);
    if constexpr (std::is_same<T, CommandMakeDir>::value) {
        OSReport("Submit MakeDir [idx=%u]: %.50s\n", (uint32_t)this->chunks.size(), std::get<CommandMakeDir>(command).dirPath.c_str());
    }
    else if constexpr (std::is_same<T, CommandWrite>::value) {
        std::string limitedFilename = std::get<CommandWrite>(command).filePath;
        if (limitedFilename.length() >= 60) {
            limitedFilename = limitedFilename.substr(limitedFilename.length()-50)+"\n";
        }
        OSReport("Submit Write [idx=%u]: path=%.50s size=%u closeFileAtEnd=%s\n", (uint32_t)this->chunks.size(), limitedFilename.c_str(), (uint32_t)std::get<CommandWrite>(command).chunkSize, std::get<CommandWrite>(command).closeFileAtEnd == true ? "true" : "false");
    }
    else if constexpr (std::is_same<T, CommandStopThread>::value) {
        OSReport("Submit Stop [idx=%u]: \n", (uint32_t)this->chunks.size());
    }
    OSMemoryBarrier();
    lck.unlock();
    OSSignalSemaphore(&this->countSemaphore);
    OSMemoryBarrier();
}

bool TransferInterface::hasFailed() {
    return this->transferError.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void TransferInterface::submitMakeFolder(std::string& dirPath) {
    this->submitCommand<CommandMakeDir>(dirPath);
}

void TransferInterface::submitWriteFile(std::string& filePath, std::array<uint8_t, BUFFER_SIZE*BUFFER_DEBUG_MULTIPLIER>& buffer, size_t size, bool closeFileAtEnd) {
    #if BUFFER_DEBUG_MULTIPLIER != 1
    memset(buffer.data()+BUFFER_SIZE, 0, BUFFER_SIZE*BUFFER_DEBUG_MULTIPLIER);
    #endif
    this->submitCommand<CommandWrite>(filePath, buffer, size, closeFileAtEnd);
}

void TransferInterface::submitStopThread() {
    this->submitCommand<CommandStopThread>();
}

std::string TransferInterface::getFailReason() {
    return this->transferError.get();
}