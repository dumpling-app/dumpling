#include "common.h"

bool showLoadingScreen();
void showMainMenu();

uint8_t showDialogPrompt(const char* message, const char* button1, const char* button2);
void showDialogPrompt(const char* message, const char* button);
bool showOptionMenu(dumpingConfig& config, bool showAccountOption);
void setErrorPrompt(const char* message);
void showErrorPrompt(const char* button);
void clearScreen();