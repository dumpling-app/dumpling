#include "gui.h"
#include "../font/log_freetype.h"

#define NUM_LINES (16)
#define LINE_LENGTH (128)

static bool usingHBL = false;

#define HBL_TITLE_ID (0x0005000013374842)
#define MII_MAKER_JPN_TITLE_ID (0x000500101004A000)
#define MII_MAKER_USA_TITLE_ID (0x000500101004A100)
#define MII_MAKER_EUR_TITLE_ID (0x000500101004A200)

void saveProcessCallback() {
    OSSavesDone_ReadyToRelease();
}

bool initializeGUI() {
    // Setup proc UI
    uint64_t titleId = OSGetTitleID();
    if (titleId == HBL_TITLE_ID || titleId == MII_MAKER_USA_TITLE_ID || titleId == MII_MAKER_EUR_TITLE_ID || titleId == MII_MAKER_JPN_TITLE_ID) {
        usingHBL = true;
    }

    OSEnableHomeButtonMenu(false);
    ProcUIInit(&saveProcessCallback);

    WHBLogFreetypeInit();
    return true;
}

void shutdownGUI() {
    WHBLogFreetypeFree();
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