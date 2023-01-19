#include "progress.h"

#include <utility>
#include "menu.h"
#include "filesystem.h"
#include "gui.h"

#define SMOOTHING_FACTOR 0.2

// Current Dumping Context

OSTime startTime;

std::wstring dumpingMessage;
const char* currFilename;

uint64_t totalQueueBytes;
uint64_t copiedQueueBytes;
uint64_t totalFileBytes;
uint64_t copiedFileBytes;

OSTick lastTime;
uint64_t lastBytesCopied;
uint64_t bytesCopiedSecond;
uint32_t filesCopied = 0;

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
    lastBytesCopied = 0;
    bytesCopiedSecond = 0;
    filesCopied = 0;

    startTime = OSGetTick();
    lastTime = (OSTick)startTime - (OSTick)OSMillisecondsToTicks(1001);
}

extern "C" double profile_getSegment(const char* segmentName);

void showCurrentProgress() {
    // Calculate the bytes per second and print an estimate of the time
    OSTick timeSinceLastPeriod = OSGetTick()-lastTime;
    if (timeSinceLastPeriod > (OSTick)OSSecondsToTicks(1)) {
        lastTime = OSGetTick();
        
        // This averages the bytes per second
        bytesCopiedSecond = (uint64_t)(((SMOOTHING_FACTOR*(double)(copiedQueueBytes-lastBytesCopied)) + ((1-SMOOTHING_FACTOR)*(double)bytesCopiedSecond))*(((double)OSSecondsToTicks(1))/((double)timeSinceLastPeriod)));
        lastBytesCopied = copiedQueueBytes;

        // Print general dumping message
        WHBLogFreetypeStartScreen();
        WHBLogFreetypePrint(L"Dumping In Progress:");
        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrint(dumpingMessage.c_str());
        if (totalQueueBytes != 0) printEstimateTime();

        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrint(L"Details:");
        WHBLogFreetypePrintf(L"File Name = %S", toWstring(currFilename).c_str());
        WHBLogFreetypePrintf(L"Current Speed = %.3fMB/s", (double)bytesCopiedSecond/1000000.0);
        if (totalQueueBytes != 0) WHBLogFreetypePrintf(L"Overall Progress = %.1f%% done - %S", calculatePercentage(copiedQueueBytes, totalQueueBytes), formatByteSizes(copiedQueueBytes, totalQueueBytes).c_str());
        else WHBLogFreetypePrintf(L"Overall Progress = %S written, %d files copied", formatByteSize(copiedQueueBytes).c_str(), filesCopied);
        WHBLogFreetypePrint(L"");
        WHBLogFreetypePrintf(L"File Progress = %.1f%% done - %S", calculatePercentage(copiedFileBytes, totalFileBytes), formatByteSizes(copiedFileBytes, totalFileBytes).c_str());

//        WHBLogFreetypePrintf("Total Fat32 Time Spent on %.0f files: %.0f ms", profile_getSegment("files"), profile_getSegment("total"));
//        WHBLogFreetypePrintf(" - follow_path: %.0f ms", profile_getSegment("follow_path"));
//        WHBLogFreetypePrintf("   - dir_find's time: %.0f ms", profile_getSegment("followfinds"));
//        WHBLogFreetypePrintf(" - dir_register: %.0f ms", profile_getSegment("dir_register"));
//        WHBLogFreetypePrintf("   - dir_find's time: %.0f ms", profile_getSegment("registerfinds"));
//        WHBLogFreetypePrintf("   - dir_alloc: %.0f ms", profile_getSegment("dir_alloc"));

        WHBLogFreetypePrint(L"");
        WHBLogFreetypeScreenPrintBottom(L"===============================");
        WHBLogFreetypeScreenPrintBottom(L"\uE001 Button = Cancel Dumping");
        WHBLogFreetypeDrawScreen();
    }
}


// Set Progress Functions
void setDumpingStatus(const std::wstring& message) {
    dumpingMessage = message;
}

void setFile(const char* filename, uint64_t total) {
    currFilename = filename;
    totalFileBytes = total;
    copiedFileBytes = 0;
    filesCopied++;
}

void setFileProgress(uint64_t copied) {
    copiedFileBytes += copied;
    copiedQueueBytes += copied;
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

constexpr const wchar_t *suffix[] = {L"Bytes", L"KB", L"MB", L"GB"};
std::wstring formatByteSize(uint64_t bytes) {
    constexpr uint8_t length = sizeof(suffix)/sizeof(suffix[0]);

    uint32_t i = 0;
    double dblBytes = (double)bytes;
    if (bytes > 1024) {
        for (i = 0; (bytes/1024) > 0 && i<length-1; i++, bytes /= 1024) {
            dblBytes = ((double)bytes)/1024.0;
        }
    }

    wchar_t output[50];
    swprintf(output, std::size(output), L"%.01lf %S", dblBytes, suffix[i]);
    return output;
}

std::wstring formatByteSizes(uint64_t firstBytes, uint64_t secondBytes) {
    constexpr uint8_t length = sizeof(suffix)/sizeof(suffix[0]);

    uint32_t i = 0;
    double dblDominant = (double)secondBytes;
    double dblOther = (double)firstBytes;
    if (secondBytes > 1024) {
        for (i = 0; (secondBytes/1024) > 0 && i<length-1; i++, firstBytes /= 1024, secondBytes /= 1024) {
            dblDominant = ((double)secondBytes)/1024.0;
            dblOther = ((double)firstBytes)/1024.0;
        }
    }

    wchar_t output[50];
    swprintf(output, std::size(output), L"%.01lf/%.01lf %S", dblOther, dblDominant, suffix[i]);
    return output;
}

std::wstring formatElapsedTime(std::chrono::seconds elapsedSeconds) {
    std::wstringstream fmtTime;
    auto hr = std::chrono::duration_cast<std::chrono::hours>(elapsedSeconds);
    if (hr.count() != 0) {
        elapsedSeconds -= hr;
        fmtTime << hr.count() << ((hr.count() == 1) ? " hour, " : " hours, ");
    }

    auto min = std::chrono::duration_cast<std::chrono::minutes>(elapsedSeconds);
    if (!(hr.count() == 0 && min.count() == 0)) {
        elapsedSeconds -= min;
        fmtTime << min.count() << ((min.count() == 1) ? " minute and " : " minutes and ");
    }

    auto sec = std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds);
    fmtTime << sec.count() << ((sec.count() == 1) ? " second" : " seconds");
    return fmtTime.str();
}