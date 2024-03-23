#include "http.h"

static std::atomic<bool> is_http_initialized = false;

void http_init() {
    socket_lib_init();
    netconf_init();
    nn::ac::Initialize();
    nn::ac::ConnectAsync();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    is_http_initialized = true;
}

size_t http_curlWriteCallback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

bool http_uploadFile(const std::string& url, const std::vector<uint8_t>& data, int& responseCode) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    if (url.empty())
        return false;

    if (data.empty())
        return false;

    NetConfProxyConfig proxyConfig;
    netconf_get_proxy_config(&proxyConfig);
    if (proxyConfig.use_proxy == NET_CONF_PROXY_ENABLED) {
        char proxyUrl[196];
        snprintf(proxyUrl, sizeof(proxyUrl), "%s:%d", proxyConfig.host, proxyConfig.port);
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyUrl);
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    struct curl_blob blob {};
    blob.data = (void*)caroot_pem;
    blob.len = caroot_pem_len;
    blob.flags = CURL_BLOB_COPY;
    curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        WHBLogPrintf("curl_easy_perform() failed: %s (%d)", curl_easy_strerror(res), res);
        return false;
    }
    else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return true;
}

void http_exit() {

    if (!is_http_initialized)
        return;

    curl_global_cleanup();
    nn::ac::Finalize();
    netconf_close();
    socket_lib_finish();

    is_http_initialized = false;
}