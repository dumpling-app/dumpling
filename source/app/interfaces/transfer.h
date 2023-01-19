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
    size_t fileSize;
    uint8_t* chunkBuffer;
    uint32_t chunkSize;
    bool closeFileAtEnd;
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
    bool submitWriteFile(const std::string& filePath, size_t fileSize, uint8_t* buffer, uint32_t bufferSize, bool closeFileAtEnd);
    bool submitStopThread();

    const uint32_t maxQueueSize = 64;

    bool hasStopped();
    std::optional<std::wstring> getStopError();
protected:
    virtual void transferThreadLoop(dumpingConfig config) = 0;

    template <typename T, typename... Args>
    bool submitCommand(Args&&... args);

    bool runThreadLoop = true;
    std::atomic<bool> threadStopped = false;
    std::optional<std::wstring> threadStoppedError;

    std::thread transferThread;

    std::mutex mutex;
    OSSemaphore countSemaphore{};
    OSSemaphore maxSemaphore{};
    std::queue<TransferCommands> chunks;
};