#include "common.h"

void startQueue(uint64_t queueByteSize);
void startSingleDump();
void showCurrentProgress();

void setDumpingStatus(std::string message);
void setFile(const char* filename, uint64_t total);
void setFileProgress(uint64_t copied);
void reportFileError();

double calculatePercentage(uint64_t copied, uint64_t total);
void printEstimateTime();