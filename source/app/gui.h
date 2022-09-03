#include "common.h"
#include "../utils/log_freetype.h"

bool initializeGUI();
void shutdownGUI();
void exitApplication(bool shutdown);

void printToLog(const char *fmt, ...);
void guiSafeLog(const char* fmt, ...);