/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_TINYREST_H
#define ECFLOW_LIGHT_TINYREST_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ecflow::light {
namespace net {

template <typename... ARGS>
std::string stringify(ARGS... args) {
    std::ostringstream os;
    ((os << args), ...);
    return os.str();
}

class URL {
public:
    explicit URL(std::string url) : url_{std::move(url)} {}

    [[nodiscard]] const std::string& as_string() const { return url_; }

private:
    std::string url_;
};

struct UnknownStatusCode : public std::exception {};
struct UnsupportedMethodDetected : public std::exception {};
struct StillNotImplemented : public std::exception {};

enum class Method
{
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH
};

class Status {
public:
    enum class Code : uint32_t
    {
        // Informal responses
        UNKNOWN = 0,

        // Successful responses
        OK = 200,

        // Redirection responses

        // Client Error responses
        BAD_REQUEST  = 400,
        UNAUTHORIZED = 401,
        NOT_FOUND    = 404,

        // Server Error responses
        INTERNAL_SERVER_ERROR = 500
    };

    static const std::string& as_description(Code code) {
        auto found = std::find_if(std::begin(status_set_), std::end(status_set_),
                                  [&code](const Status& status) { return status.code_ == code; });
        if (found == std::end(status_set_)) {
            throw UnknownStatusCode();
        }

        return found->description_;
    }

private:
    Status(Code code, std::string_view description) : code_{code}, description_{description} {}

    Code code_;
    std::string description_;

    static std::vector<Status> status_set_;
};

struct Field {
    using name_t  = std::string;
    using value_t = std::string;

    std::string name;
    std::string value;
};

class Fields {
private:
    using fields_set_t   = std::vector<Field>;
    using const_iterator = std::vector<Field>::const_iterator;

public:
    void insert(Field field) { fields_.push_back(std::move(field)); }
    void insert(const Field::name_t& name, const Field::value_t& value) { fields_.push_back(Field{name, value}); }

    [[nodiscard]] bool empty() const { return fields_.empty(); }
    [[nodiscard]] size_t size() const { return fields_.size(); }

    void clear() { fields_.clear(); }

    [[nodiscard]] const_iterator begin() const { return fields_.begin(); }
    [[nodiscard]] const_iterator end() const { return fields_.end(); }

private:
    fields_set_t fields_;
};

class Header {
    using version_t = uint32_t;
    using fields_t  = Fields;

public:
    Header() : version_{11}, fields_{} {}

    [[nodiscard]] version_t version() const { return version_; }
    [[nodiscard]] const fields_t& fields() const { return fields_; }

    void add(const Field& field) { fields_.insert(field); }

private:
    version_t version_;
    fields_t fields_;
};

class RequestHeader : public Header {
public:
    using url_t = URL;  // TODO: this should be 'target_t' instead... to be combined with 'Host' header to become a URL
    using method_t = Method;

    explicit RequestHeader(method_t method, url_t url) : Header(), url_{std::move(url)}, method_{method} {}

    [[nodiscard]] Method method() const { return method_; };
    [[nodiscard]] const URL& url() const { return url_; };

private:
    url_t url_;
    method_t method_;
};

class ResponseHeader : public Header {
public:
    using status_t = Status::Code;

    explicit ResponseHeader(status_t status) : Header(), status_{status} {}

    [[nodiscard]] status_t status() const { return status_; };

private:
    status_t status_;
};

class Body {
public:
    using value_t = std::string;

    Body() : value_() {}
    explicit Body(value_t value) : value_(std::move(value)) {}

    [[nodiscard]] const value_t& value() const { return value_; }

private:
    value_t value_;
};

class Request {
public:
    using url_t    = URL;
    using header_t = RequestHeader;
    using body_t   = Body;

    explicit Request(url_t url, header_t::method_t method) : header_{method, std::move(url)}, body_{} {}

    [[nodiscard]] header_t::method_t method() const { return header_.method(); }

    void add_header_field(const Field& field) { header_.add(field); }
    void add_body(const Body& body) { body_ = body; }

    [[nodiscard]] const header_t& header() const { return header_; }
    [[nodiscard]] const body_t& body() const { return body_; }


private:
    header_t header_;
    body_t body_;
};

class Response {
public:
    using header_t = ResponseHeader;
    using body_t   = Body;

    explicit Response(header_t::status_t status) : header_{status}, body_{} {}

    [[nodiscard]] const header_t& header() const { return header_; }
    [[nodiscard]] const body_t& body() const { return body_; }

private:
    header_t header_;
    body_t body_;
};

class TinyRESTClient {
public:
    [[nodiscard]] Response handle(const Request&) const;

private:
    [[nodiscard]] Response GET(const Request&) const;
    [[nodiscard]] Response HEAD(const Request&) const;
    [[nodiscard]] Response POST(const Request&) const;
    [[nodiscard]] Response PUT(const Request&) const;
    [[nodiscard]] Response DELETE(const Request&) const;

    static size_t read_callback(char* ptr, size_t size [[maybe_unused]], size_t nmemb [[maybe_unused]],
                                std::string* stream) {
        memcpy(ptr, stream->c_str(), stream->size());
        return stream->size();
    }

    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }
};

}  // namespace net
}  // namespace ecflow::light

#endif  // ECFLOW_LIGHT_TINYREST_H
