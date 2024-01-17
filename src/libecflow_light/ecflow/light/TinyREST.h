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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ecflow/light/StringUtils.h"

namespace ecflow::light {
namespace net {

class Host {
public:
    explicit Host(std::string host) : uri_host_{std::move(host)} {}
    explicit Host(const std::string& host, const std::string& port) : uri_host_{stringify(host, ":", port)} {}

    [[nodiscard]] const std::string& str() const { return uri_host_; }

private:
    std::string uri_host_;
};

class Target {
public:
    explicit Target(std::string target) : target_{std::move(target)} {}

    [[nodiscard]] const std::string& str() const { return target_; }

private:
    std::string target_;
};

class URL {
public:
    explicit URL(Host host) : host_{std::move(host)}, target_{""} {}
    explicit URL(Host host, Target target) : host_{std::move(host)}, target_{std::move(target)} {}

    [[nodiscard]] std::string str() const {
        // Notice: target is expected to start with "/", so no need to have a separator after host
        return stringify("https://", host_.str(), target_.str());
    }

private:
    Host host_;
    Target target_;
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

    static Code from_value(long value) {
        auto found = std::find_if(std::begin(status_set_), std::end(status_set_), [&value](const Status& status) {
            return static_cast<std::underlying_type_t<Code>>(status.code_) == value;
        });
        if (found == std::end(status_set_)) {
            throw UnknownStatusCode();
        }

        return found->code_;
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
    Header(fields_t fields) : version_{11}, fields_{std::move(fields)} {}

    [[nodiscard]] version_t version() const { return version_; }
    [[nodiscard]] const fields_t& fields() const { return fields_; }

    void add(const Field& field) { fields_.insert(field); }

private:
    version_t version_;
    fields_t fields_;
};

template <Method METHOD>
class RequestHeader : public Header {
public:
    using target_t = Target;

    explicit RequestHeader(target_t target) : Header(), target_{std::move(target)} {}

    [[nodiscard]] Method method() const { return METHOD; };
    [[nodiscard]] const target_t& target() const { return target_; };

private:
    target_t target_;
};

class ResponseHeader : public Header {
public:
    using status_t = Status::Code;

    explicit ResponseHeader(status_t status) : Header(), status_{status} {}
    explicit ResponseHeader(status_t status, Fields fields) : Header(std::move(fields)), status_{status} {}

    [[nodiscard]] status_t status() const { return status_; };

    friend std::ostream& operator<<(std::ostream& o, const ResponseHeader& header);

private:
    status_t status_;
};

class Body {
public:
    using value_t = std::string;

    Body() : value_() {}
    explicit Body(value_t value) : value_(std::move(value)) {}

    [[nodiscard]] const value_t& value() const { return value_; }

    friend std::ostream& operator<<(std::ostream& s, const Body& o);

private:
    value_t value_;
};

template <Method METHOD>
class Request {
public:
    using target_t = Target;
    using header_t = RequestHeader<METHOD>;
    using body_t   = Body;

    explicit Request(target_t target) : header_{std::move(target)}, body_{} {}

    [[nodiscard]] Method method() const { return METHOD; }

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
    Response(header_t::status_t status, const body_t& body) : header_{status}, body_{body} {}
    Response(const header_t& header, const body_t& body) : header_{header}, body_{body} {}

    [[nodiscard]] const header_t& header() const { return header_; }
    [[nodiscard]] const body_t& body() const { return body_; }

private:
    header_t header_;
    body_t body_;
};

class TinyRESTClient {
public:
    [[nodiscard]] Response handle(const Host& host, const Request<Method::GET>& request) const;
    [[nodiscard]] Response handle(const Host& host, const Request<Method::POST>& request) const;
    [[nodiscard]] Response handle(const Host& host, const Request<Method::PUT>& request) const;
};

}  // namespace net
}  // namespace ecflow::light

#endif  // ECFLOW_LIGHT_TINYREST_H
