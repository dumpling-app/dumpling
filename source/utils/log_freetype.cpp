#include "log_freetype.h"
#include "libschrift/schrift.h"
#include <mutex>
#include <whb/log.h>
#include <cstring>
#include <cstdarg>
#include <cwchar>
#include <unordered_map>
#include <coreinit/debug.h>

#define CONSOLE_FRAME_HEAP_TAG (0x0002B2B)
#define NUM_LINES (18)
#define LINE_LENGTH (128)

wchar_t queueBuffer[NUM_LINES][LINE_LENGTH];
uint32_t newLines = 0;
uint32_t bottomLines = 0;

bool freetypeHasForeground = false;

uint8_t* frameBufferTVFrontPtr = nullptr;
uint8_t* frameBufferTVBackPtr = nullptr;
uint32_t frameBufferTVSize = 0;
uint8_t* frameBufferDRCFrontPtr = nullptr;
uint8_t* frameBufferDRCBackPtr = nullptr;
uint32_t frameBufferDRCSize = 0;
uint8_t* currTVFrameBuffer = nullptr;
uint8_t* currDRCFrameBuffer = nullptr;

uint32_t fontColor = 0xFFFFFFFF;
uint32_t backgroundColor = 0x0B5D5E00;
SFT fontFace = {};
int32_t cursorSpaceWidth = 0;
uint8_t* fontBuffer;
std::unordered_map<SFT_Glyph, SFT_Image> glyphCache;

#define ENABLE_THREADSAFE FALSE
#if ENABLE_THREADSAFE
std::mutex _mutex;
#define DEBUG_THREADSAFE std::scoped_lock<std::mutex> lck(_mutex);
#else
#define DEBUG_THREADSAFE do {} while(0)
#endif

static void FreetypeSetLine(uint32_t position, const wchar_t *line) {
    DEBUG_THREADSAFE;
    size_t length = wcslen(line);
    memcpy(queueBuffer[newLines - 1], line, length);
    queueBuffer[newLines - 1][length] = L'\0';
}

static void FreetypeAddLine(const char *line) {
    DEBUG_THREADSAFE;
    size_t length = strlen(line);

    if (length > LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }

    if (newLines == NUM_LINES) {
        for (uint32_t i=0; i<NUM_LINES-1; i++) {
            memcpy(queueBuffer[i], queueBuffer[i + 1], LINE_LENGTH);
        }

        size_t wideLength = std::mbstowcs(queueBuffer[newLines - 1], line, length);
        if (wideLength != (size_t)-1) {
            queueBuffer[newLines - 1][wideLength] = L'\0';
        }
    }
    else {
        size_t wideLength = std::mbstowcs(queueBuffer[newLines], line, length);
        if (wideLength != (size_t)-1) {
            queueBuffer[newLines][wideLength] = L'\0';
            newLines++;
        }
    }
}

static void FreetypeAddLine(const wchar_t *line) {
    DEBUG_THREADSAFE;
    size_t length = wcslen(line);

    if (length > LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }

    if (newLines == NUM_LINES) {
        for (uint32_t i=0; i<NUM_LINES-1; i++) {
            memcpy(queueBuffer[i], queueBuffer[i + 1], LINE_LENGTH);
        }

        wmemcpy(queueBuffer[newLines - 1], line, length);
        queueBuffer[newLines - 1][length] = L'\0';
    }
    else {
        wmemcpy(queueBuffer[newLines], line, length);
        queueBuffer[newLines][length] = L'\0';
        newLines++;
    }
}

void drawPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    {
        constexpr uint32_t width = 1280;
        uint32_t v = (x + y * width) * 4;
        currTVFrameBuffer[v + 0] = r;
        currTVFrameBuffer[v + 1] = g;
        currTVFrameBuffer[v + 2] = b;
    }

    {
        constexpr uint32_t width = 896;
        uint32_t v = (x + y * width) * 4;
        currDRCFrameBuffer[v + 0] = r;
        currDRCFrameBuffer[v + 1] = g;
        currDRCFrameBuffer[v + 2] = b;
    }
}

void drawPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const float letterOpacity = (float)a/255.0f;
    const float backgroundOpacity = 1.0f-letterOpacity;
    float R = ((float)r * letterOpacity);
    float G = ((float)g * letterOpacity);
    float B = ((float)b * letterOpacity);

    {
        constexpr uint32_t width = 1280;
        uint32_t v = (x + y * width) * 4;
        currTVFrameBuffer[v + 0] = (uint8_t)(R + ((float)currTVFrameBuffer[v + 0] * backgroundOpacity));
        currTVFrameBuffer[v + 1] = (uint8_t)(G + ((float)currTVFrameBuffer[v + 1] * backgroundOpacity));
        currTVFrameBuffer[v + 2] = (uint8_t)(B + ((float)currTVFrameBuffer[v + 2] * backgroundOpacity));
    }

    {
        constexpr uint32_t width = 896;
        uint32_t v = (x + y * width) * 4;
        currDRCFrameBuffer[v + 0] = (uint8_t)(R + ((float)currDRCFrameBuffer[v + 0] * backgroundOpacity));
        currDRCFrameBuffer[v + 1] = (uint8_t)(G + ((float)currDRCFrameBuffer[v + 1] * backgroundOpacity));
        currDRCFrameBuffer[v + 2] = (uint8_t)(B + ((float)currDRCFrameBuffer[v + 2] * backgroundOpacity));
    }
}

void drawBitmap(SFT_Image *bitmap, int32_t x, int32_t y) {
    uint8_t r = (fontColor >> 16) & 0xff;
    uint8_t g = (fontColor >> 8) & 0xff;
    uint8_t b = (fontColor >> 0) & 0xff;

    int32_t x_max = x + bitmap->width;
    int32_t y_max = y + bitmap->height;

    for (int32_t i=x, p=0; i < x_max; i++, p++) {
        for (int32_t j=y, q=0; j < y_max; j++, q++) {
            if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;

            uint8_t col = ((uint8_t*)bitmap->pixels)[q * bitmap->width + p];
            if (col == 0) continue;
            else if (col == 0xFF) drawPixel(i, j, r, g, b);
            else drawPixel(i, j, r, g, b, col);
        }
    }
}

uint32_t renderLine(uint32_t x, uint32_t y, const wchar_t *string, bool wrap) {
    int32_t pen_x = (int32_t)x;
    int32_t pen_y = (int32_t)y;

    for (; *string; string++) {
        SFT_Glyph gid;
        if (sft_lookup(&fontFace, *string, &gid) == -1) {
            OSReport("[log_freetype] Failed to do character lookup!\n");
            continue;
        }

        SFT_GMetrics mtx;
        if (sft_gmetrics(&fontFace, gid, &mtx) == -1) {
            OSReport("[log_freetype] Failed to extract metrics from character!\n");
            continue;
        }

        if (*string == L'\n') {
            pen_y += mtx.minHeight;
            pen_x = (int32_t)x;
            continue;
        }

        if (!glyphCache.contains(gid)) {
            int32_t textureWidth = (mtx.minWidth + 3) & ~3;
            int32_t textureHeight = mtx.minHeight;

            SFT_Image img{
                    .pixels = nullptr,
                    .width = textureWidth,
                    .height = textureHeight
            };

            if (textureWidth == 0) textureWidth = 4;
            if (textureHeight == 0) textureHeight = 4;

            img.pixels = malloc((uint32_t)(img.width * img.height));
            if (img.pixels == nullptr || sft_render(&fontFace, gid, img) == -1) {
                OSReport("[log_freetype] Failed to render a character!\n");
                continue;
            }
            glyphCache.emplace(gid, img);
        }

        if ((pen_x + (int32_t)mtx.advanceWidth) > 853) {
            if (wrap) {
                pen_y += (int32_t)fontFace.yOffset;
                pen_x = x;
            }
            else {
                return pen_y;
            }
        }

        drawBitmap(&glyphCache[gid], pen_x + (int32_t)mtx.leftSideBearing, pen_y + (int32_t)mtx.yOffset);
        if ((int32_t)x == pen_x && *string == ' ' && cursorSpaceWidth != 0) {
            pen_x += cursorSpaceWidth;
        }
        else {
            pen_x += (int32_t)mtx.advanceWidth;
        }
    }
    return pen_y;
}

