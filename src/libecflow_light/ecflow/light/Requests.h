/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_REQUESTS_H
#define ECFLOW_LIGHT_REQUESTS_H

#include "ecflow/light/Configuration.h"
#include "ecflow/light/Environment.h"
#include "ecflow/light/Options.h"
#include "ecflow/light/TinyREST.h"

namespace ecflow::light {

struct RequestBuilder;

// *** Request(s)***************************************************************
// *****************************************************************************

struct RequestMessage {
    virtual ~RequestMessage() = default;

    [[nodiscard]] virtual const Environment& environment() const = 0;
    [[nodiscard]] virtual const Options& options() const         = 0;

    virtual std::string description() = 0;

    virtual void construct(RequestBuilder& builder) const = 0;
};

template <typename R>
struct DefaultRequestMessage : public RequestMessage {

    DefaultRequestMessage() : environment_{}, options_{} {}
    DefaultRequestMessage(Environment environment, Options options) :
        environment_{std::move(environment)}, options_{std::move(options)} {}

    [[nodiscard]] const Environment& environment() const final { return environment_; };
    [[nodiscard]] const Options& options() const final { return options_; };

    [[nodiscard]] std::string description() final { return static_cast<R*>(this)->as_string(); }

    void construct(RequestBuilder& builder) const final { return static_cast<const R*>(this)->call_construct(builder); }

private:
    Environment environment_;
    Options options_;
};

struct UpdateNodeStatus : DefaultRequestMessage<UpdateNodeStatus> {

    UpdateNodeStatus() : DefaultRequestMessage<UpdateNodeStatus> {}
    {}
    UpdateNodeStatus(Environment environment, Options options) : DefaultRequestMessage<UpdateNodeStatus> {
        std::move(environment), std::move(options)
    }
    {}

    [[nodiscard]] std::string as_string() const {
        return Message("UpdateNodeStatus: new_status=?, at node=", environment().get("ECF_NAME").value).str();
    }

    void call_construct(RequestBuilder& builder) const;
};

struct UpdateNodeAttribute : DefaultRequestMessage<UpdateNodeAttribute> {

    UpdateNodeAttribute() : DefaultRequestMessage<UpdateNodeAttribute> {}
    {}
    UpdateNodeAttribute(Environment environment, Options options) : DefaultRequestMessage<UpdateNodeAttribute> {
        std::move(environment), std::move(options)
    }
    {}

    [[nodiscard]] std::string as_string() const {
        return Message("UpdateNodeAttribute: name=", options().get("name").value,
                       ", value=", options().get("value").value, ", at node=", environment().get("ECF_NAME").value)
            .str();
    }

    void call_construct(RequestBuilder& builder) const;
};

struct RequestBuilder {
    virtual ~RequestBuilder() = default;

    virtual void construct(const UpdateNodeStatus& request)    = 0;
    virtual void construct(const UpdateNodeAttribute& request) = 0;
};

struct UDPRequestBuilder : public RequestBuilder {
    using formatted_request_t = std::string;

    explicit UDPRequestBuilder(const ClientCfg& cfg) : cfg_{cfg}, request_{} {}

    void construct(const UpdateNodeStatus& request [[maybe_unused]]) override {
        ECFLOW_LIGHT_THROW(NotImplemented,
                           Message("UDPRequestBuilder::construct(const UpdateNodeStatus&) not supported"));
    }

    void construct(const UpdateNodeAttribute& request) override {
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("method":"put",)"
                << R"("version":")" << cfg_.version << R"(",)"
                << R"("header":)"
                << R"({)"
                    << R"("task_rid":")" << request.environment().get("ECF_RID").value << R"(",)"
                    << R"("task_password":")" << request.environment().get("ECF_PASS").value << R"(",)"
                    << R"("task_try_no":)" << request.environment().get("ECF_TRYNO").value
                << R"(},)"
                << R"("payload":)"
                << R"({)"
                    << R"("command":")" << request.options().get("command").value << R"(",)"
                    << R"("path":")" << request.environment().get("ECF_NAME").value << R"(",)"
                    << R"("name":")" << request.options().get("name").value << R"(",)"
                    << R"("value":")"<< request.options().get("value").value << R"(")"
                << R"(})"
            << R"(})";
        // clang-format on
        request_ = oss.str();
    };

    [[nodiscard]] const formatted_request_t& request() const { return request_; }

private:
    const ClientCfg& cfg_;
    formatted_request_t request_;
};

class HTTPRequestBuilder : public RequestBuilder {
public:
    using formatted_request_t = net::Request;

    explicit HTTPRequestBuilder(const ClientCfg& cfg) :
        cfg_{cfg}, request_{net::URL{"https://localhost"}, net::Method::GET} {}

