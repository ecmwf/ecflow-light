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

#include "ecflow/light/TinyREST.h"

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

class URL {
public:
    explicit URL(Host host, Target target) : host_{std::move(host)}, target_{std::move(target)} {}

    [[nodiscard]] std::string str() const {
        // Notice: target is expected to start with "/", so no need to have a separator after host
        return stringify("https://", host_.str(), target_.str());
    }

private:
    Host host_;
    Target target_;
};

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
    [[nodiscard]] struct curl_slist* native() const { return values_; }

    struct curl_slist* values_;
};

class Handle {
public:
    static Handle make_handle(const URL& url, const WrapperCURL::List& fields) {
        Handle curl;

        curl.set_noprogress();
        curl.set_keepalive();
        curl.set_maxredirs(50L);

        // TODO: Remove the following insecurities!
        curl.set_verifyhost(false);
        curl.set_verifypeer(false);
        curl.set_verifystatus(false);

        curl.set_url(url);

        curl.set_headers(fields);

        return curl;
    }

    Handle() : handle_{curl_easy_init()} {
        if (!handle_) {
            throw UnsuccessfulOperation("Unable to initialise cURL handle");
        }
    }

    // Handle object cannot be copied, but it can be moved!
    Handle(const Handle&)            = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(Handle&& other) noexcept : handle_{other.handle_} { other.handle_ = nullptr; }
    Handle& operator=(Handle&& other) noexcept {
        handle_       = other.handle_;
        other.handle_ = nullptr;
        return *this;
    };

    ~Handle() { curl_easy_cleanup(handle_); }

    void set_headers(const List& headers) { curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headers.native()); }

    void set_url(const URL& url) {
        auto url_as_string = url.str();
        curl_easy_setopt(handle_, CURLOPT_URL, url_as_string.c_str());
    }

    void set_data(CURLoption option, const std::string& value) { curl_easy_setopt(handle_, option, &value); }

    void set_noprogress(bool flag = true) { set_option(CURLOPT_NOPROGRESS, flag ? 1L : 0L); }
    void set_keepalive(bool flag = true) { set_option(CURLOPT_TCP_KEEPALIVE, flag ? 1L : 0L); }
    void set_maxredirs(long maxredirs) { set_option(CURLOPT_MAXREDIRS, maxredirs); }

    void set_verifyhost(bool flag = true) { set_option(CURLOPT_SSL_VERIFYHOST, flag ? 1L : 0L); }
    void set_verifypeer(bool flag = true) { set_option(CURLOPT_SSL_VERIFYPEER, flag ? 1L : 0L); }
    void set_verifystatus(bool flag = true) { set_option(CURLOPT_SSL_VERIFYSTATUS, flag ? 1L : 0L); }

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

Response TinyRESTClient::handle(const Host& host, const Request<Method::GET>& request) const {

    WrapperCURL::List header_fields;
    for (const auto& field : request.header().fields()) {
        header_fields.append(field.name, field.value);
    }

    WrapperCURL::Handle curl =
        WrapperCURL::Handle::make_handle(WrapperCURL::URL{host, request.header().target()}, header_fields);

    // Configure GET related options

    std::string response_string;
    std::string header_string;
    curl.set_option(CURLOPT_WRITEFUNCTION, writeFunction);
    curl.set_option(CURLOPT_WRITEDATA, &response_string);
    curl.set_option(CURLOPT_HEADERDATA, &header_string);

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    std::cout << header_string << std::endl;
    std::cout << response_string << std::endl;

    return Response{Status::Code::OK};
}

Response TinyRESTClient::handle(const Host& host, const Request<Method::POST>& request) const {

    WrapperCURL::List headers;
    for (const auto& field : request.header().fields()) {
        headers.append(field.name, field.value);
    }

    WrapperCURL::Handle curl =
        WrapperCURL::Handle::make_handle(WrapperCURL::URL{host, request.header().target()}, headers);

    // Configure PUT related options

    curl.set_option(CURLOPT_POST, 1L);
    curl.set_option(CURLOPT_POSTFIELDS, request.body().value().c_str());

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    return Response{Status::Code::OK};
}

Response TinyRESTClient::handle(const Host& host, const Request<Method::PUT>& request) const {

    std::cerr << "--------" << std::endl;

    WrapperCURL::List headers;
    for (const auto& field : request.header().fields()) {
        headers.append(field.name, field.value);
    }

    WrapperCURL::Handle curl =
        WrapperCURL::Handle::make_handle(WrapperCURL::URL{host, request.header().target()}, headers);

    // Configure PUT related options

    curl.set_option(CURLOPT_PUT, 1L);
    curl.set_option(CURLOPT_UPLOAD, 1L);

    curl.set_option(CURLOPT_READFUNCTION, read_callback);
    curl.set_data(CURLOPT_READDATA, request.body().value());
    curl.set_option(CURLOPT_INFILESIZE_LARGE, request.body().value().size());

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    std::cerr << "--------" << std::endl;

    return Response{Status::Code::OK};
};

}  // namespace net

}  // namespace ecflow::light
