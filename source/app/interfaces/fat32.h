#include "../common.h"
#include "transfer.h"

class Fat32Transfer : public TransferInterface {
public:
    explicit Fat32Transfer(const dumpingConfig& config);

    ~Fat32Transfer() override;

    static std::vector<std::pair<std::string, std::string>> getDrives();
    static uint64_t getDriveSpace(const std::string& drivePath);
    static bool deletePath(const std::string &path, const std::function<void()>& callback_updateGUI);
protected:
    void transferThreadLoop(dumpingConfig config) override;

    static constexpr const char* targetDirectoryName = "Dumpling";
    std::string fatTarget;

    std::wstring error;

    struct DIRPtr;
    std::unique_ptr<DIRPtr> currDir;

    struct FILPtr;
    static size_t filePtrAlignment;
    FILPtr* currFileHandle = nullptr;
    std::string currFilePath;
    uint8_t openFile(const std::string &path, size_t prepareSize);
    uint8_t closeFile(const std::string& path);

    struct FATFSPtr;
    FATFSPtr* fs{};
};