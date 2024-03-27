#include "menu.h"
#include "titles.h"
#include "users.h"
#include "navigation.h"
#include "titlelist.h"
#include "dumping.h"
#include "filesystem.h"
#include "gui.h"
#include "cfw.h"
#include "interfaces/fat32.h"
#include "interfaces/stub.h"

// Menu screens

void showLoadingScreen() {
    WHBLogFreetypeSetBackgroundColor(0x0b5d5e00);
    WHBLogFreetypeSetFontColor(0xFFFFFFFF);
    WHBLogFreetypeSetFontSize(22);
    WHBLogPrint("Dumpling V2.7.1");
    WHBLogPrint("-- Made by Crementif and Emiyl --");
    WHBLogPrint("");
    WHBLogFreetypeDraw();
}

#define OPTION(n) (selectedOption == (n) ? L'>' : L' ')

// Can get recursively called
void showMainMenu() {
    uint8_t selectedOption = 0;
    bool startSelectedOption = false;
    while(!startSelectedOption) {
        // Print menu text
        WHBLogFreetypeStartScreen();
        WHBLogFreetypePrint(L"Dumpling V2.7.1");
        WHBLogFreetypePrint(L"===============================");
        WHBLogFreetypePrintf(L"%C Dump a game disc", OPTION(0));
        WHBLogFreetypePrintf(L"%C Dump digital games", OPTION(1));
        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrintf(L"%C Dump files to use Cemu online", OPTION(2));
        WHBLogFreetypePrintf(L"%C Dump Wii U applications (e.g. Friend List, eShop etc.)", OPTION(3));
        // WHBLogFreetypePrintf("%C Dump Amiibo Files", OPTION(4));
        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrintf(L"%C Dump only Base files of a game", OPTION(4));
        WHBLogFreetypePrintf(L"%C Dump only Update files of a game", OPTION(5));
        WHBLogFreetypePrintf(L"%C Dump only DLC files of a game", OPTION(6));
        WHBLogFreetypePrintf(L"%C Dump only Save files of a game", OPTION(7));
        WHBLogFreetypePrintf(L"%C Dump whole MLC (everything stored on internal storage)", OPTION(8));
        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrintf(L"%C Dump all SpotPass data", OPTION(9));
        WHBLogFreetypeScreenPrintBottom(L"===============================");
        WHBLogFreetypeScreenPrintBottom(L"\uE000 Button = Select Option \uE001 Button = Exit Dumpling");
        WHBLogFreetypeScreenPrintBottom(L"");
        WHBLogFreetypeDrawScreen();

        // Loop until there's new input
        sleep_for(200ms); // Cooldown between each button press
        updateInputs();
        while(!startSelectedOption) {
            updateInputs();
            // Check each button state
            if (navigatedUp() && selectedOption > 0) {
                selectedOption--;
                break;
            }
            if (navigatedDown() && selectedOption < 9) {
                selectedOption++;
                break;
            }
            if (pressedOk()) {
                startSelectedOption = true;
                break;
            }
            if (pressedBack()) {
                uint8_t exitSelectedOption = showDialogPrompt(getCFWVersion() == MOCHA_FSCLIENT ? L"Do you really want to exit Dumpling?" : L"Do you really want to exit Dumpling?\nYour console will shutdown to prevent compatibility issues!", L"Yes", L"No");
                if (exitSelectedOption == 0) {
                    WHBLogFreetypeClear();
                    return;
                }
                else break;
            }
            sleep_for(50ms);
        }
    }

    // Go to the selected menu
    switch(selectedOption) {
        case 0:
            dumpDisc();
            break;
        case 1:
            showTitleList(L"Select all the games you want to dump!", {.filterTypes = DUMP_TYPE_FLAGS::GAME, .dumpTypes = (DUMP_TYPE_FLAGS::GAME | DUMP_TYPE_FLAGS::UPDATE | DUMP_TYPE_FLAGS::DLC | DUMP_TYPE_FLAGS::SAVES), .queue = true});
            break;
        case 2:
            dumpOnlineFiles();
            break;
        case 3:
            showTitleList(L"Select all the system applications you want to dump!", {.filterTypes = DUMP_TYPE_FLAGS::SYSTEM_APP, .dumpTypes = DUMP_TYPE_FLAGS::GAME, .queue = true});
            break;
        case 4:
            showTitleList(L"Select all the games that you want to dump the base game from!", {.filterTypes = DUMP_TYPE_FLAGS::GAME, .dumpTypes = DUMP_TYPE_FLAGS::GAME, .queue = true});
            break;
        case 5:
            showTitleList(L"Select all the games that you want to dump the update from!", {.filterTypes = DUMP_TYPE_FLAGS::UPDATE, .dumpTypes = DUMP_TYPE_FLAGS::UPDATE, .queue = true});
            break;
        case 6:
            showTitleList(L"Select all the games that you want to dump the DLC from!", {.filterTypes = DUMP_TYPE_FLAGS::DLC, .dumpTypes = DUMP_TYPE_FLAGS::DLC, .queue = true});
            break;
        case 7:
            showTitleList(L"Select all the games that you want to dump the save from!", {.filterTypes = DUMP_TYPE_FLAGS::SAVES, .dumpTypes = DUMP_TYPE_FLAGS::SAVES, .queue = true});
            break;
        case 8:
            dumpMLC();
            break;
        case 9:
            dumpSpotpass();
            break;
        case 10:
            break;
        default:
            break;
    }

    cleanDumpingProcess();
    sleep_for(500ms);
    showMainMenu();
}

