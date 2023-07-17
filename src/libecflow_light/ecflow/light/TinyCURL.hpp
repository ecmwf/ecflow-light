/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_TINYCURL_HPP
#define ECFLOW_LIGHT_TINYCURL_HPP

#include <iostream>
#include <sstream>
#include <string>

#include <curl/curl.h>

#include "ecflow/light/TinyREST.hpp"

namespace ecflow::light {
namespace net {

class TinyCURL {
public:
    TinyCURL() {
        // CURL init
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~TinyCURL() {
        // CURL cleanup
        curl_global_cleanup();
    }

    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }

    void get(const URL& url) {

        auto curl = curl_easy_init();

        if (!curl) {
            throw std::runtime_error("Unable to initialise cURL handle");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.as_string().c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        std::string response_string;
        std::string header_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

        auto result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            std::cout << "curl_easy_perform(GET) failed: " << curl_easy_strerror(result) << std::endl;
        }

        std::cout << header_string << std::endl;
        std::cout << response_string << std::endl;

        curl_easy_cleanup(curl);
    }


    static size_t read_callback(char* ptr, size_t size [[maybe_unused]], size_t nmemb [[maybe_unused]],
                                std::string* stream) {
        memcpy(ptr, stream->c_str(), stream->size());
        return stream->size();
    }


    void put(const URL& url, const std::string& request) {

        std::cout << "--- PUT: [" << url.as_string() << "] -> " << request << std::endl;

        auto curl = curl_easy_init();

        if (!curl) {
            throw std::runtime_error("Unable to initialise cURL handle");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.as_string().c_str());

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        struct curl_slist* headers = nullptr;
        headers                    = curl_slist_append(headers, "Content-Type: application/json");
        headers                    = curl_slist_append(headers, "Authorization: Bearer justworks");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        // std::string request_string = R"({"type":"label","name":"task_label","value":"X"})";
        std::string request_string = request;
        curl_easy_setopt(curl, CURLOPT_READDATA, &request_string);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)request_string.size());

        auto result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            std::cout << "curl_easy_perform(PUT) failed: " << curl_easy_strerror(result) << std::endl;
        }

        curl_easy_cleanup(curl);
    }


    void post(const URL& url, const std::string& request) {

        auto curl = curl_easy_init();

        if (!curl) {
            throw std::runtime_error("Unable to initialise cURL handle");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.as_string().c_str());

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        struct curl_slist* headers = nullptr;
        headers                    = curl_slist_append(headers, "Content-Type: application/json");
        headers                    = curl_slist_append(headers, "Authorization: Bearer justworks");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        // std::string request_string = R"({"type":"label","name":"task_label2","value":"X"})";
        std::string request_string = request;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_string.c_str());

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
};

}  // namespace net
}  // namespace ecflow::light

#endif  // ECFLOW_LIGHT_TINYCURL_HPP
