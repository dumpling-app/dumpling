#pragma once

#include <nsysnet/netconfig.h>
#include <nsysnet/_socket.h>
#include <nsysnet/nssl.h>
#include <curl/curl.h>
#include <nn/ac.h>

#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <whb/log.h>

#include <string>
#include <vector>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <queue>

#include "http_caroot.h"

typedef struct UploadQueueEntry UploadQueueEntry;
typedef void (*UploadQueueCallback)(UploadQueueEntry& entry, float progress);

struct UploadQueueEntry {
    int handle;
    CURL* curl;
    std::string url;
    std::vector<uint8_t> data;
    long responseCode;
    bool started;
    bool finished;
    bool error;
    UploadQueueCallback callback;
    void* userdata;
};

void http_init();

/**
 * @brief Submit a file to be uploaded to a server
 * @param url The URL to upload the file to
 * @param data The data to upload
 * @param updateCallback A callback function that will be called when the upload has finished or when the progress has changed
 * @note This function is thread-safe and you need to use the handle to check if the upload has finished / check progress
 */
void http_submitUploadQueue(const std::string& url, const std::vector<uint8_t>& data, UploadQueueCallback updateCallback, void* userdata);

int http_safeDelete(int handle);

void http_exit();