#include "menu.h"
#include "titles.h"
#include "users.h"
#include "navigation.h"
#include "titlelist.h"
#include "dumping.h"
#include "filesystem.h"

// Menu screens

bool showLoadingScreen() {
    WHBLogConsoleSetColor(0x0b5d5e00);
    WHBLogPrint("Dumpling V2.0.4");
    WHBLogPrint("-- Made by Crementif and Emiyl --");
    WHBLogPrint("");
    WHBLogPrint("Loading games...");
    WHBLogConsoleDraw();
    return loadUsers() && loadTitles(true);
}

void showMainMenu() {
    uint8_t selectedOption = 0;
    bool startSelectedOption = false;
    while(!startSelectedOption) {
        // Print menu text
        clearScreen();
        WHBLogPrint("Dumpling V2.0.4");
        WHBLogPrint("===============================");
        WHBLogPrintf("%c Dump a game disc", selectedOption==0 ? '>' : ' ');
        WHBLogPrintf("%c Dump digital games", selectedOption==1 ? '>' : ' ');
        WHBLogPrint("");
        WHBLogPrintf("%c Dump files to use Cemu online", selectedOption==2 ? '>' : ' ');
        WHBLogPrintf("%c Dump Wii U applications (e.g. Friend List, Browser etc.)", selectedOption==3 ? '>' : ' ');
        WHBLogPrintf("%c Dump Compatibility Files for Cemu", selectedOption==4 ? '>' : ' ');
        WHBLogPrint("");
        WHBLogPrintf("%c Dump whole MLC (everything stored on internal storage)", selectedOption==5 ? '>' : ' ');
        WHBLogPrintf("%c Dump only Base files of a game", selectedOption==6 ? '>' : ' ');
        WHBLogPrintf("%c Dump only Update files of a game", selectedOption==7 ? '>' : ' ');
        WHBLogPrintf("%c Dump only DLC files of a game", selectedOption==8 ? '>' : ' ');
        //WHBLogPrintf("%c Dump only Save files of a game", selectedOption==9 ? '>' : ' '); // TODO: To extend save file dumping purposes, read the meta files from the save files to show save files that are from disc games
        WHBLogPrint("===============================");
        WHBLogPrint("A Button = Select Option");
        WHBLogPrint("B Button = Exit Dumpling");
        WHBLogConsoleDraw();

        // Loop until there's new input
        OSSleepTicks(OSMillisecondsToTicks(200)); // Cooldown between each button press
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
                uint8_t exitSelectedOption = showDialogPrompt("Do you really want to exit Dumpling?", "Yes", "No");
                if (exitSelectedOption == 0) return;
                else break;
            }
            OSSleepTicks(OSMillisecondsToTicks(50));
        }
    }

    // Go to the selected menu
    switch(selectedOption) {
        case 0:
            if (!dumpDisc()) return; // Quit the homebrew completely when a fatal error occurs during disc dumping
            break;
        case 1:
            showTitleList("Select all the games you want to dump!", {.filterTypes = dumpTypeFlags::GAME, .dumpTypes = (dumpTypeFlags::GAME | dumpTypeFlags::UPDATE | dumpTypeFlags::DLC | dumpTypeFlags::SAVE), .queue = true});
            break;
        case 2:
            dumpOnlineFiles();
            break;
        case 3:
            showTitleList("Select all the system applications you want to dump!", {.filterTypes = dumpTypeFlags::SYSTEM_APP, .dumpTypes = dumpTypeFlags::GAME, .queue = true});
            break;
        case 4:
            dumpCompatibilityFiles();
            break;
        case 5:
            dumpMLC();
            break;
        case 6:
            showTitleList("Select all the games that you want to dump the base game from!", {.filterTypes = dumpTypeFlags::GAME, .dumpTypes = dumpTypeFlags::GAME, .queue = true});
            break;
        case 7:
            showTitleList("Select all the games that you want to dump the update from!", {.filterTypes = dumpTypeFlags::UPDATE, .dumpTypes = dumpTypeFlags::UPDATE, .queue = true});
            break;
        case 8:
            showTitleList("Select all the games that you want to dump the DLC from!", {.filterTypes = dumpTypeFlags::DLC, .dumpTypes = dumpTypeFlags::DLC, .queue = true});
            break;
        case 9:
            //showTitleList("Select all the games that you want to dump the save from!", {.filterTypes = (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE), .dumpTypes = (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE), .queue = true});
            break;
        default:
            break;
    }

    OSSleepTicks(OSMillisecondsToTicks(500));
    showMainMenu();
}

