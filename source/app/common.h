#pragma once

#include <coreinit/screen.h>
#include <coreinit/energysaver.h>
#include <coreinit/mcp.h>
#include <nn/acp.h>
#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/ios.h>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/event.h>
#include <coreinit/memheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/foreground.h>
#include <coreinit/title.h>
#include <coreinit/launch.h>

#include <sysapp/launch.h>
#include <nn/act.h>
#include <nn/ac.h>
#include <whb/log.h>
#include <proc_ui/procui.h>
#include <vpad/input.h>
#include <padscore/wpad.h>
#include <padscore/kpad.h>

#ifdef __cplusplus
extern "C" {
#endif
    int32_t OSShutdown(int32_t status);
    ACPResult ACPInitialize();
    ACPResult ACPFinalize();
#ifdef __cplusplus
}
#endif

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <map>
#include <list>
#include <algorithm>
#include <iterator>
#include <functional>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <locale>
#include <codecvt>
#include <thread>
#include <chrono>
#include <optional>
#include <type_traits>
#include <filesystem>

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::this_thread;


// Templated bitmask operators

template<typename E>
struct enable_bitmask_operators {
    static constexpr bool enable = false;
};

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type operator| (E lhs, E rhs) {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type operator& (E lhs, E rhs) {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

// String trim functions

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char8_t ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](char8_t ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

static inline void replaceAll(std::string& str, const std::string& oldSubstr, const std::string& newSubstr) {
    size_t pos = 0;
    while ((pos = str.find(oldSubstr, pos)) != std::string::npos) {
        str.replace(pos, oldSubstr.length(), newSubstr);
        pos += newSubstr.length();
    }
}

// Enums and Structs

enum class dumpLocation {
    Unknown,
    SDFat,
    USBFat,
    USBExFAT, // TODO: Add exFAT support
    USBNTFS, // TODO: Add NTFS support
};

enum class titleLocation {
    Unknown,
    Nand,
    USB,
    Disc,
};

enum class dumpTypeFlags {
    Game = 1 << 0,
    Update = 1 << 1,
    DLC = 1 << 2,
    SystemApp = 1 << 3,
    Saves = 1 << 4,
    Custom = 1 << 5
};

template<>
struct enable_bitmask_operators<dumpTypeFlags> {
    static constexpr bool enable = true;
};

template <typename T1, typename T2>
constexpr bool HAS_FLAG(T1 flags, T2 test_flag) { return (flags & (T1)test_flag) == (T1)test_flag; }

struct dumpingConfig {
    dumpTypeFlags filterTypes;
    dumpTypeFlags dumpTypes;
    nn::act::PersistentId accountId = 0;
    bool dumpAsDefaultUser = true;
    bool queue = false;
    bool ignoreCopyErrors = false;
    dumpLocation location = dumpLocation::SDFat;
};

struct userAccount {
    bool currAccount = false;
    bool defaultAccount = false;
    bool networkAccount;
    bool passwordCached;
    std::string miiName;
    std::string persistentIdString;
    nn::act::SlotNo slot;
    nn::act::PersistentId persistentId;
};

struct titleCommonSave {
    std::string path;
    titleLocation location;
};

struct titleUserSave {
    nn::act::PersistentId userId;
    std::string path;
    titleLocation location;
};

struct titlePart {
    std::string path;
    std::string outputPath;
    uint16_t version;
    MCPAppType type;
    uint32_t titleHighId;
    titleLocation location;
};

struct savePart {
    std::vector<titleUserSave> userSaves;
    std::optional<titleCommonSave> commonSave;
    std::string savePath;
    uint32_t titleHighId;
};

struct customPart {
    std::optional<std::string> inputPath;
    std::optional<std::pair<uint8_t*, uint64_t>> inputBuffer;
    std::string outputPath;
    titleLocation location;
};

struct titleEntry {
    uint32_t titleLowId;
    std::string shortTitle = "";
    std::string productCode = "";
    std::string folderName = "";
    
    std::optional<titlePart> base;
    std::optional<titlePart> update;
    std::optional<titlePart> dlc;
    std::optional<savePart> saves;
    std::optional<customPart> custom;
};