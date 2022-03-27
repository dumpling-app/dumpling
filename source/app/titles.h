#include "common.h"

extern std::vector<titleEntry> installedTitles;

bool checkForDiscTitles(int32_t mcpHandle);
bool loadTitles(bool skipDiscs);
std::optional<titleEntry> getTitleWithName(std::string& nameOfTitle);

std::string normalizeFolderName(std::string& unsafeTitle);
void decodeXMLEscapeLine(std::string& xmlString);
bool isBase(MCPAppType type);
bool isUpdate(MCPAppType type);
bool isDLC(MCPAppType type);
bool isSystemApp(MCPAppType type);