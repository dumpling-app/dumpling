#include "progress.h"
#include "menu.h"
#include "filesystem.h"
#include "gui.h"

// TODO: Fix smoothing so it doesn't show a radically different speed
#define SMOOTHING_FACTOR 1.0

// Current Dumping Context

OSTime startTime;
bool isQueueDump;

std::string dumpingMessage;
const char* currFilename;

uint64_t totalQueueBytes;
uint64_t copiedQueueBytes;
uint64_t totalFileBytes;
uint64_t copiedFileBytes;

OSTick lastTime;
uint64_t lastBytesCopied;
uint64_t bytesCopiedSecond;
uint32_t fileErrors = 0;


// Show Progress Functions

void startQueue(uint64_t queueByteSize) {
    startSingleDump();
    totalQueueBytes = queueByteSize;
}

void startSingleDump() {
    totalQueueBytes = 0;
    copiedQueueBytes = 0;
    totalFileBytes = 0;
    copiedFileBytes = 0;
    fileErrors = 0;
    
    startTime = OSGetTime();
    lastTime = startTime;
}

void showCurrentProgress() {
    // Calculate the bytes per second and print an estimate of the time
    OSTick timeSinceLastPeriod = OSGetTick()-lastTime;
    if (timeSinceLastPeriod > (OSTick)OSSecondsToTicks(2)) {
        lastTime = OSGetTick();
        // This averages the bytes per second
        bytesCopiedSecond = ((SMOOTHING_FACTOR*(copiedQueueBytes-lastBytesCopied)) + ((1-SMOOTHING_FACTOR)*bytesCopiedSecond)) / OSTicksToSeconds(timeSinceLastPeriod);
        lastBytesCopied = copiedQueueBytes;
    }
    
    // Print general dumping message
    WHBLogFreetypeStartScreen();
    WHBLogPrint("Dumping In Progress:");
    WHBLogPrint("");
    WHBLogPrint(dumpingMessage.c_str());
    if (totalQueueBytes != 0) printEstimateTime();

    WHBLogPrint("");
    WHBLogPrint("Details:");
    WHBLogPrintf("Current Speed = %fMB/s", (double)bytesCopiedSecond/1000000.0);
    if (totalQueueBytes != 0) WHBLogPrintf("Overall Progress = %.1f%% done - %s", calculatePercentage(copiedQueueBytes, totalQueueBytes), formatByteSizes(totalQueueBytes, copiedQueueBytes).c_str());
    else WHBLogPrintf("Overall Progress = %s written", formatByteSize(copiedQueueBytes).c_str());
    if (fileErrors != 0) WHBLogPrintf("Files Skipped/Errors: %d", fileErrors);
    WHBLogPrint("");
    WHBLogPrintf("File Name = %s", currFilename);
    WHBLogPrintf("File Progress = %.1f%% done - %s", calculatePercentage(copiedFileBytes, totalFileBytes), formatByteSizes(totalFileBytes, copiedFileBytes).c_str());
    WHBLogFreetypeScreenPrintBottom("===============================");
    WHBLogFreetypeScreenPrintBottom("\uE001 Button = Cancel Dumping");
    WHBLogFreetypeDrawScreen();
}


// Set Progress Functions

void setDumpingStatus(std::string message) {
    dumpingMessage = message;
}

void setFile(const char* filename, uint64_t total) {
    currFilename = filename;
    totalFileBytes = total;
    copiedFileBytes = 0;
}

void setFileProgress(uint64_t copied) {
    copiedFileBytes += copied;
    copiedQueueBytes += copied;
}

void reportFileError() {
    fileErrors++;
}


// Helper Functions

double calculatePercentage(uint64_t copied, uint64_t total) {
    return ((double)copied / (double)total) * 100.0;
}

void printEstimateTime() {
    // Calculate the remaining time
    uint64_t remainingSeconds = (totalQueueBytes-copiedQueueBytes) / bytesCopiedSecond;
    OSCalendarTime remainingTime;
    OSTicksToCalendarTime(OSSecondsToTicks(remainingSeconds), &remainingTime);

    // Print the remaining time
    std::string estimatedTimeStr("Estimated Time Remaining: ");
    if (remainingTime.tm_hour > 0) {
        estimatedTimeStr += std::to_string(remainingTime.tm_hour);
        estimatedTimeStr += " hours, ";
        estimatedTimeStr += std::to_string(remainingTime.tm_min);
        estimatedTimeStr += " minutes left";
    }
    else if (remainingTime.tm_min > 0) {
        estimatedTimeStr += std::to_string(remainingTime.tm_min);
        estimatedTimeStr += " minutes, ";
        estimatedTimeStr += std::to_string(remainingTime.tm_sec);
        estimatedTimeStr += " seconds left";
    }
    else if (remainingTime.tm_sec > 0) {
        estimatedTimeStr += std::to_string(remainingTime.tm_sec);
        estimatedTimeStr += " seconds left";
    }
    else {
        estimatedTimeStr += "Unknown";
    }
    WHBLogPrint(estimatedTimeStr.c_str());
}