    void construct(const UpdateNodeStatus& request) override {
        // Build body
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("ECF_NAME":")" << request.environment().get("ECF_NAME").value << R"(",)"
                << R"("ECF_PASS":")" << request.environment().get("ECF_PASS").value << R"(",)"
                << R"("ECF_RID":")" << request.environment().get("ECF_RID").value << R"(",)"
                << R"("ECF_TRYNO":")" << request.environment().get("ECF_TRYNO").value << R"(",)"
                << R"("action":")" << request.options().get("action").value << R"(")";
                if(request.options().get("action").value == "abort") {
                    oss << R"(,"abort_why":")" << request.options().get("abort_why").value << R"(")";
                } else if(request.options().get("action").value == "wait") {
                    oss << R"(,"wait_expression":")" << request.options().get("wait_expression").value << R"(")";
                }
        oss << R"(})";
        // clang-format on
        auto body = oss.str();
        // Build URL
        std::ostringstream os;
        os << "https://" << cfg_.host << ":" << cfg_.port << "/v1/suites" << request.environment().get("ECF_NAME").value
           << "/status";
        net::URL url(os.str());

        request_ = net::Request{url, net::Method::PUT};
        request_.add_header_field(net::Field{"Content-Type", "application/json"});
        request_.add_header_field(net::Field{"Authorization", "Bearer justworks"});
        // TODO: must take the token from configuration
        request_.add_body(net::Body{body});
    };

    void construct(const UpdateNodeAttribute& request) override {
        // Build body
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("ECF_NAME":")" << request.environment().get("ECF_NAME").value << R"(",)"
                << R"("ECF_PASS":")" << request.environment().get("ECF_PASS").value << R"(",)"
                << R"("ECF_RID":")" << request.environment().get("ECF_RID").value << R"(",)"
                << R"("ECF_TRYNO":")" << request.environment().get("ECF_TRYNO").value << R"(",)"
                << R"("type":")" << request.options().get("command").value << R"(",)"
                << R"("name":")" << request.options().get("name").value << R"(",)"
                << R"("value":")" << request.options().get("value").value << R"(")"
            << R"(})";
        // clang-format on
        auto body = oss.str();
        // Build URL
        std::ostringstream os;
        os << "https://" << cfg_.host << ":" << cfg_.port << "/v1/suites" << request.environment().get("ECF_NAME").value
           << "/attributes";
        net::URL url(os.str());

        request_ = net::Request{url, net::Method::PUT};
        request_.add_header_field(net::Field{"Content-Type", "application/json"});
        request_.add_header_field(net::Field{"Authorization", "Bearer justworks"});
        // TODO: must take the token from configuration
        request_.add_body(net::Body{body});
    };

    const formatted_request_t& request() { return request_; }

private:
    const ClientCfg& cfg_;
    formatted_request_t request_;
};

struct CLIRequestBuilder : public RequestBuilder {
    using formatted_request_t = std::string;

    explicit CLIRequestBuilder(const ClientCfg& cfg [[maybe_unused]]) : request_{} {}

    void construct(const UpdateNodeStatus& request [[maybe_unused]]) override {
        ECFLOW_LIGHT_THROW(NotImplemented,
                           Message("CLIRequestBuilder::construct(const UpdateNodeStatus&) not supported"));
    }

    void construct(const UpdateNodeAttribute& request) override {
        std::ostringstream oss;
        oss << R"(ecflow_client --)" << request.options().get("command").value << R"(=)"
            << request.options().get("name").value << R"( ")" << request.options().get("value").value << R"(" &)";
        request_ = oss.str();
    };

    [[nodiscard]] const formatted_request_t& request() const { return request_; }

private:
    formatted_request_t request_;
};

struct Request final {
public:
    template <typename M, typename... ARGS>
    static Request make_request(ARGS&&... args) {
        return Request{std::make_unique<M>(std::forward<ARGS>(args)...)};
    }

    [[nodiscard]] std::string description() const { return message_->description(); }

    [[nodiscard]] std::string get_environment(const std::string& name) const {
        return message_->environment().get(name).value;
    }
    [[nodiscard]] std::string get_option(const std::string& name) const { return message_->options().get(name).value; }

    void construct(RequestBuilder& builder) const { return message_->construct(builder); }

private:
    explicit Request(std::unique_ptr<RequestMessage>&& message) : message_{std::move(message)} {}

    std::unique_ptr<RequestMessage> message_;
};

// *** Response(s) *************************************************************
// *****************************************************************************

struct Response final {
    std::string response;
};

std::ostream& operator<<(std::ostream& o, const Response& response);

}  // namespace ecflow::light

#endif
