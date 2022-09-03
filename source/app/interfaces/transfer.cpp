#include "transfer.h"
#include "./../gui.h"
#include "./../filesystem.h"

TransferInterface::TransferInterface(dumpingConfig config) {
    this->chunks = std::queue<TransferCommands>();
    std::packaged_task<std::string(dumpingConfig)> threadTask(std::bind(&TransferInterface::transferThreadLoop, this, config));
    this->transferError = threadTask.get_future();
    this->transferThread = std::thread(std::move(threadTask), config);
}

TransferInterface::~TransferInterface() {
    this->submitStopThread();
    this->transferThread.join();
}

template <typename T, typename... Args>
void TransferInterface::submitCommand(Args&&... args) {
    std::unique_lock<std::mutex> lck(this->mutex);
    this->condVariable.wait(lck, [this]{ return this->chunks.size() < this->maxChunks; });

    TransferCommands& command = this->chunks.emplace(std::in_place_type<T>, std::forward<Args>(args)...);
    if constexpr (std::is_same<T, CommandWrite>::value) {
        WHBLogPrintf("Send File [idx=%zu]: path=%s size=%zu closeFileAtEnd=%s", this->chunks.size(), std::get<CommandWrite>(command).filePath.c_str(), std::get<CommandWrite>(command).chunkSize, std::get<CommandWrite>(command).closeFileAtEnd == true ? "true" : "false");
    }
    OSMemoryBarrier();
    lck.unlock();
    this->condVariable.notify_all();
}

bool TransferInterface::hasFailed() {
    return this->transferError.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void TransferInterface::submitMakeFolder(std::string& dirPath) {
    this->submitCommand<CommandMakeDir>(dirPath);
}

void TransferInterface::submitWriteFile(std::string& filePath, uint8_t* buffer, size_t size, bool closeFileAtEnd) {
    this->submitCommand<CommandWrite>(filePath, buffer, size, closeFileAtEnd);
}

void TransferInterface::submitStopThread() {
    this->submitCommand<CommandStopThread>();
}

std::string TransferInterface::getFailReason() {
    return this->transferError.get();
}