#pragma once

#include <coreinit/screen.h>
#include <coreinit/energysaver.h>
#include <coreinit/mcp.h>
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
#include <coreinit/semaphore.h>
#include <coreinit/debug.h>

#include <sysapp/launch.h>
#include <nn/acp.h>
#include <nn/act.h>
#include <nn/ac.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <proc_ui/procui.h>
#include <vpad/input.h>
#include <padscore/wpad.h>
#include <padscore/kpad.h>

#ifdef __cplusplus
extern "C" {
#endif
    int32_t OSGetSystemMode();
#ifdef __cplusplus
}
#endif

#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <climits>
#include <sys/stat.h>
#include <libgen.h>
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
#include <regex>
#include <thread>
#include <chrono>
#include <optional>
#include <type_traits>
#include <filesystem>

#include <thread>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <future>
#include <variant>
#include <semaphore>

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

[[maybe_unused]]
static inline void ltrim(std::wstring& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
        return !std::isspace(ch);
        }));
}

[[maybe_unused]]
static inline void rtrim(std::wstring& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

[[maybe_unused]]
static inline void trim(std::wstring& s) {
    ltrim(s);
    rtrim(s);
}

[[maybe_unused]]
static inline void replaceAll(std::wstring& str, const std::wstring& oldSubstr, const std::wstring& newSubstr) {
    size_t pos = 0;
    while ((pos = str.find(oldSubstr, pos)) != std::wstring::npos) {
        str.replace(pos, oldSubstr.length(), newSubstr);
        pos += newSubstr.length();
    }
}

[[maybe_unused]]
static inline std::wstring toWstring(const std::string& stringToConvert) {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(stringToConvert);
}

// Enums and Structs

enum class DUMP_METHOD {
    FAT,
    NTFS // TODO: Add NTFS support
};

enum class TITLE_LOCATION {
    UNKNOWN,
    NAND,
    USB,
    DISC
};

enum class DUMP_TYPE_FLAGS {
    NONE = 0 << 0,
    GAME = 1 << 0,
    UPDATE = 1 << 1,
    DLC = 1 << 2,
    SYSTEM_APP = 1 << 3,
    SAVES = 1 << 4,
    CUSTOM = 1 << 5
};

template<>
struct enable_bitmask_operators<DUMP_TYPE_FLAGS> {
    static constexpr bool enable = true;
};

template <typename T1, typename T2>
constexpr bool HAS_FLAG(T1 flags, T2 test_flag) { return (flags & (T1)test_flag) == (T1)test_flag; }

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct dumpingConfig {
    DUMP_TYPE_FLAGS filterTypes = DUMP_TYPE_FLAGS::NONE;
    DUMP_TYPE_FLAGS dumpTypes = DUMP_TYPE_FLAGS::NONE;
    nn::act::PersistentId accountId = 0;
    bool scanTitleSize = USE_DEBUG_STUBS == 0;
    bool dumpAsDefaultUser = true;
    bool queue = false;
    DUMP_METHOD dumpMethod = DUMP_METHOD::FAT;
    std::string dumpTarget;
    uint32_t debugCacheSize = 1024 * 4;
};

struct userAccount {
    bool currAccount = false;
    bool defaultAccount = false;
    bool networkAccount;
    bool passwordCached;
    std::wstring miiName;
    std::string accountId;
    std::string persistentIdString;
    nn::act::SlotNo slot;
    nn::act::PersistentId persistentId;
};

struct titleCommonSave {
    std::string path;
    TITLE_LOCATION location;
};

struct titleUserSave {
    nn::act::PersistentId userId;
    std::string path;
    TITLE_LOCATION location;
};

struct titlePart {
    std::string posixPath;
    std::string outputPath;
    uint16_t version;
    MCPAppType type;
    uint32_t titleHighId;
    TITLE_LOCATION location;
};

struct savePart {
    std::vector<titleUserSave> userSaves;
    std::optional<titleCommonSave> commonSave;
    std::string savePath;
    std::string outputSuffix;
    std::string outputPath;
    uint32_t titleHighId;
};

struct folderPart {
    std::string inputPath;
    std::string outputPath;
    TITLE_LOCATION location;
};

struct filePart {
    uint8_t* srcBuffer;
    uint64_t srcBufferSize;
    std::string outputFolder;
    std::string outputFile;
    TITLE_LOCATION location;
};

struct titleEntry {
    uint32_t titleLowId;
    std::wstring shortTitle;
    std::wstring productCode;
    std::string folderName;

    std::optional<titlePart> base;
    std::optional<titlePart> update;
    std::optional<titlePart> dlc;
    std::optional<savePart> saves;
    std::optional<folderPart> customFolder;
    std::optional<filePart> customFile;
};

#if USE_DEBUG_STUBS
#define IS_CEMU_PRESENT() (OSGetSystemMode() == 0)
#define USE_WUT_DEVOPTAB() (IS_CEMU_PRESENT() && USE_RAMDISK == 0)
#define USE_LIBMOCHA() (!IS_CEMU_PRESENT())
#else
#define IS_CEMU_PRESENT() false
#define USE_WUT_DEVOPTAB() (IS_CEMU_PRESENT())
#define USE_LIBMOCHA() (!IS_CEMU_PRESENT())
#endif