void WHBLogFreetypeDraw() {
    if (!freetypeHasForeground) return;

    OSScreenClearBufferEx(SCREEN_TV, backgroundColor);
    OSScreenClearBufferEx(SCREEN_DRC, backgroundColor);
    const int32_t x = 0;

    DEBUG_THREADSAFE;
    for (uint32_t y=0; y<NUM_LINES; y++) {
        renderLine((x+2)*12, (y+2)*24, queueBuffer[y], false);
    }

    DCFlushRange(currTVFrameBuffer, frameBufferTVSize);
    DCFlushRange(currDRCFrameBuffer, frameBufferDRCSize);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    currTVFrameBuffer = (currTVFrameBuffer == frameBufferTVFrontPtr) ? frameBufferTVBackPtr : frameBufferTVFrontPtr;
    currDRCFrameBuffer = (currDRCFrameBuffer == frameBufferDRCFrontPtr) ? frameBufferDRCBackPtr : frameBufferDRCFrontPtr;
}

static uint32_t FreetypeProcCallbackAcquired(void *context) {
    freetypeHasForeground = true;

    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    MEMRecordStateForFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);
    if (frameBufferTVSize) {
        frameBufferTVFrontPtr = (uint8_t*)MEMAllocFromFrmHeapEx(heap, frameBufferTVSize, 0x100);
        frameBufferTVBackPtr = (uint8_t*)frameBufferTVFrontPtr + (1*(1280*720*4));
    }

    if (frameBufferDRCSize) {
        frameBufferDRCFrontPtr = (uint8_t*)MEMAllocFromFrmHeapEx(heap, frameBufferDRCSize, 0x100);
        frameBufferDRCBackPtr = (uint8_t*)frameBufferDRCFrontPtr + (1*(896*480*4));
    }

    OSScreenSetBufferEx(SCREEN_TV, frameBufferTVFrontPtr);
    OSScreenSetBufferEx(SCREEN_DRC, frameBufferDRCFrontPtr);

    OSScreenPutPixelEx(SCREEN_TV, 0, 0, 0xABCDEFFF);
    OSScreenPutPixelEx(SCREEN_DRC, 0, 0, 0xABCDEFFF);
    DCFlushRange(frameBufferTVFrontPtr, frameBufferTVSize);
    DCFlushRange(frameBufferDRCFrontPtr, frameBufferDRCSize);
    currTVFrameBuffer = (((uint32_t*)frameBufferTVFrontPtr)[0] == 0xABCDEFFF) ? frameBufferTVFrontPtr : frameBufferTVBackPtr;
    currDRCFrameBuffer = (((uint32_t*)frameBufferTVFrontPtr)[0] == 0xABCDEFFF) ? frameBufferDRCFrontPtr : frameBufferDRCBackPtr;
    return 0;
}

static uint32_t FreetypeProcCallbackReleased(void *context) {
    freetypeHasForeground = false;

    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    MEMFreeByStateToFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);
    currTVFrameBuffer = nullptr;
    frameBufferTVFrontPtr = nullptr;
    frameBufferTVBackPtr = nullptr;
    currDRCFrameBuffer = nullptr;
    frameBufferDRCFrontPtr = nullptr;
    frameBufferDRCBackPtr = nullptr;
    return 0;
}

bool WHBLogFreetypeInit() {
    // Initialize screen
    OSScreenInit();
    frameBufferTVSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    frameBufferDRCSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    FreetypeProcCallbackAcquired(nullptr);
    OSScreenEnableEx(SCREEN_TV, TRUE);
    OSScreenEnableEx(SCREEN_DRC, TRUE);

    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, FreetypeProcCallbackAcquired, nullptr, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, FreetypeProcCallbackReleased, nullptr, 100);

    uint32_t fontBufferSize;
    OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, (void**)&fontBuffer, &fontBufferSize);

    // Initialize libschrift
    fontFace.xScale = 20;
    fontFace.yScale = 20;
    fontFace.flags = SFT_DOWNWARD_Y;
    fontFace.font = sft_loadmem(fontBuffer, fontBufferSize);

    if (fontFace.font == nullptr) {
        OSReport("[log_freetype] Failed to load the font!\n");
        return true;
    }
    if (WHBLogFreetypeSetFontSize(20)) {
        OSReport("[log_freetype] Failed to set the font size while initializing!\n");
        return true;
    }

    WHBAddLogHandler(FreetypeAddLine);
    return false;
}

