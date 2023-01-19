#include "common.h"

extern std::vector<std::shared_ptr<titleEntry>> installedTitles;

bool checkForDiscTitles(int32_t mcpHandle);
bool loadTitles(bool skipDiscs);
std::optional<std::shared_ptr<titleEntry>> getTitleWithName(std::string& nameOfTitle);

std::string normalizeFolderName(std::wstring unsafeString);
void decodeXMLEscapeLine(std::wstring& xmlString);
bool isBase(MCPAppType type);
bool isUpdate(MCPAppType type);
bool isDLC(MCPAppType type);
bool isSystemApp(MCPAppType type);