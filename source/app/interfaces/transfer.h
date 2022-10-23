#pragma once
#include "../common.h"

struct CommandSwitchDir {
    const std::string dirPath;
};

struct CommandMakeDir {
    const std::string dirPath;
};

struct CommandWrite {
    const std::string filePath;
    uint8_t* chunkBuffer;
    size_t chunkSize = 0;
    bool closeFileAtEnd = false;
};

struct CommandStopThread {
    uint32_t placeholder = 0;
};

using TransferCommands = std::variant<CommandSwitchDir, CommandMakeDir, CommandWrite, CommandStopThread>;

class TransferInterface {
public:
    explicit TransferInterface(const dumpingConfig& config);
    virtual ~TransferInterface();

    bool submitSwitchFolder(const std::string& dirPath);
    bool submitWriteFolder(const std::string& dirPath);
    bool submitWriteFile(const std::string& filePath, uint8_t* buffer, size_t size, bool closeFileAtEnd);
    bool submitStopThread();

    const uint32_t maxQueueSize = 64;

    bool hasStopped();
    std::optional<std::string> getStopError();
protected:
    virtual void transferThreadLoop(dumpingConfig config) = 0;

    template <typename T, typename... Args>
    bool submitCommand(Args&&... args);

    bool runThreadLoop = true;
    std::atomic<bool> threadStopped = false;
    std::optional<std::string> threadStoppedError;

    std::thread transferThread;

    std::mutex mutex;
    OSSemaphore countSemaphore{};
    OSSemaphore maxSemaphore{};
    std::queue<TransferCommands> chunks;
};