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

#include <eckit/utils/StringTools.h>
#include <eckit/utils/Tokenizer.h>

#include "ecflow/light/TinyREST.h"

namespace ecflow::light {

namespace net {

std::ostream& operator<<(std::ostream& s, const ResponseHeader& o) {
    for (const auto& field : o.fields()) {
        s << "{'" << field.name << "': '" << field.value << "'}";
    }
    return s;
}

std::ostream& operator<<(std::ostream& s, const Body& o) {
    s << "{value:'" << o.value() << "'}";
    return s;
}

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

/**
 * Copies the content from input `userdata` into output `buffer`, returning the number of characters copied
 */
size_t read_callback(char* buffer, size_t size [[maybe_unused]], size_t nitems [[maybe_unused]], void* userdata) {
    auto data = static_cast<std::string*>(userdata);

    memcpy(buffer, data->c_str(), data->size());
    return data->size();
}

size_t write_headers_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto data = static_cast<Fields*>(userdata);

    std::string all = eckit::StringTools::trim(std::string(buffer, size * nitems), " \n\r");

    // Ignore 'response-line'
    if (eckit::StringTools::startsWith(all, "HTTP/")) {
        return size * nitems;
    }

    // Ignore (final) empty line
    if (all.empty()) {
        return size * nitems;
    }

    std::vector<std::string> tokens;
    eckit::Tokenizer tokenizer(":");
    tokenizer(all, tokens);

    data->insert(tokens[0], eckit::StringTools::trim(tokens[1]));

    return size * nitems;
}

size_t write_body_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto data = static_cast<Body*>(userdata);

    std::string all = eckit::StringTools::trim(std::string(buffer, size * nitems), " \n\r");
    *data = Body(all);

    return size * nitems;
}

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
            throw UnsuccessfulOperation("curl_easy_perform operation failed: ", curl_easy_strerror(result));
        }
    }

    [[nodiscard]] long response_code() const {
        long code;
        curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &code);
        return code;
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

    Fields fields;
    curl.set_option(CURLOPT_HEADERFUNCTION, WrapperCURL::write_headers_callback);
    curl.set_option(CURLOPT_HEADERDATA, &fields);

    Body body;
    curl.set_option(CURLOPT_WRITEFUNCTION, WrapperCURL::write_body_callback);
    curl.set_option(CURLOPT_WRITEDATA, &body);

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    auto code   = curl.response_code();
    auto header = ResponseHeader(Status::from_value(code), fields);

    return Response{header, body};
}

Response TinyRESTClient::handle(const Host& host, const Request<Method::POST>& request) const {

    WrapperCURL::List headers;
    for (const auto& field : request.header().fields()) {
        headers.append(field.name, field.value);
    }

    WrapperCURL::Handle curl =
        WrapperCURL::Handle::make_handle(WrapperCURL::URL{host, request.header().target()}, headers);

    // Configure POST related options

    curl.set_option(CURLOPT_POST, 1L);
    curl.set_option(CURLOPT_POSTFIELDS, request.body().value().c_str());

    Fields fields;
    curl.set_option(CURLOPT_HEADERFUNCTION, WrapperCURL::write_headers_callback);
    curl.set_option(CURLOPT_HEADERDATA, &fields);

    Body body;
    curl.set_option(CURLOPT_WRITEFUNCTION, WrapperCURL::write_body_callback);
    curl.set_option(CURLOPT_WRITEDATA, &body);

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    auto code   = curl.response_code();
    auto header = ResponseHeader(Status::from_value(code), fields);

    return Response{header, body};
}

Response TinyRESTClient::handle(const Host& host, const Request<Method::PUT>& request) const {

    WrapperCURL::List headers;
    for (const auto& field : request.header().fields()) {
        headers.append(field.name, field.value);
    }

    WrapperCURL::Handle curl =
        WrapperCURL::Handle::make_handle(WrapperCURL::URL{host, request.header().target()}, headers);

    // Configure PUT related options

    curl.set_option(CURLOPT_UPLOAD, 1L);

    curl.set_option(CURLOPT_READFUNCTION, WrapperCURL::read_callback);
    curl.set_data(CURLOPT_READDATA, request.body().value());
    curl.set_option(CURLOPT_INFILESIZE_LARGE, request.body().value().size());

    Fields fields;
    curl.set_option(CURLOPT_HEADERFUNCTION, WrapperCURL::write_headers_callback);
    curl.set_option(CURLOPT_HEADERDATA, &fields);

    Body body;
    curl.set_option(CURLOPT_WRITEFUNCTION, WrapperCURL::write_body_callback);
    curl.set_option(CURLOPT_WRITEDATA, &body);

    try {
        curl.perform();
    }
    catch (WrapperCURL::UnsuccessfulOperation& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return Response{Status::Code::BAD_REQUEST};  // TODO: must report proper error!
    }

    auto code   = curl.response_code();
    auto header = ResponseHeader(Status::from_value(code), fields);

    return Response{header, body};
};

}  // namespace net

}  // namespace ecflow::light