bool showOptionMenu(dumpingConfig& config, bool showAccountOption) {
    uint8_t selectedOption = 0;
    uint8_t selectedAccount = 0;
    while(true) {
        // Print option menu text
        clearScreen();
        WHBLogPrint("Change any options for this dump:");
        WHBLogPrint("===============================");
        WHBLogPrintf("%c Dump destination: %s", selectedOption==0 ? '>' : ' ', config.location == dumpLocation::SDFat ? "SD Card" : "USB Drive");
        if (showAccountOption) WHBLogPrintf("%c Account: %s", selectedOption==1 ? '>' : ' ', allUsers[selectedAccount].miiName.c_str());
        WHBLogPrint("");
        WHBLogPrintf("%c Start", selectedOption==(1+showAccountOption) ? '>' : ' ');
        WHBLogPrint("===============================");
        WHBLogPrint("A Button = Select Option");
        WHBLogPrint("B Button = Go Back");
        WHBLogPrint("Left/Right = Change Value");
        WHBLogConsoleDraw();

        OSSleepTicks(OSMillisecondsToTicks(200)); // Cooldown between each button press
        updateInputs();
        while(true) {
            updateInputs();
            // Check each button state
            if (navigatedUp() && selectedOption > 0) {
                selectedOption--;
                break;
            }
            if (navigatedDown() && selectedOption < (1+showAccountOption)) {
                selectedOption++;
                break;
            }
            if (navigatedLeft()) {
                if (selectedOption == 0 && config.location == dumpLocation::USBFat) {
                    config.location = dumpLocation::SDFat;
                }
                if (selectedOption == 1 && showAccountOption && selectedAccount > 0) {
                    selectedAccount--;
                }
                break;
            }
            if (navigatedRight()) {
                if (selectedOption == 0 && config.location == dumpLocation::SDFat) {
                    if (usbfatMounted) config.location = dumpLocation::USBFat;
                    else showDialogPrompt("Didn't detect a FAT32 formatted USB stick when Dumpling started.\nMake sure that it's plugged in BEFORE you start Dumpling!", "OK");
                }
                if (selectedOption == 1 && showAccountOption && selectedAccount < allUsers.size()-1) {
                    selectedAccount++;
                }
                break;
            }
            if (pressedOk() && selectedOption == (1+showAccountOption)) {
                config.accountID = allUsers[selectedAccount].persistentId;
                return true;
            }
            if (pressedBack()) {
                return false;
            }
            OSSleepTicks(OSMillisecondsToTicks(50));
        }
    }
}


// Helper functions

uint8_t showDialogPrompt(const char* message, const char* button1, const char* button2) {
    OSSleepTicks(OSMillisecondsToTicks(500));
    uint8_t selectedButton = 0;
    while(true) {
        // Print dialog and buttons
        clearScreen();
        WHBLogPrint(message);
        WHBLogPrint("");
        WHBLogPrintf("%c %s", selectedButton==0 ? '>' : ' ', button1);
        if (button2 != NULL) WHBLogPrintf("%c %s", selectedButton==1 ? '>' : ' ', button2);
        WHBLogPrint("");
        WHBLogPrint("===============================");
        WHBLogPrint("A Button = Select Option");
        WHBLogConsoleDraw();

        // Input loop
        OSSleepTicks(OSMillisecondsToTicks(200));
        updateInputs();
        while (true) {
            updateInputs();
            // Handle navigation between the buttons
            if (button2 != NULL) {
                if (navigatedUp() && selectedButton == 1) {
                    selectedButton = 0;
                    break;
                }
                else if (navigatedDown() && selectedButton == 0) {
                    selectedButton = 1;
                    break;
                }
            }

            if (pressedOk()) {
                return selectedButton;
            }

            OSSleepTicks(OSMillisecondsToTicks(50));
        }
    }
}

void showDialogPrompt(const char* message, const char* button) {
    showDialogPrompt(message, button, NULL);
}

const char* errorMessage;
void setErrorPrompt(const char* message) {
    errorMessage = message;
}

void showErrorPrompt(const char* button) {
    std::string promptMessage("An error occured:\n");
    promptMessage += errorMessage;
    showDialogPrompt(promptMessage.c_str(), button);
}

#define NUM_LINES (16)

void clearScreen() {
    for (int i=0; i<NUM_LINES; i++) {
        WHBLogPrint("");
    }
}