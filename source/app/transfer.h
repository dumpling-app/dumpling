#pragma once
#include "common.h"

typedef void ChunkFile;

struct TransferChunk {
    std::string destFile;
    uint8_t* chunkBuffer = nullptr;
    size_t chunkSize = 0;
    bool openFileAtEnd = false;
    bool closeFileAtEnd = false;
};

class TransferInterface {
public:
    TransferInterface();
    virtual void initializeTransfer() = 0;
    virtual void stopTransfer() = 0;
    virtual void transferThreadLoop() = 0;
    void addChunk(std::string destFilePath, uint8_t* buffer, size_t byteSize, bool closeFileAfterWriting);

protected:
    std::thread transferThread;
    std::unordered_map<std::string, FILE*> fileHandles;

    std::mutex lock;
    std::atomic_uint32_t maxChunks = 50;
    std::atomic_uint32_t currChunkSize = 0;
    std::queue<TransferChunk> chunks;
    std::atomic_bool stopTransferLoop = false;
};

#define BUFFER_SIZE_ALIGNMENT 64
#define BUFFER_SIZE (1024 * BUFFER_SIZE_ALIGNMENT)

void startTransfer(TransferInterface* interface);

void stopTransfer(TransferInterface* interface);