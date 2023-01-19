#include "common.h"

void showLoadingScreen();
void showMainMenu();

uint8_t showDialogPrompt(const wchar_t* message, const wchar_t* button1, const wchar_t* button2);
void showDialogPrompt(const wchar_t* message, const wchar_t* button);
bool showOptionMenu(dumpingConfig& config, bool showAccountOption);
void setErrorPrompt(const wchar_t* message);
void setErrorPrompt(std::wstring message);
void showErrorPrompt(const wchar_t* button);