void WHBLogFreetypeClear() {
    DEBUG_THREADSAFE;
    newLines = 0;
    bottomLines = 0;
    wmemset((wchar_t*)queueBuffer, L'\0', NUM_LINES * LINE_LENGTH);
}

void WHBLogFreetypeStartScreen() {
    WHBLogFreetypeClear();
}

void WHBLogFreetypeDrawScreen() {
    WHBLogFreetypeDraw();
}

uint32_t WHBLogFreetypeGetScreenPosition() {
    return newLines;
}

void WHBLogFreetypePrintfAtPosition(uint32_t position, const wchar_t *fmt, ...) {
    DEBUG_THREADSAFE;
    if (position < 0 || position > NUM_LINES) return;

    va_list va;
    va_start(va, fmt);
    vswprintf(queueBuffer[position], LINE_LENGTH, fmt, va);
    va_end(va);
}

void WHBLogFreetypePrintf(const wchar_t *fmt, ...) {
    wchar_t formattedLine[LINE_LENGTH];
    
    va_list va;
    va_start(va, fmt);
    vswprintf(formattedLine, LINE_LENGTH, fmt, va);
    va_end(va);
    FreetypeAddLine(formattedLine);
}

void WHBLogFreetypePrintAtPosition(uint32_t position, const wchar_t *line) {
    if (position < 0 || position > NUM_LINES) return;

    FreetypeSetLine(position, line);
}

void WHBLogFreetypeScreenPrintBottom(const wchar_t *line) {
    DEBUG_THREADSAFE;
    uint32_t length = wcslen(line);

    if (length > LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }
    
    for (uint32_t i=(NUM_LINES-1)-bottomLines+1; i<NUM_LINES; i++) {
        wmemcpy(queueBuffer[i-1], queueBuffer[i], LINE_LENGTH);
    }

    wmemcpy(queueBuffer[NUM_LINES - 1], line, length);
    queueBuffer[NUM_LINES - 1][length] = L'\0';
    if (bottomLines < NUM_LINES-1) bottomLines++;
}

uint32_t WHBLogFreetypeScreenSize() {
    return NUM_LINES;
}

void WHBLogFreetypePrint(const wchar_t* line) {
    FreetypeAddLine(line);
}

void WHBLogFreetypeSetBackgroundColor(uint32_t color) {
    backgroundColor = color;
}

void WHBLogFreetypeSetFontColor(uint32_t color) {
    fontColor = color;
}

bool WHBLogFreetypeSetFontSize(uint8_t size) {
    fontFace.xScale = size;
    fontFace.yScale = size;

    SFT_LMetrics lmetrics;
    sft_lmetrics(&fontFace, &lmetrics);

    SFT_Glyph gid;
    SFT_GMetrics mtx;
    if (sft_lookup(&fontFace, L'>', &gid) == -1 || sft_gmetrics(&fontFace, gid, &mtx) == -1) {
        OSReport("[log_freetype] Failed to lookup the character for the selector list!\n");
        return true;
    }

    cursorSpaceWidth = (int32_t)mtx.advanceWidth;
    for (auto& glyph : glyphCache) {
        free(glyph.second.pixels);
    }
    glyphCache.clear();
    return false;
}

void WHBLogFreetypeFree() {
    sft_freefont(fontFace.font);
    fontFace.font = nullptr;
    fontFace = {};

    for (auto& glyph : glyphCache) {
        free(glyph.second.pixels);
    }

    if (freetypeHasForeground) {
        // OSScreenShutdown();
        FreetypeProcCallbackReleased(nullptr);
    }
}