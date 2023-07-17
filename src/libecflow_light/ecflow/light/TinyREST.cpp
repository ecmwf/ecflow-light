/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <curl/curl.h>

#include <stdexcept>

#include "ecflow/light/TinyREST.hpp"

namespace ecflow::light {
namespace net {

std::vector<Status> Status::status_set_ = {
    // Informal responses
    Status{Code::UNKNOWN, "UNKNOWN"},
    // Successful responses
    Status{Code::OK, "OK"},
    // Client Error responses
    Status{Code::BAD_REQUEST, "BAD_REQUEST"}, Status{Code::UNAUTHORIZED, "UNAUTHORIZED"},
    Status{Code::NOT_FOUND, "NOT_FOUND"},
    // Server Error responses
    Status{Code::INTERNAL_SERVER_ERROR, "INTERNAL_SERVER_ERROR"}};

namespace WrapperCURL {

struct UnsuccessfulOperation : public std::runtime_error {
    explicit UnsuccessfulOperation(const char* what) : std::runtime_error(what) {}

    template <typename... ARGS>
    explicit UnsuccessfulOperation(ARGS... args) : std::runtime_error(stringify(args...).c_str()) {}
};

class Handle;

class List {
public:
    List() : values_{nullptr} {}
    ~List() { curl_slist_free_all(values_); }

    void append(const std::string& name, const std::string& value) {
        std::string header = stringify(name, ": ", value);
        values_            = curl_slist_append(values_, header.c_str());
    }

private:
    friend Handle;
    struct curl_slist* native() const { return values_; }

    struct curl_slist* values_;
};

class Handle {
public:
    Handle() : handle_{nullptr} {
        handle_ = curl_easy_init();
        if (!handle_) {
            throw UnsuccessfulOperation("Unable to initialise cURL handle");
        }
    }
    ~Handle() { curl_easy_cleanup(handle_); }

    void set_headers(const List& headers) { curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headers.native()); }

    void set_url(const URL& url) {
        auto url_as_string = url.as_string();
        curl_easy_setopt(handle_, CURLOPT_URL, url_as_string.c_str());
    }

    void set_data(CURLoption option, const std::string& value) { curl_easy_setopt(handle_, option, &value); }

    template <typename VALUE>
    void set_option(CURLoption option, VALUE value) {
        curl_easy_setopt(handle_, option, value);
    }

    void perform() {
        if (auto result = curl_easy_perform(handle_); result != CURLE_OK) {
            throw UnsuccessfulOperation("curl_easy_perform failed: ", curl_easy_strerror(result));
        }
    }

private:
    CURL* handle_;
};

}  // namespace WrapperCURL


Response TinyRESTClient::handle(const Request& request) const {

    switch (request.method()) {
        case Method::GET:
            return GET(request);
        case Method::HEAD:
            return HEAD(request);
        case Method::POST:
            return POST(request);
        case Method::PUT:
            return PUT(request);
        case Method::DELETE:
            return DELETE(request);
        case Method::OPTIONS:
        case Method::CONNECT:
        case Method::TRACE:
        case Method::PATCH:
            [[fallthrough]];
        default:
            throw UnsupportedMethodDetected();
    }
}

Response TinyRESTClient::GET(const Request&) const {
    throw StillNotImplemented();
}

Response TinyRESTClient::HEAD(const Request&) const {
    throw StillNotImplemented();
}

Response TinyRESTClient::POST(const Request&) const {
    throw StillNotImplemented();
}

Response TinyRESTClient::PUT(const Request& request) const {

    // std::cout << "--- PUT: [" << url.as_string() << "] -> " << request << std::endl;

    WrapperCURL::Handle curl;

    URL url = request.header().url();
    curl.set_url(url);
    curl.set_option(CURLOPT_NOPROGRESS, 1L);
    curl.set_option(CURLOPT_MAXREDIRS, 50L);
    curl.set_option(CURLOPT_TCP_KEEPALIVE, 1L);
    curl.set_option(CURLOPT_SSL_VERIFYPEER, 0L);

    WrapperCURL::List headers;
    for (const auto& field : request.header().fields()) {
        headers.append(field.name, field.value);
    }
    //    headers.append("Content-Type: application/json");
    //    headers.append("Authorization: Bearer justworks");
    curl.set_headers(headers);

    curl.set_option(CURLOPT_PUT, 1L);
    curl.set_option(CURLOPT_UPLOAD, 1L);

    curl.set_option(CURLOPT_READFUNCTION, read_callback);
    curl.set_data(CURLOPT_READDATA, request.body().value());
    curl.set_option(CURLOPT_INFILESIZE_LARGE, request.body().value().size());

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    return Response{Status::Code::OK};
};

Response TinyRESTClient::DELETE(const Request&) const {
    throw StillNotImplemented();
}

}  // namespace net
}  // namespace ecflow::light
