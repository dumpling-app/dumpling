#include "../common.h"
#include "transfer.h"

class StubTransfer : public TransferInterface {
public:
    explicit StubTransfer(const dumpingConfig& config);
    ~StubTransfer() override;

    static std::vector<std::pair<std::string, std::string>> getDrives();

protected:
    void transferThreadLoop(dumpingConfig config) override;

    FILE* getFileHandle(const std::string& path);
    void closeFileHandle(const std::string& path);

    const std::string sdPath = "fs:/vol/external01";

    std::wstring error;
    std::unordered_map<std::string, FILE*> fileHandles;
};