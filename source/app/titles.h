#include "common.h"

extern std::vector<titleEntry> installedTitles;

bool getTitles();
bool loadTitles(bool skipDiscs);
std::reference_wrapper<titleEntry> getTitleWithName(std::string nameOfTitle);

std::string normalizeTitle(std::string& unsafeTitle);
bool isBase(MCPAppType type);
bool isUpdate(MCPAppType type);
bool isDLC(MCPAppType type);
bool isSystemApp(MCPAppType type);