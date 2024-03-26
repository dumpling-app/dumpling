#include "common.h"

bool dumpQueue(std::vector<std::shared_ptr<titleEntry>>& queue, dumpingConfig& config, std::optional<dumpFileFilter*> filter = std::nullopt);
bool dumpDisc();
void dumpMLC();
void dumpOnlineFiles();
void dumpAmiibo();
void dumpSpotpass();
void cleanDumpingProcess();