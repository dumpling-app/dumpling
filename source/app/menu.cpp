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
    WHBLogFreetypeSetFontSize(22, 22);
    WHBLogPrint("Dumpling V2.6.1");
    WHBLogPrint("-- Made by Crementif and Emiyl --");
    WHBLogPrint("");
    WHBLogFreetypeDraw();
}

#define OPTION(n) (selectedOption == (n) ? '>' : ' ')

// Can get recursively called
void showMainMenu() {
    uint8_t selectedOption = 0;
    bool startSelectedOption = false;
    while(!startSelectedOption) {
        // Print menu text
        WHBLogFreetypeStartScreen();
        WHBLogFreetypePrint("Dumpling V2.6.1");
        WHBLogFreetypePrint("===============================");
        WHBLogFreetypePrintf("%c Dump a game disc", OPTION(0));
        WHBLogFreetypePrintf("%c Dump digital games", OPTION(1));
        WHBLogFreetypePrint("");
        WHBLogFreetypePrintf("%c Dump files to use Cemu online", OPTION(2));
        WHBLogFreetypePrintf("%c Dump Wii U applications (e.g. Friend List, eShop etc.)", OPTION(3));
        // WHBLogFreetypePrintf("%c Dump Amiibo Files", selectedOption==4 ? '>' : ' ');
        WHBLogFreetypePrintf("");
        WHBLogFreetypePrintf("%c Dump only Base files of a game", OPTION(4));
        WHBLogFreetypePrintf("%c Dump only Update files of a game", OPTION(5));
        WHBLogFreetypePrintf("%c Dump only DLC files of a game", OPTION(6));
        WHBLogFreetypePrintf("%c Dump only Save files of a game", OPTION(7));
        WHBLogFreetypePrintf("%c Dump whole MLC (everything stored on internal storage)", OPTION(8));
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Option \uE001 Button = Exit Dumpling");
        WHBLogFreetypeScreenPrintBottom("");
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
            if (navigatedDown() && selectedOption < 8) {
                selectedOption++;
                break;
            }
            if (pressedOk()) {
                startSelectedOption = true;
                break;
            }
            if (pressedBack()) {
                uint8_t exitSelectedOption = showDialogPrompt(getCFWVersion() == MOCHA_FSCLIENT ? "Do you really want to exit Dumpling?" : "Do you really want to exit Dumpling?\nYour console will shutdown to prevent compatibility issues!", "Yes", "No");
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
            showTitleList("Select all the games you want to dump!", {.filterTypes = DUMP_TYPE_FLAGS::GAME, .dumpTypes = (DUMP_TYPE_FLAGS::GAME | DUMP_TYPE_FLAGS::UPDATE | DUMP_TYPE_FLAGS::DLC | DUMP_TYPE_FLAGS::SAVES), .queue = true});
            break;
        case 2:
            dumpOnlineFiles();
            break;
        case 3:
            showTitleList("Select all the system applications you want to dump!", {.filterTypes = DUMP_TYPE_FLAGS::SYSTEM_APP, .dumpTypes = DUMP_TYPE_FLAGS::GAME, .queue = true});
            break;
        case 4:
            showTitleList("Select all the games that you want to dump the base game from!", {.filterTypes = DUMP_TYPE_FLAGS::GAME, .dumpTypes = DUMP_TYPE_FLAGS::GAME, .queue = true});
            break;
        case 5:
            showTitleList("Select all the games that you want to dump the update from!", {.filterTypes = DUMP_TYPE_FLAGS::UPDATE, .dumpTypes = DUMP_TYPE_FLAGS::UPDATE, .queue = true});
            break;
        case 6:
            showTitleList("Select all the games that you want to dump the DLC from!", {.filterTypes = DUMP_TYPE_FLAGS::DLC, .dumpTypes = DUMP_TYPE_FLAGS::DLC, .queue = true});
            break;
        case 7:
            showTitleList("Select all the games that you want to dump the save from!", {.filterTypes = DUMP_TYPE_FLAGS::SAVES, .dumpTypes = DUMP_TYPE_FLAGS::SAVES, .queue = true});
            break;
        case 8:
            dumpMLC();
            break;
        case 9:
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
        WHBLogFreetypePrint("Change any options for this dump:");
        WHBLogFreetypePrint("===============================");
        WHBLogFreetypePrintf("%c Dump Destination: %s", OPTION(0), drives.empty() ? "No Drives Detected" : drives[selectedDrive].second.c_str());
        WHBLogFreetypePrintf("%c Do Initial Scan For Required Empty Space: %s", OPTION(1), config.scanTitleSize ? "Yes" : "No");
        if (showAccountOption && dumpingOnlineFiles) WHBLogFreetypePrintf("%c Online Account: %s", OPTION(2), allUsers[selectedAccount].miiName.c_str());
        if (showAccountOption && !dumpingOnlineFiles) WHBLogFreetypePrintf("%c Account To Get Saves From: %s", OPTION(2), allUsers[selectedAccount].miiName.c_str());
        if (showAccountOption && dumpingOnlineFiles) WHBLogFreetypePrintf("%c Merge Account To Default Cemu User: %s", OPTION(3), config.dumpAsDefaultUser ? "Yes" : "No");
        if (showAccountOption && !dumpingOnlineFiles) WHBLogFreetypePrintf("%c Merge Saves To Default Cemu User: %s", OPTION(3), config.dumpAsDefaultUser ? "Yes" : "No");
        if (USE_DEBUG_STUBS == 1) WHBLogFreetypePrintf("%c Change Fat32 Cache Size (DEBUG): %u", OPTION(4), config.debugCacheSize);
        else WHBLogFreetypePrint("");
        WHBLogFreetypePrintf("%c [Confirm]", OPTION(5));
        WHBLogFreetypePrintf("%c [Cancel]", OPTION(6));
        if (selectedOption <= 4) {
            WHBLogFreetypePrint("===============================");
        }
        if (selectedOption == 0) {
            WHBLogFreetypePrint("Select an fat32/exfat device to write the files too.");
            WHBLogFreetypePrint("Press \uE000 to refresh list. If your device doesn't show up:");
            WHBLogFreetypePrint(" - Reinsert the SD card/USB drive and double-check the lock switch");
            WHBLogFreetypePrint(" - Make sure its formatted as fat32/exfat drive");
            WHBLogFreetypePrint(" - It doesn't have multiple (hidden) partitions");
            WHBLogFreetypePrint(" - Try a different SD card (recommended) or USB drive");
        }
        else if (selectedOption == 1) {
            WHBLogFreetypePrint("Doing an initial scan allows you to:");
            WHBLogFreetypePrint(" - Determine space required on SD/USB destination");
            WHBLogFreetypePrint(" - Show overall progress while dumping");
            WHBLogFreetypePrint(" - Show rough time estimate while dumping");
            WHBLogFreetypePrint("This takes a few minutes (depending on size) extra.");
        }
        else if (selectedOption == 2 && dumpingOnlineFiles) {
            WHBLogFreetypePrint("Select the account you want to dump the online files for.");
            if (!allUsers[selectedAccount].networkAccount) {
                WHBLogFreetypePrint("This account doesn't have a NNID connected!");
                WHBLogFreetypePrint(" - Click the Mii icon on the Wii U's homescreen");
                WHBLogFreetypePrint(" - Click on Link a Nintendo Network ID option");
                WHBLogFreetypePrint(" - Enable the Save Password option");
                WHBLogFreetypePrint(" - Return to Dumpling");
            }
            else if (!allUsers[selectedAccount].passwordCached) {
                WHBLogFreetypePrint("This account doesn't have it's password saved!");
                WHBLogFreetypePrint("This is required to use Cemu online!");
                WHBLogFreetypePrint(" - Click the Mii icon on the Wii U's homescreen");
                WHBLogFreetypePrint(" - Enable the Save Password option");
                WHBLogFreetypePrint(" - Return to Dumpling");
            }
            else {
                WHBLogFreetypePrint("This account is ready for Cemu's online functionality:");
                WHBLogFreetypePrint(" - Selected account has NNID connected to it!");
                WHBLogFreetypePrint(" - Selected account has its password saved!");
            }
        }
        else if (selectedOption == 2 && !dumpingOnlineFiles) {
            WHBLogFreetypePrint("Select the Wii U account you want to dump the saves from.");
            WHBLogFreetypePrint("If you don't care about the save files, then just any ");
        }
        else if (selectedOption == 3 && dumpingOnlineFiles) {
            WHBLogFreetypePrint("Enabling this changes the dumped account files to");
            WHBLogFreetypePrint("replace the existing Cemu account instead of being");
            WHBLogFreetypePrint("an additional account.");
            WHBLogFreetypePrint("Your existing saves, assuming you use Cemu's default");
            WHBLogFreetypePrint("account, won't need to be transferred that way.");
        }
        else if (selectedOption == 3 && !dumpingOnlineFiles) {
            WHBLogFreetypePrint("Enabling this changes the dumped saves to replace existing saves");
            WHBLogFreetypePrint("from the default Cemu account instead of being dumped");
            WHBLogFreetypePrint("for your original Wii U account. This is recommended since");
            WHBLogFreetypePrint("otherwise you have to dump/create a Cemu account that matches");
            WHBLogFreetypePrint("the ID of your Wii U account to use these save files.");
        }
        else if (selectedOption == 4) {
            WHBLogFreetypePrint("This allows you to change the amount of fat32 sectors are kept");
            WHBLogFreetypePrint("in memory before they are actually written. Used for trial and");
            WHBLogFreetypePrint("erroring a good value for all games, since some options might");
            WHBLogFreetypePrint("favor games with a few very large files but be detrimental");
            WHBLogFreetypePrint("for small files. Recommended Values: 32,64,128,256 etc.");
        }
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Option \uE001 Button = Cancel");
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
            const bool validAccount = !dumpingOnlineFiles || (dumpingOnlineFiles && allUsers[selectedAccount].networkAccount && allUsers[selectedAccount].passwordCached);
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

uint8_t showDialogPrompt(const char* message, const char* button1, const char* button2) {
    sleep_for(100ms);
    uint8_t selectedOption = 0;
    while(true) {
        WHBLogFreetypeStartScreen();

        // Print each line
        std::istringstream messageStream(message);
        std::string line;

        while(std::getline(messageStream, line)) {
            WHBLogPrint(line.c_str());
        }

        WHBLogFreetypePrint("");
        WHBLogFreetypePrintf("%c [%s]", OPTION(0), button1);
        if (button2 != nullptr) WHBLogFreetypePrintf("%c [%s]", OPTION(1), button2);
        WHBLogFreetypePrint("");
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Option");
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

void showDialogPrompt(const char* message, const char* button) {
    showDialogPrompt(message, button, nullptr);
}

const char* errorMessage = nullptr;
void setErrorPrompt(const char* message) {
    errorMessage = message;
}

std::string messageCopy;
void setErrorPrompt(std::string message) {
    messageCopy = std::move(message);
    setErrorPrompt(messageCopy.c_str());
}

void showErrorPrompt(const char* button) {
    std::string promptMessage("An error occurred:\n");
    if (errorMessage) promptMessage += errorMessage;
    else promptMessage += "No error was specified!";
    showDialogPrompt(promptMessage.c_str(), button);
}