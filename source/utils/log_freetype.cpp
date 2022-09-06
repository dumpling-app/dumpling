#include "log_freetype.h"
#include "../app/stub_asan.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define CONSOLE_FRAME_HEAP_TAG (0x0002B2B)
#define NUM_LINES (18)
#define LINE_LENGTH (128)

char queueBuffer[NUM_LINES][LINE_LENGTH];
uint32_t newLines = 0;
uint32_t bottomLines = 0;

bool freetypeHasForeground = false;

uint8_t* frameBufferTVFrontPtr = NULL;
uint8_t* frameBufferTVBackPtr = NULL;
uint32_t frameBufferTVSize = 0;
uint8_t* frameBufferDRCFrontPtr = NULL;
uint8_t* frameBufferDRCBackPtr = NULL;
uint32_t frameBufferDRCSize = 0;
uint8_t* currTVFrameBuffer = NULL;
uint8_t* currDRCFrameBuffer = NULL;

uint32_t fontColor = 0xFFFFFFFF;
uint32_t backgroundColor = 0x0B5D5E00;
FT_Library fontLibrary;
FT_Face fontFace;
uint8_t* fontBuffer;
FT_Pos cursorSpaceWidth = 0;

#define ENABLE_THREADSAFE FALSE
#if ENABLE_THREADSAFE
std::mutex _mutex;
#define DEBUG_THREADSAFE std::scoped_lock<std::mutex> lck(_mutex);
#else
#define DEBUG_THREADSAFE do {} while(0)
#endif

static void FreetypeSetLine(uint32_t position, const char *line) {
    DEBUG_THREADSAFE;
    uint32_t length = strlen(line);
    memcpy(queueBuffer[newLines - 1], line, length);
    queueBuffer[newLines - 1][length] = '\0';
}

static void FreetypeAddLine(const char *line) {
    DEBUG_THREADSAFE;
    uint32_t length = strlen(line);

    if (length > LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }

    if (newLines == NUM_LINES) {
        for (uint32_t i=0; i<NUM_LINES-1; i++) {
            memcpy(queueBuffer[i], queueBuffer[i + 1], LINE_LENGTH);
        }

        memcpy(queueBuffer[newLines - 1], line, length);
        queueBuffer[newLines - 1][length] = '\0';
    }
    else {
        memcpy(queueBuffer[newLines], line, length);
        queueBuffer[newLines][length] = '\0';
        newLines++;
    }
}

void drawPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t opacity = a;
    uint8_t backgroundOpacity = (255-opacity);
    {
        uint32_t width = 1280;
        uint32_t v = (x + y * width) * 4;
        currTVFrameBuffer[v + 0] = (r * opacity + (backgroundOpacity * currTVFrameBuffer[v + 0]))/255;
        currTVFrameBuffer[v + 1] = (g * opacity + (backgroundOpacity * currTVFrameBuffer[v + 1]))/255;
        currTVFrameBuffer[v + 2] = (b * opacity + (backgroundOpacity * currTVFrameBuffer[v + 2]))/255;
        currTVFrameBuffer[v + 3] = a;
    }

    {
        uint32_t width = 896;
        uint32_t v = (x + y * width) * 4;
        currDRCFrameBuffer[v + 0] = (r * opacity + (backgroundOpacity * currDRCFrameBuffer[v + 0]))/255;
        currDRCFrameBuffer[v + 1] = (g * opacity + (backgroundOpacity * currDRCFrameBuffer[v + 1]))/255;
        currDRCFrameBuffer[v + 2] = (b * opacity + (backgroundOpacity * currDRCFrameBuffer[v + 2]))/255;
        currDRCFrameBuffer[v + 3] = a;
    }
}

void drawBitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y) {
    uint8_t r = (fontColor >> 16) & 0xff;
    uint8_t g = (fontColor >> 8) & 0xff;
    uint8_t b = (fontColor >> 0) & 0xff;
    uint8_t a = 0xFF;

    FT_Int y_max = y + bitmap->rows;

    switch (bitmap->pixel_mode) {
        case FT_PIXEL_MODE_GRAY: {
            FT_Int x_max = x + bitmap->width;
            for (FT_Int i=x, p=0; i < x_max; i++, p++) {
                for (FT_Int j=y, q=0; j < y_max; j++, q++) {
                    if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;

                    uint8_t col = bitmap->buffer[q * bitmap->pitch + p];
                    if (col == 0) continue;

                    uint8_t pixelOpacity = col;
                    drawPixel(i, j, r, g, b, (a+pixelOpacity)/2);
                }
            }
            break;
        }
        case FT_PIXEL_MODE_LCD: {
            FT_Int x_max = x + bitmap->width / 3;
            for (FT_Int i=x, p=0; i < x_max; i++, p++) {
                for (FT_Int j=y, q=0; j < y_max; j++, q++) {
                    if (i < 0 || j < 0 || i >= 854 || j >= 480)
                        continue;
                    uint8_t cr = bitmap->buffer[q * bitmap->pitch + p * 3 + 0];
                    uint8_t cg = bitmap->buffer[q * bitmap->pitch + p * 3 + 1];
                    uint8_t cb = bitmap->buffer[q * bitmap->pitch + p * 3 + 2];

                    if ((cr | cg | cb) == 0)
                        continue;
                    drawPixel(i, j, cr, cg, cb, 255);
                }
            }
            break;
        }
    }
}

