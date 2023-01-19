#include "common.h"

void startQueue(uint64_t queueByteSize);
void startSingleDump();
void showCurrentProgress();

void setDumpingStatus(const std::wstring& message);
void setFile(const char* filename, uint64_t total);
void setFileProgress(uint64_t copied);

double calculatePercentage(uint64_t copied, uint64_t total);
void printEstimateTime();
std::wstring formatByteSize(uint64_t bytes);
std::wstring formatByteSizes(uint64_t firstBytes, uint64_t secondBytes);
std::wstring formatElapsedTime(std::chrono::seconds elapsedSeconds);