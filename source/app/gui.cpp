#include "gui.h"

#define NUM_LINES (16)
#define LINE_LENGTH (128)
#define MEMORY_HEAP_TAG (0x0002B2B2)

static char textBuffer[NUM_LINES][LINE_LENGTH];
static uint32_t currLineNumber = 0;

static uint8_t* frameBufferTVFrontPtr = nullptr;
static uint8_t* frameBufferTVBackPtr = nullptr;
static uint32_t frameBufferTVSize = 0;
static uint8_t* frameBufferDRCFrontPtr = nullptr;
static uint8_t* frameBufferDRCBackPtr = nullptr;
static uint32_t frameBufferDRCSize = 0;

static uint32_t* currTVFrameBuffer = nullptr;
static uint32_t* currDRCFrameBuffer = nullptr;

static uint32_t backgroundColor = 0x0b5d5e00;

static bool usingHBL = false;

#define HBL_TITLE_ID (0x0005000013374842)
#define MII_MAKER_JPN_TITLE_ID (0x000500101004A000)
#define MII_MAKER_USA_TITLE_ID (0x000500101004A100)
#define MII_MAKER_EUR_TITLE_ID (0x000500101004A200)

void saveProcessCallback() {
    OSSavesDone_ReadyToRelease();
}

bool initializeGUI() {
    // Prepare rendering and framebuffers
    OSScreenInit();
    frameBufferTVSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    frameBufferDRCSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    OSScreenEnableEx(SCREEN_TV, TRUE);
    OSScreenEnableEx(SCREEN_DRC, TRUE);

    MEMHeapHandle framebufferHeap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    frameBufferTVFrontPtr = (uint8_t*)MEMAllocFromFrmHeapEx(framebufferHeap, frameBufferTVSize, 100);
    frameBufferDRCFrontPtr = (uint8_t*)MEMAllocFromFrmHeapEx(framebufferHeap, frameBufferDRCSize, 100);

    frameBufferTVBackPtr = frameBufferTVFrontPtr+(1280*720*4);
    frameBufferDRCBackPtr = frameBufferDRCFrontPtr+(896*480*4);

    currTVFrameBuffer = (uint32_t*)frameBufferTVFrontPtr;
    currDRCFrameBuffer = (uint32_t*)frameBufferDRCFrontPtr;
    OSScreenSetBufferEx(SCREEN_TV, currTVFrameBuffer);
    OSScreenSetBufferEx(SCREEN_DRC, currDRCFrameBuffer);

    WHBAddLogHandler(printLine);

    // Setup proc UI
    uint64_t titleId = OSGetTitleID();
    if (titleId == HBL_TITLE_ID || titleId == MII_MAKER_USA_TITLE_ID || titleId == MII_MAKER_EUR_TITLE_ID || titleId == MII_MAKER_JPN_TITLE_ID) {
        usingHBL = true;
    }

    OSEnableHomeButtonMenu(false);
    ProcUIInit(&saveProcessCallback);
    return true;
}

void shutdownGUI() {
    OSScreenShutdown();
    MEMHeapHandle framebufferHeap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    MEMFreeByStateToFrmHeap(framebufferHeap, MEMORY_HEAP_TAG);
}

static OSDynLoad_Module coreinitHandle = nullptr;
static int32_t (*OSShutdown)(int32_t status);

void exitApplication(bool shutdownOnExit) {
    // Initialize OSShutdown functions
    OSDynLoad_Acquire("coreinit.rpl", &coreinitHandle);
    OSDynLoad_FindExport(coreinitHandle, FALSE, "OSShutdown", (void **)&OSShutdown);
    OSDynLoad_Release(coreinitHandle);

    // Loop through ProcUI messages until it says Dumpling should exit
    ProcUIStatus status;
    while((status = ProcUIProcessMessages(true)) != PROCUI_STATUS_EXITING) {
        if (status == PROCUI_STATUS_RELEASE_FOREGROUND) {
            ProcUIDrawDoneRelease();
        }

        if (status != PROCUI_STATUS_IN_FOREGROUND) {
            continue;
        }

        if (shutdownOnExit) OSShutdown(1);
        else if (usingHBL) SYSRelaunchTitle(0, NULL);
        else SYSLaunchMenu();
    }
    ProcUIShutdown();
}

void WHBLogConsoleDraw() {
    OSScreenClearBufferEx(SCREEN_TV, backgroundColor);
    OSScreenClearBufferEx(SCREEN_DRC, backgroundColor);

    for (int32_t i=0; i<NUM_LINES; i++) {
        OSScreenPutFontEx(SCREEN_TV, 0, i, textBuffer[i]);
        OSScreenPutFontEx(SCREEN_DRC, 0, i, textBuffer[i]);
    }

    DCFlushRange(currTVFrameBuffer, frameBufferTVSize);
    DCFlushRange(currTVFrameBuffer, frameBufferDRCSize);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

void printLine(const char* message) {
    int32_t length = strlen(message);

    if (length > LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }

    if (currLineNumber == NUM_LINES) {
        for (int i=0; i<NUM_LINES-1; i++) {
            memcpy(textBuffer[i], textBuffer[i + 1], LINE_LENGTH);
        }
        memcpy(textBuffer[currLineNumber - 1], message, length);
        textBuffer[currLineNumber - 1][length] = 0;
    }
    else {
        memcpy(textBuffer[currLineNumber], message, length);
        textBuffer[currLineNumber][length] = 0;
        currLineNumber++;
    }
}

void setBackgroundColor(uint32_t color) {
    backgroundColor = color;
}