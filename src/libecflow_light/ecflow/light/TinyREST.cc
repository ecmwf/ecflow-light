/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */


#include <eckit/io/EasyCURL.h>
#include <eckit/utils/StringTools.h>

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

namespace detail {

class Handle {
public:
    Handle() : handle_{} {
        // TODO: Remove the following insecurities!
        set_verifyhost(false);
        set_verifypeer(false);
    }

    // Handle object cannot be copied!
    Handle(const Handle&)            = delete;
    Handle& operator=(const Handle&) = delete;

    ~Handle() = default;

    Response perform(const URL& url, const Request<Method::GET>& request) {
        // Set headers
        const RequestHeader<Method::GET>& request_header = request.header();
        set_headers(request_header.fields());

        // No body content

        // Make request
        auto response = handle_.GET(url.str());

        // Handle response
        return to_response(response);
    }

    Response perform(const URL& url, const Request<Method::POST>& request) {
        // Set headers
        const RequestHeader<Method::POST>& request_header = request.header();
        set_headers(request_header.fields());

        // Set body
        const Body& request_body = request.body();
        const auto& request_data = request_body.value();

        // Make request
        auto response = handle_.POST(url.str(), request_data);

        // Handle response
        return to_response(response);
    }

    Response perform(const URL& url, const Request<Method::PUT>& request) {
        // Set headers
        const RequestHeader<Method::PUT>& request_header = request.header();
        set_headers(request_header.fields());

        // Set body
        const Body& request_body = request.body();
        const auto& request_data = request_body.value();

        // Make request

        auto response = handle_.PUT(url.str(), request_data);

        // Handle response
        return to_response(response);
    }

private:
    void set_headers(const Fields& fields) {
        eckit::EasyCURLHeaders fieldx;
        for (const auto& field : fields) {
            fieldx[field.name] = field.value;
        }

        handle_.headers(fieldx);
    }

    void set_verifyhost(bool flag = true) { handle_.sslVerifyHost(flag); }
    void set_verifypeer(bool flag = true) { handle_.sslVerifyPeer(flag); }

    static Response to_response(const eckit::EasyCURLResponse& response) {
        Fields fields;
        for (const auto& entry : response.headers()) {
            fields.insert(Field{entry.first, entry.second});
        }
        auto response_header = ResponseHeader(Status::from_value(response.code()), fields);
        auto response_body   = Body{response.body()};

        return Response{response_header, response_body};
    }

private:
    eckit::EasyCURL handle_;
};

template <Method METHOD>
Response handle_request(const Host& host, const Request<METHOD>& request) {
    detail::Handle curl;
    auto url = URL{host, request.header().target()};

    return curl.perform(url, request);
}

}  // namespace detail


Response TinyRESTClient::handle(const Host& host, const Request<Method::GET>& request) const {
    return detail::handle_request(host, request);
}

Response TinyRESTClient::handle(const Host& host, const Request<Method::POST>& request) const {
    return detail::handle_request(host, request);
}

Response TinyRESTClient::handle(const Host& host, const Request<Method::PUT>& request) const {
    return detail::handle_request(host, request);
}

}  // namespace net

}  // namespace ecflow::light
