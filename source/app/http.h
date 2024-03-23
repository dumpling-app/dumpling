#pragma once

#include <nsysnet/netconfig.h>
#include <nsysnet/_socket.h>
#include <nsysnet/nssl.h>
#include <curl/curl.h>
#include <nn/ac.h>

#include <whb/log.h>

#include <string>
#include <vector>
#include <atomic>

#include "http_caroot.h"

void http_init();

size_t http_curlWriteCallback(void* ptr, size_t size, size_t nmemb, std::string* data);

/**
 * @brief Uploads a file with a POST request to the specified URL (the file is sent as the body of the request)
 * @note This function is blocking
 * @param url The URL to upload the file to
 * @param data The data to upload
 * @param responseCode The response code of the request
 * @return
 */
bool http_uploadFile(const std::string& url, const std::vector<uint8_t>& data, int& responseCode);

void http_exit();