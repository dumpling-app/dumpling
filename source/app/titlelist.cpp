#include "titlelist.h"
#include "titles.h"
#include "dumping.h"
#include "menu.h"
#include "navigation.h"
#include "gui.h"

#define MAX_LIST_SIZE (9)

struct listEntry {
    bool queued;
    std::reference_wrapper<titleEntry> titleEntryRef;
};

void showTitleList(const char* message, dumpingConfig config) {
    // Filter and create the list of titles that are displayed
    std::vector<listEntry> printTitles = {};
    for (auto& title : installedTitles) {
        // Check if the type is available
        if ((config.filterTypes & dumpTypeFlags::GAME) == dumpTypeFlags::GAME && !title.hasBase) continue;
        else if ((config.filterTypes & dumpTypeFlags::UPDATE) == dumpTypeFlags::UPDATE && !title.hasUpdate) continue;
        else if ((config.filterTypes & dumpTypeFlags::DLC) == dumpTypeFlags::DLC && !title.hasDLC) continue;
        else if ((config.filterTypes & dumpTypeFlags::SYSTEM_APP) == dumpTypeFlags::SYSTEM_APP && !title.hasBase) continue;
        else if ((config.filterTypes & (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE)) == (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE) && !title.hasBase) continue;

        // Differentiate between game and system app
        if ((config.filterTypes & dumpTypeFlags::GAME) == dumpTypeFlags::GAME && !isBase(title.base.type)) continue;
        if ((config.filterTypes & dumpTypeFlags::SYSTEM_APP) == dumpTypeFlags::SYSTEM_APP && !isSystemApp(title.base.type)) continue;

        // Prevent the disc copy from showing in the title lists
        if ((config.filterTypes & dumpTypeFlags::GAME) == dumpTypeFlags::GAME && title.base.location == titleLocation::Disc) continue;

        // Prevent a game that doesn't have save files from showing
        if ((config.filterTypes & (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE)) == (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE) && (title.commonSave.path.empty() && title.saves.empty())) continue;

        printTitles.emplace_back(listEntry{
            .queued = false,
            .titleEntryRef = std::ref(title)
        });
    }

    // Create a basic draw/input loop
    size_t selectedEntry = 0;
    size_t listOffset = 0;
    size_t listSize = MAX_LIST_SIZE;
    if (listSize > printTitles.size()) listSize = printTitles.size();

    bool startQueueDump = false;
    while(!startQueueDump) {
        // Print selection header
        WHBLogFreetypeStartScreen();
        WHBLogPrint(message);
        WHBLogPrint("===============================");

        // Print message when no title is found
        if (printTitles.empty()) {
            WHBLogPrint("Nothing was found!");
            WHBLogPrint("Make sure that everything is installed.");
        }

        // Print range of titles
        for (size_t i=listOffset; i<listOffset+listSize; i++) {
            WHBLogPrintf("%c %s %.30s", i == selectedEntry ? '>' : ' ', config.queue ? (printTitles[i].queued ? "[X]": "[ ]") : "", printTitles[i].titleEntryRef.get().shortTitle.c_str());
        }
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE045 Button = Start Dumping");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Title");
        WHBLogFreetypeScreenPrintBottom("\uE001 Button = Back to Main Menu");
        WHBLogFreetypeDrawScreen();

        // Loop until there's new input
        sleep_for(250ms);
        updateInputs();
        while(!startQueueDump) {
            updateInputs();

            if (navigatedUp()) {
                if (selectedEntry > 0) selectedEntry--;
                if (selectedEntry < listOffset) listOffset--;
                break;
            }
            if (navigatedDown()) {
                if (selectedEntry < printTitles.size()-1) selectedEntry++;
                if (listOffset+listSize == selectedEntry) listOffset++;
                break;
            }
            if (pressedOk()) {
                printTitles[selectedEntry].queued = !printTitles[selectedEntry].queued;
                if (!config.queue) startQueueDump = true;
                break;
            }
            if (pressedStart() && config.queue) {
                startQueueDump = true;
                break;
            }
            if (pressedBack()) {
                sleep_for(200ms);
                return;
            }

            sleep_for(50ms);
        }
    }

    // Create dumping queue
    std::vector<std::reference_wrapper<titleEntry>> queuedTitles = {};
    for (auto& title : printTitles) {
        if (title.queued) queuedTitles.emplace_back(title.titleEntryRef.get());
    }

    if (queuedTitles.empty()) {
        showDialogPrompt("Select at least one title to dump!", "Go Back");
        showTitleList(message, config);
        return;
    }

    // Show the option screen and give a last chance to stop the update
    if (!showOptionMenu(config, (config.dumpTypes & dumpTypeFlags::SAVE) == dumpTypeFlags::SAVE)) return;

    if (dumpQueue(queuedTitles, config)) showDialogPrompt("Dumping was successful!", "Continue to Main Menu");
}