bool alreadyDumpedDefaultAccount = false;
bool showOptionMenu(dumpingConfig& config, bool showAccountOption) {
    uint8_t selectedOption = 0;
    uint8_t selectedAccount = 0;
    uint8_t selectedDrive = 0;

    // Detect when multiple online files are getting dumped
    if (showAccountOption && HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::CUSTOM) && alreadyDumpedDefaultAccount) {
        config.dumpAsDefaultUser = false;
    }

    // This is slightly hacky
    const bool dumpingOnlineFiles = showAccountOption && HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::CUSTOM);

    sleep_for(1s);
    while(true) {
        auto drives = !USE_WUT_DEVOPTAB() ? Fat32Transfer::getDrives() : StubTransfer::getDrives();
        if ((drives.size()-1) < selectedDrive) selectedDrive = drives.size()-1;

        WHBLogFreetypeStartScreen();
        WHBLogFreetypePrint(L"Change any options for this dump:");
        WHBLogFreetypePrint(L"===============================");
        WHBLogFreetypePrintf(L"%C Dump Destination: %S", OPTION(0), drives.empty() ? L"No Drives Detected" : toWstring(drives[selectedDrive].second).c_str());
        WHBLogFreetypePrintf(L"%C Do Initial Scan For Required Empty Space: %S", OPTION(1), config.scanTitleSize ? L"Yes" : L"No");
        if (showAccountOption && dumpingOnlineFiles) WHBLogFreetypePrintf(L"%C Online Account: %S (NNID: %s)", OPTION(2), allUsers[selectedAccount].miiName.c_str(), allUsers[selectedAccount].networkAccount ? allUsers[selectedAccount].accountId.c_str() : "<NO NNID LINKED>");
        if (showAccountOption && !dumpingOnlineFiles) WHBLogFreetypePrintf(L"%C Account To Get Saves From: %S", OPTION(2), allUsers[selectedAccount].miiName.c_str());
        if (showAccountOption && dumpingOnlineFiles) WHBLogFreetypePrintf(L"%C Merge Account To Default Cemu User: %S", OPTION(3), config.dumpAsDefaultUser ? L"Yes" : L"No");
        if (showAccountOption && !dumpingOnlineFiles) WHBLogFreetypePrintf(L"%C Merge Saves To Default Cemu User: %S", OPTION(3), config.dumpAsDefaultUser ? L"Yes" : L"No");
        if (USE_DEBUG_STUBS == 1) WHBLogFreetypePrintf(L"%C Change Fat32 Cache Size (DEBUG): %u", OPTION(4), config.debugCacheSize);
        else WHBLogFreetypePrint(L"");
        WHBLogFreetypePrintf(L"%C [Confirm]", OPTION(5));
        WHBLogFreetypePrintf(L"%C [Cancel]", OPTION(6));
        if (selectedOption <= 4) {
            WHBLogFreetypePrint(L"===============================");
        }
        if (selectedOption == 0) {
            WHBLogFreetypePrint(L"Select an fat32/exfat device to write the files too.");
            WHBLogFreetypePrint(L"Press \uE000 to refresh list. If your device doesn't show up:");
            WHBLogFreetypePrint(L" - Reinsert the SD card/USB drive and double-check the lock switch");
            WHBLogFreetypePrint(L" - Make sure its formatted as fat32/exfat drive");
            WHBLogFreetypePrint(L" - It doesn't have multiple (hidden) partitions");
            WHBLogFreetypePrint(L" - Try a different SD card (recommended) or USB drive");
        }
        else if (selectedOption == 1) {
            WHBLogFreetypePrint(L"Doing an initial scan allows you to:");
            WHBLogFreetypePrint(L" - Determine space required on SD/USB destination");
            WHBLogFreetypePrint(L" - Show overall progress while dumping");
            WHBLogFreetypePrint(L" - Show rough time estimate while dumping");
            WHBLogFreetypePrint(L"This takes a few minutes (depending on size) extra.");
        }
        else if (selectedOption == 2 && dumpingOnlineFiles) {
            WHBLogFreetypePrint(L"Select the account you want to dump the online files for.");
            if (!allUsers[selectedAccount].networkAccount) {
                WHBLogFreetypePrint(L"This account doesn't have a NNID connected!");
                WHBLogFreetypePrint(L" - Click the Mii icon on the Wii U's homescreen");
                WHBLogFreetypePrint(L" - Click on Link a Nintendo Network ID option");
                WHBLogFreetypePrint(L" - Enable the Save Password option");
                WHBLogFreetypePrint(L" - Return to Dumpling");
            }
            else if (!allUsers[selectedAccount].passwordCached) {
                WHBLogFreetypePrint(L"This account doesn't have it's password saved!");
                WHBLogFreetypePrint(L"This is required to use Cemu online!");
                WHBLogFreetypePrint(L" - Click the Mii icon on the Wii U's homescreen");
                WHBLogFreetypePrint(L" - Enable the Save Password option");
                WHBLogFreetypePrint(L" - Return to Dumpling");
            }
            else {
                WHBLogFreetypePrint(L"This account is ready for Cemu's online functionality:");
                WHBLogFreetypePrint(L" - Selected account has NNID connected to it!");
                WHBLogFreetypePrint(L" - Selected account has its password saved!");
            }
        }
        else if (selectedOption == 2 && !dumpingOnlineFiles) {
            WHBLogFreetypePrint(L"Select the Wii U account you want to dump the saves from.");
            WHBLogFreetypePrint(L"If you don't care about the save files, then just any ");
        }
        else if (selectedOption == 3 && dumpingOnlineFiles) {
            WHBLogFreetypePrint(L"Enabling this changes the dumped account files to");
            WHBLogFreetypePrint(L"replace the existing Cemu account instead of being");
            WHBLogFreetypePrint(L"an additional account.");
            WHBLogFreetypePrint(L"Your existing saves, assuming you use Cemu's default");
            WHBLogFreetypePrint(L"account, won't need to be transferred that way.");
        }
        else if (selectedOption == 3 && !dumpingOnlineFiles) {
            WHBLogFreetypePrint(L"Enabling this changes the dumped saves to replace existing saves");
            WHBLogFreetypePrint(L"from the default Cemu account instead of being dumped");
            WHBLogFreetypePrint(L"for your original Wii U account. This is recommended since");
            WHBLogFreetypePrint(L"otherwise you have to dump/create a Cemu account that matches");
            WHBLogFreetypePrint(L"the ID of your Wii U account to use these save files.");
        }
        else if (selectedOption == 4) {
            WHBLogFreetypePrint(L"This allows you to change the amount of fat32 sectors are kept");
            WHBLogFreetypePrint(L"in memory before they are actually written. Used for trial and");
            WHBLogFreetypePrint(L"erroring a good value for all games, since some options might");
            WHBLogFreetypePrint(L"favor games with a few very large files but be detrimental");
            WHBLogFreetypePrint(L"for small files. Recommended Values: 32,64,128,256 etc.");
        }
        WHBLogFreetypeScreenPrintBottom(L"===============================");
        WHBLogFreetypeScreenPrintBottom(L"\uE000 Button = Select Option \uE001 Button = Cancel");
        WHBLogFreetypeDrawScreen();

        sleep_for(100ms);
        updateInputs();
        while(true) {
            updateInputs();
            if (navigatedUp() && selectedOption > 0) {
                selectedOption--;
                if (USE_DEBUG_STUBS == 0 && selectedOption == 4) selectedOption--;
                while ((selectedOption == 2 || selectedOption == 3) && !showAccountOption) selectedOption--;
                break;
            }
            if (navigatedDown() && selectedOption < 6) {
                selectedOption++;
                while ((selectedOption == 2 || selectedOption == 3) && !showAccountOption) selectedOption++;
                if (USE_DEBUG_STUBS == 0 && selectedOption == 4) selectedOption++;
                break;
            }
            if (navigatedLeft()) {
                if (selectedOption == 0 && selectedDrive > 0) selectedDrive--;
                if (selectedOption == 1) config.scanTitleSize = !config.scanTitleSize;
                if (selectedOption == 2 && selectedAccount > 0) selectedAccount--;
                if (selectedOption == 3) config.dumpAsDefaultUser = !config.dumpAsDefaultUser;
                if (selectedOption == 4 && config.debugCacheSize > 256) config.debugCacheSize -= 256;
                break;
            }
            if (navigatedRight()) {
                if (selectedOption == 0 && selectedDrive < drives.size()-1) selectedDrive++;
                if (selectedOption == 1) config.scanTitleSize = !config.scanTitleSize;
                if (selectedOption == 2 && selectedAccount < allUsers.size()-1) selectedAccount++;
                if (selectedOption == 3) config.dumpAsDefaultUser = !config.dumpAsDefaultUser;
                if (selectedOption == 4 && config.debugCacheSize < (256*512)) config.debugCacheSize += 256;
                break;
            }
            if (pressedOk() && selectedOption == 0) {
                sleep_for(500ms);
                break; // Refreshes screen with drive list
            }
            const bool validAccount = !dumpingOnlineFiles || true || (dumpingOnlineFiles && allUsers[selectedAccount].networkAccount && allUsers[selectedAccount].passwordCached);
            if (!drives.empty() & validAccount && (pressedStart() || (pressedOk() && selectedOption == 5))) {
                if (dumpingOnlineFiles && config.dumpAsDefaultUser) {
                    alreadyDumpedDefaultAccount = true;
                }
                config.accountId = allUsers[selectedAccount].persistentId;
                config.dumpTarget = drives[selectedDrive].first;
                return true;
            }
            if (pressedBack() || (pressedOk() && selectedOption == 6)) {
                return false;
            }
            sleep_for(50ms);
        }
    }
}


