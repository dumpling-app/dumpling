#pragma once
#include "../common.h"
#include <coreinit/debug.h>

#define BUFFER_SIZE_ALIGNMENT 64
#define BUFFER_SIZE (1024 * BUFFER_SIZE_ALIGNMENT)
#define BUFFER_DEBUG_MULTIPLIER 1

struct MemTester
{
    MemTester()
    {
        chonky.resize(1024 * 512 + (rand()%10000), 0xE7);
    }

    ~MemTester()
    {
        for(uint32_t offset=0; offset < (uint32_t)chonky.size(); offset++)
        {
            if(chonky[offset] == 0xE7)
                continue;
            OSReport("Memory corruption detected at offset +0x%08x (address %08x)\n", offset, (uint32_t)(uintptr_t)chonky.data() + offset);
            std::string offsetError = "MEMORY CORRUPTION OCCURED AT OFFSET: "+std::to_string(offset)+"\n";
            OSFatal(offsetError.c_str());
        }
    }

    std::vector<uint8_t> chonky;
};

struct CommandMakeDir {
    std::string dirPath;
    MemTester test;
};

struct CommandWrite {
    std::string filePath;
    alignas(BUFFER_SIZE_ALIGNMENT) std::array<uint8_t, BUFFER_SIZE*BUFFER_DEBUG_MULTIPLIER> chunkBuffer;
    size_t chunkSize = 0;
    bool closeFileAtEnd = false;
    MemTester test;
};

struct CommandStopThread {
    uint32_t __placeholder = 0;
    MemTester test;
};

using TransferCommands = std::variant<CommandMakeDir, CommandWrite, CommandStopThread>;
using ChunkFile = void;

class TransferInterface {
public:
    TransferInterface(dumpingConfig config);
    virtual ~TransferInterface();

    void submitMakeFolder(std::string& dirPath);
    void submitWriteFile(std::string& filePath, std::array<uint8_t, BUFFER_SIZE*BUFFER_DEBUG_MULTIPLIER>& buffer, size_t size, bool closeFileAtEnd);
    void submitStopThread();
    const uint32_t maxQueueSize = 30;
    
    bool hasFailed();
    std::string getFailReason();
protected:
    virtual std::string transferThreadLoop(dumpingConfig config) = 0;

    template <typename T, typename... Args>
    void submitCommand(Args&&... args);

    std::future<std::string> transferError;
    std::thread transferThread;

    std::mutex mutex;
    OSSemaphore countSemaphore;
    OSSemaphore maxSemaphore;
    std::queue<TransferCommands> chunks;
};