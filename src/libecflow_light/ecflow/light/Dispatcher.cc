/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/Dispatcher.h"

#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.h"
#include "ecflow/light/Exception.h"
#include "ecflow/light/Token.h"

namespace ecflow::light {

// *** Exceptions **************************************************************
// *****************************************************************************

struct InvalidRequest : public eckit::Exception {
    InvalidRequest(const std::string& msg, const eckit::CodeLocation& loc) : eckit::Exception(msg, loc) {}
};

// *** Client Dispatcher (CLI) *************************************************
// *****************************************************************************

CLIDispatcher::CLIDispatcher(const ClientCfg& cfg) : BaseRequestDispatcher<CLIDispatcher>(cfg) {}

void CLIDispatcher::dispatch_request(const UpdateNodeStatus& request [[maybe_unused]]) {
    ECFLOW_LIGHT_THROW(NotImplemented, Message("CLIDispatcher::dispatch(const UpdateNodeStatus&) not supported"));
}

void CLIDispatcher::dispatch_request(const UpdateNodeAttribute& request) {
    std::ostringstream oss;
    oss << R"(ecflow_client --)" << request.options().get("command").value << R"(=)"
        << request.options().get("name").value << R"( ")" << request.options().get("value").value << R"(" &)";

    response_ = CLIDispatcher::exchange_request(cfg_, oss.str());
}

Response CLIDispatcher::exchange_request(const ClientCfg& cfg [[maybe_unused]], const std::string& request) {
    Log::info() << "Dispatching CLI Request: " << request << std::endl;
    ::system(request.c_str());

    return Response{"OK"};
}

// *** Client Dispatcher (UDP) *************************************************
// *****************************************************************************

UDPDispatcher::UDPDispatcher(const ClientCfg& cfg) : BaseRequestDispatcher<UDPDispatcher>(cfg) {}

std::string UDPDispatcher::format_request(const UpdateNodeAttribute& request) const {
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
    return oss.str();
}

void UDPDispatcher::dispatch_request(const UpdateNodeStatus& request [[maybe_unused]]) {
    ECFLOW_LIGHT_THROW(NotImplemented, Message("UDPDispatcher::dispatch(const UpdateNodeStatus&) not supported"));
}

void UDPDispatcher::dispatch_request(const UpdateNodeAttribute& request) {
    auto contents = format_request(request);
    response_ = UDPDispatcher::exchange_request(cfg_, contents);
}

Response UDPDispatcher::exchange_request(const ClientCfg& cfg, const std::string& request) {
    Log::info() << "Dispatching UDP Request: " << request << ", to " << cfg.host << ":" << cfg.port << std::endl;

    const size_t packet_size = request.size() + 1;
    if (packet_size > UDPPacketMaximumSize) {
        ECFLOW_LIGHT_THROW(InvalidRequest, Message("Request too large. Maximum size expected is ", UDPPacketMaximumSize,
                                                   ", but found: ", packet_size));
    }

    int port = convert_to<int>(cfg.port);
    eckit::net::UDPClient client(cfg.host, port);
    client.send(request.data(), packet_size);

    return Response{"OK"};
}

// *** Client Dispatcher (HTTP) ************************************************
// *****************************************************************************

HTTPDispatcher::HTTPDispatcher(const ClientCfg& cfg) : BaseRequestDispatcher<HTTPDispatcher>(cfg) {}

void HTTPDispatcher::dispatch_request(const UpdateNodeStatus& request) {
    // Build body
    std::ostringstream oss;
    auto action = request.options().get("action").value;
    // clang-format off
        oss << R"({)"
                << R"("ECF_NAME":")" << request.environment().get("ECF_NAME").value << R"(",)"
                << R"("ECF_PASS":")" << request.environment().get("ECF_PASS").value << R"(",)"
                << R"("ECF_RID":")" << request.environment().get("ECF_RID").value << R"(",)"
                << R"("ECF_TRYNO":")" << request.environment().get("ECF_TRYNO").value << R"(",)"
                << R"("action":")" << action << R"(")";
                if(action == "abort") {
                    oss << R"(,"abort_why":")" << request.options().get("abort_why").value << R"(")";
                } else if(action == "wait") {
                    oss << R"(,"wait_expression":")" << request.options().get("wait_expression").value << R"(")";
                }
        oss << R"(})";
    // clang-format on
    auto body = oss.str();

    // Build Target
    auto target = net::Target{stringify("/v1/suites", request.environment().get("ECF_NAME").value, "/status")};

    net::Request<net::Method::PUT> low_level_request{target};
    low_level_request.add_header_field(net::Field{"Accept", "application/json"});
    low_level_request.add_header_field(net::Field{"Content-Type", "application/json"});
    low_level_request.add_header_field(net::Field{"charsets", "utf-8"});
    std::optional<Token> secret = Tokens().secret(net::URL(net::Host(cfg_.host, cfg_.port), net::Target("/v1")).str());
    if (secret) {
        low_level_request.add_header_field(net::Field{"Authorization", "Bearer " + secret.value().key});
    }
    low_level_request.add_body(net::Body{body});

    response_ = HTTPDispatcher::exchange_request(cfg_, low_level_request);
}

void HTTPDispatcher::dispatch_request(const UpdateNodeAttribute& request) {
    const auto& options     = request.options();
    const auto& environment = request.environment();

    // Build body
    auto type = request.options().get("command").value;
    std::ostringstream oss;
    // clang-format off
    oss << R"({)"
        << R"("ECF_NAME":")" << environment.get("ECF_NAME").value << R"(",)"
        << R"("ECF_PASS":")" << environment.get("ECF_PASS").value << R"(",)"
        << R"("ECF_RID":")" << environment.get("ECF_RID").value << R"(",)"
        << R"("ECF_TRYNO":")" << environment.get("ECF_TRYNO").value << R"(",)"
        << R"("type":")" << type << R"(",)"
        << R"("name":")" << options.get("name").value << R"(")";
    // clang-format on

    if (auto found = options.find_value("queue_action"); found) {
        oss << R"(,"queue_action":")" << found.value().value << R"(")";
    }
    if (auto found = options.find_value("queue_step"); found) {
        oss << R"(,"queue_step":")" << found.value().value << R"(")";
    }
    if (auto found = options.find_value("queue_path"); found) {
        oss << R"(,"queue_path":")" << found.value().value << R"(")";
    }
    if (auto found = options.find_value("value"); found) {
        oss << R"(,"value":")" << found.value().value << R"(")";
    }
    oss << R"(})";

    auto body = oss.str();

    // Build Target
    auto target = net::Target{stringify("/v1/suites", environment.get("ECF_NAME").value, "/attributes")};

    net::Request<net::Method::PUT> low_level_request{target};
    low_level_request.add_header_field(net::Field{"Accept", "application/json"});
    low_level_request.add_header_field(net::Field{"Content-Type", "application/json"});
    low_level_request.add_header_field(net::Field{"charsets", "utf-8"});
    std::optional<Token> secret = Tokens().secret(net::URL(net::Host(cfg_.host, cfg_.port), net::Target("/v1")).str());
    if (secret) {
        low_level_request.add_header_field(net::Field{"Authorization", "Bearer " + secret.value().key});
    }
    low_level_request.add_body(net::Body{body});

    response_ = HTTPDispatcher::exchange_request(cfg_, low_level_request);
}

}  // namespace ecflow::light