int32_t renderLine(int32_t x, int32_t y, char *string, bool wrap) {
    FT_GlyphSlot slot = fontFace->glyph;
    int32_t pen_x = x;
    int32_t pen_y = y;
    FT_UInt previous_glyph = 0;

    while (*string) {
        uint32_t buf = *string++;

        if ((buf >> 6) == 3) {
            uint8_t b1 = buf & 0xFF;
            uint8_t b2 = *string++;
            uint8_t b3 = *string++;
            if ((buf & 0xF0) == 0xC0) {
                if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
                buf = ((b1 & 0xF) << 6) | b2;
            }
            else if ((buf & 0xF0) == 0xD0) {
                if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
                buf = 0x400 | ((b1 & 0xF) << 6) | b2;
            }
            else if ((buf & 0xF0) == 0xE0) {
                if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
                if ((b3 & 0xC0) == 0x80) b3 &= 0x3F;
                buf = ((b1 & 0xF) << 12) | (b2 << 6) | b3;
            }
        }
        else if (buf & 0x80) {
            string++;
            continue;
        }

        if (buf == '\n') {
            pen_y += (fontFace->size->metrics.height >> 6);
            pen_x = x;
            continue;
        }

        FT_UInt glyph_index = FT_Get_Char_Index(fontFace, buf);

        if (FT_HAS_KERNING(fontFace)) {
            FT_Vector vector;
            FT_Get_Kerning(fontFace, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &vector);
            pen_x += (vector.x >> 6);
        }

        if (FT_Load_Glyph(fontFace, glyph_index, FT_LOAD_DEFAULT)) continue;

        if (FT_Render_Glyph(fontFace->glyph, FT_RENDER_MODE_NORMAL)) continue;

        if ((pen_x + (slot->advance.x >> 6)) > 853) {
            if (wrap) {
                pen_y += (fontFace->size->metrics.height >> 6);
                pen_x = x;
            }
            else {
                return pen_x;
            }
        }

        drawBitmap(&slot->bitmap, pen_x + slot->bitmap_left, (fontFace->height >> 6) + pen_y - slot->bitmap_top);

        if (x == pen_x && buf == ' ' && cursorSpaceWidth != 0) {
            pen_x += (cursorSpaceWidth >> 6);
        }
        else {
            pen_x += (slot->advance.x >> 6);
        }
        previous_glyph = glyph_index;
    }
    return pen_x;
}

void WHBLogFreetypeDraw() {
    if (!freetypeHasForeground) return;

    OSScreenClearBufferEx(SCREEN_TV, backgroundColor);
    OSScreenClearBufferEx(SCREEN_DRC, backgroundColor);
    const int32_t x = 0;

    for (int32_t y=0; y<NUM_LINES; y++) {
        renderLine((x+4)*12, (y+1)*24, queueBuffer[y], false);
    }

    DCFlushRange(currTVFrameBuffer, frameBufferTVSize);
    DCFlushRange(currDRCFrameBuffer, frameBufferDRCSize);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    currTVFrameBuffer = (currTVFrameBuffer == frameBufferTVFrontPtr) ? frameBufferTVBackPtr : frameBufferTVFrontPtr;
    currDRCFrameBuffer = (currDRCFrameBuffer == frameBufferDRCFrontPtr) ? frameBufferDRCBackPtr : frameBufferDRCFrontPtr;
}

