#include "http.h"

static std::atomic<bool> is_http_initialized = false;

static std::mutex uploadQueueAccessMutex;
static std::queue<UploadQueueEntry> uploadQueue;

static float currentUploadProgress = 0.0f;

size_t http_curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return size * nmemb;
}

int http_curlProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)  {
    UploadQueueEntry* entry = (UploadQueueEntry*)clientp;
    if (ultotal > 0) {
        currentUploadProgress = (float)ulnow / (float)ultotal;
        entry->callback(*entry, currentUploadProgress);
    }

    return 0;
}

bool http_handleUpload(UploadQueueEntry& entry) {
    CURL* curl = curl_easy_init();
    struct curl_slist *slist = nullptr;
    slist = curl_slist_append(slist, "Content-Type: application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_URL, entry.url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, entry.data.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, entry.data.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, http_curlProgressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, entry);

    struct curl_blob blob {};
    blob.data = (void*)caroot_pem;
    blob.len = caroot_pem_len;
    blob.flags = CURL_BLOB_COPY;
    curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);

    entry.curl = curl;
    entry.started = true;
    currentUploadProgress = 0.0f;
    entry.callback(entry, currentUploadProgress);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        WHBLogPrintf("curl_easy_perform() failed: %s (%d)", curl_easy_strerror(res), res);
        entry.error = true;
        entry.finished = true;
        return false;
    }
    else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &entry.responseCode);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    entry.finished = true;

    return true;
}

void http_uploadQueueWorker() {
    while (is_http_initialized) {
       uploadQueueAccessMutex.lock();
        if (uploadQueue.empty()) {
            uploadQueueAccessMutex.unlock();
            std::this_thread::yield();
            continue;
        }

        UploadQueueEntry entry = uploadQueue.front();
        uploadQueue.pop();

        uploadQueueAccessMutex.unlock();

        WHBLogPrintf("Uploading %lu bytes to %s", entry.data.size(), entry.url.c_str());
        http_handleUpload(entry);
        entry.callback(entry, currentUploadProgress);

    }
}

float http_getCurrentProgress() {
    DCInvalidateRange(&currentUploadProgress, sizeof(currentUploadProgress));
    return currentUploadProgress;
}

void http_submitUploadQueue(const std::string& url, const std::vector<uint8_t>& data, UploadQueueCallback updateCallback, void* userdata) {
    uploadQueueAccessMutex.lock();
    UploadQueueEntry entry { .curl = nullptr, .url = url, .data = data, .started = false, .finished = false, .error = false, .callback = updateCallback, .userdata = userdata};
    uploadQueue.push(entry);
    uploadQueueAccessMutex.unlock();
}

void http_init() {

    if (is_http_initialized)
        return;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    is_http_initialized = true;

    std::thread(http_uploadQueueWorker).detach();
}

void http_exit() {

    if (!is_http_initialized)
        return;

    curl_global_cleanup();
    is_http_initialized = false;
}