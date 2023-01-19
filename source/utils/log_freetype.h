#include <coreinit/memheap.h>
#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>

// Initialization functions
bool WHBLogFreetypeInit();
void WHBLogFreetypeFree();

// Logging
void WHBLogFreetypePrintf(const wchar_t *fmt, ...);
void WHBLogFreetypePrint(const wchar_t *line);
void WHBLogFreetypeDraw();
void WHBLogFreetypeClear();

// Menu
void WHBLogFreetypeStartScreen();
void WHBLogFreetypePrintfAtPosition(uint32_t position, const wchar_t *fmt, ...);
void WHBLogFreetypePrintAtPosition(uint32_t position, const wchar_t *line);
void WHBLogFreetypeScreenPrintBottom(const wchar_t *line);
uint32_t WHBLogFreetypeScreenSize();
uint32_t WHBLogFreetypeGetScreenPosition();
void WHBLogFreetypeDrawScreen();

// Rendering options
void WHBLogFreetypeSetFontColor(uint32_t color);
void WHBLogFreetypeSetBackgroundColor(uint32_t color);
bool WHBLogFreetypeSetFontSize(uint8_t size);