// Helper functions

uint8_t showDialogPrompt(const wchar_t* message, const wchar_t* button1, const wchar_t* button2) {
    sleep_for(100ms);
    uint8_t selectedOption = 0;
    while(true) {
        WHBLogFreetypeStartScreen();

        // Print each line
        std::wistringstream messageStream(message);
        std::wstring line;

        while(std::getline(messageStream, line)) {
            WHBLogFreetypePrint(line.c_str());
        }

        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrintf(L"%C [%S]", OPTION(0), button1);
        if (button2 != nullptr) WHBLogFreetypePrintf(L"%C [%S]", OPTION(1), button2);
        WHBLogFreetypePrint(L"");
        WHBLogFreetypeScreenPrintBottom(L"===============================");
        WHBLogFreetypeScreenPrintBottom(L"\uE000 Button = Select Option");
        WHBLogFreetypeDrawScreen();

        // Input loop
        sleep_for(400ms);
        updateInputs();
        while (true) {
            updateInputs();
            // Handle navigation between the buttons
            if (button2 != nullptr) {
                if (navigatedUp() && selectedOption == 1) {
                    selectedOption = 0;
                    break;
                }
                else if (navigatedDown() && selectedOption == 0) {
                    selectedOption = 1;
                    break;
                }
            }

            if (pressedOk()) {
                return selectedOption;
            }

            sleep_for(50ms);
        }
    }
}

void showDialogPrompt(const wchar_t* message, const wchar_t* button) {
    showDialogPrompt(message, button, nullptr);
}

const wchar_t* errorMessage = nullptr;
void setErrorPrompt(const wchar_t* message) {
    errorMessage = message;
}

std::wstring messageCopy;
void setErrorPrompt(std::wstring message) {
    messageCopy = std::move(message);
    setErrorPrompt(messageCopy.c_str());
}

void showErrorPrompt(const wchar_t* button) {
    std::wstring promptMessage(L"An error occurred:\n");
    if (errorMessage) promptMessage += errorMessage;
    else promptMessage += L"No error was specified!";
    showDialogPrompt(promptMessage.c_str(), button);
}