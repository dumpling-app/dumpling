#pragma once
#include "../common.h"

#define BUFFER_SIZE_ALIGNMENT 64
#define BUFFER_SIZE (1024 * BUFFER_SIZE_ALIGNMENT)

struct CommandMakeDir {
    std::string dirPath;
};

struct CommandWrite {
    std::string filePath;
    uint8_t* chunkBuffer = nullptr;
    size_t chunkSize = 0;
    bool closeFileAtEnd = false;
};

struct CommandStopThread {
    uint32_t __placeholder = 0;
};

using TransferCommands = std::variant<CommandMakeDir, CommandWrite, CommandStopThread>;
using ChunkFile = void;

class TransferInterface {
public:
    TransferInterface(dumpingConfig config);
    virtual ~TransferInterface();

    void submitMakeFolder(std::string& dirPath);
    void submitWriteFile(std::string& filePath, uint8_t* buffer, size_t size, bool closeFileAtEnd);
    void submitStopThread();
    
    bool hasFailed();
    std::string getFailReason();
protected:
    virtual std::string transferThreadLoop(dumpingConfig config) = 0;

    template <typename T, typename... Args>
    void submitCommand(Args&&... args);

    std::future<std::string> transferError;
    std::thread transferThread;

    std::mutex mutex;
    std::condition_variable_any condVariable;
    const uint32_t maxChunks = 10;
    std::queue<TransferCommands> chunks;
};