static uint32_t FreetypeProcCallbackAcquired(void *context) {
    if (freetypeHasForeground) return 0;
    freetypeHasForeground = true;

    if (frameBufferTVSize) {
        frameBufferTVFrontPtr = (uint8_t*)MEM2_alloc(frameBufferTVSize, 0x100);
        frameBufferTVBackPtr = (uint8_t*)frameBufferTVFrontPtr + (1*(1280*720*4));
    }

    if (frameBufferDRCSize) {
        frameBufferDRCFrontPtr = (uint8_t*)MEM2_alloc(frameBufferDRCSize, 0x100);
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
    if (!freetypeHasForeground) return 0;
    freetypeHasForeground = false;
    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    MEMFreeByStateToFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);
    currTVFrameBuffer = NULL;
    frameBufferTVFrontPtr = NULL;
    frameBufferTVBackPtr = NULL;
    currDRCFrameBuffer = NULL;
    frameBufferDRCFrontPtr = NULL;
    frameBufferDRCBackPtr = NULL;
    return 0;
}

bool WHBLogFreetypeInit() {
    // Initialize screen
    OSScreenInit();
    frameBufferTVSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    frameBufferDRCSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    FreetypeProcCallbackAcquired(NULL);
    OSScreenEnableEx(SCREEN_TV, TRUE);
    OSScreenEnableEx(SCREEN_DRC, TRUE);

    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, FreetypeProcCallbackAcquired, NULL, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, FreetypeProcCallbackReleased, NULL, 100);

    // Initialize freetype2
    FT_Error result;
    if ((result = FT_Init_FreeType(&fontLibrary)) != 0) {
        return true;
    }

    uint32_t fontSize;
    OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, (void**)&fontBuffer, &fontSize);

    if ((result = FT_New_Memory_Face(fontLibrary, fontBuffer, fontSize, 0, &fontFace)) != 0) {
        return true;
    }
    if ((result = FT_Select_Charmap(fontFace, FT_ENCODING_UNICODE)) != 0) {
        return true;
    }
    if (WHBLogFreetypeSetFontSize(22, 0)) {
        return true;
    }

    WHBAddLogHandler(FreetypeAddLine);
    return false;
}

void WHBLogFreetypeClear() {
    DEBUG_THREADSAFE;
    newLines = 0;
    bottomLines = 0;
    memset(queueBuffer, '\0', sizeof(queueBuffer));
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

void WHBLogFreetypePrintfAtPosition(uint32_t position, const char *fmt, ...) {
    DEBUG_THREADSAFE;
    if (position < 0 || position > NUM_LINES) return;

    va_list va;
    va_start(va, fmt);
    vsnprintf(queueBuffer[position], LINE_LENGTH, fmt, va);
    va_end(va);
}

void WHBLogFreetypePrintf(const char *fmt, ...) {
    char formattedLine[LINE_LENGTH];
    
    va_list va;
    va_start(va, fmt);
    vsnprintf(formattedLine, LINE_LENGTH, fmt, va);
    va_end(va);
    FreetypeAddLine(formattedLine);
}

void WHBLogFreetypePrintAtPosition(uint32_t position, const char *line) {
    if (position < 0 || position > NUM_LINES) return;

    FreetypeSetLine(position, line);
}

void WHBLogFreetypeScreenPrintBottom(const char *line) {
    DEBUG_THREADSAFE;
    uint32_t length = strlen(line);

    if (length > LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }
    
    for (uint32_t i=(NUM_LINES-1)-bottomLines+1; i<NUM_LINES; i++) {
        memcpy(queueBuffer[i-1], queueBuffer[i], LINE_LENGTH);
    }

    memcpy(queueBuffer[NUM_LINES - 1], line, length);
    queueBuffer[NUM_LINES - 1][length] = '\0';
    if (bottomLines < NUM_LINES-1) bottomLines++;
}

uint32_t WHBLogFreetypeScreenSize() {
    return NUM_LINES;
}

void WHBLogFreetypePrint(const char* line) {
    FreetypeAddLine(line);
}

void WHBLogFreetypeSetBackgroundColor(uint32_t color) {
    backgroundColor = color;
}

void WHBLogFreetypeSetFontColor(uint32_t color) {
    fontColor = color;
}

bool WHBLogFreetypeSetFontSize(uint8_t width, uint8_t height) {
    if (FT_Set_Pixel_Sizes(fontFace, width, height) != 0) return true;

    FT_UInt glyph_index = FT_Get_Char_Index(fontFace, '>');

    if (FT_Load_Glyph(fontFace, glyph_index, FT_LOAD_DEFAULT)) return true;
    if (FT_Render_Glyph(fontFace->glyph, FT_RENDER_MODE_NORMAL)) return true;
    cursorSpaceWidth = fontFace->glyph->advance.x;
    return false;
}

void WHBLogFreetypeFree() {
    FT_Done_Face(fontFace);
    FT_Done_FreeType(fontLibrary);

    if (freetypeHasForeground) {
        OSScreenShutdown();
        FreetypeProcCallbackReleased(NULL);
    }
}