#include "../common.h"
#include "transfer.h"

class Fat32Transfer : public TransferInterface {
public:
    Fat32Transfer(dumpingConfig config) : TransferInterface(config) {}
    
protected:
    std::string transferThreadLoop(dumpingConfig config) override;

    FILE* getFileHandle(std::string& path);
    void closeFileHandle(std::string& path);

    bool breakThreadLoop = false;
    std::string error = "";
    std::unordered_map<std::string, FILE*> fileHandles;

    const uint32_t maxQueueSize = 25;
};