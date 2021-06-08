#include "common.h"
#include "transfer.h"

class Fat32Transfer : public TransferInterface {
public:
    void initializeTransfer() override;
    void stopTransfer() override;
    void transferThreadLoop() override;
    FILE* getFileHandle(std::string& path);
};