#include "common.h"

void startQueue(uint64_t queueByteSize);
void startSingleDump();
void showCurrentProgress();

void setDumpingStatus(const std::string& message);
void setFile(const char* filename, uint64_t total);
void setFileProgress(uint64_t copied);

double calculatePercentage(uint64_t copied, uint64_t total);
void printEstimateTime();
std::string formatByteSize(uint64_t bytes);
std::string formatByteSizes(uint64_t firstBytes, uint64_t secondBytes);
std::string formatElapsedTime(std::chrono::seconds elapsedSeconds);