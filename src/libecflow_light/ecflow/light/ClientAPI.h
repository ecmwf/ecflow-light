/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_CLIENTAPI_H
#define ECFLOW_LIGHT_CLIENTAPI_H

#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <eckit/exception/Exceptions.h>

#include "ecflow/light/Configuration.h"
#include "ecflow/light/Exception.h"
#include "ecflow/light/Log.h"
#include "ecflow/light/Requests.h"

namespace ecflow::light {

// *** Exceptions **************************************************************
// *****************************************************************************

struct InvalidRequest : public eckit::Exception {
    InvalidRequest(const std::string& msg, const eckit::CodeLocation& loc) : eckit::Exception(msg, loc) {}
};

// *** Client ******************************************************************
// *****************************************************************************

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    [[nodiscard]] virtual Response process(const Request& request) const = 0;
};

// *** Client (Phony) **********************************************************
// *****************************************************************************

class PhonyClientAPI : public ClientAPI {
public:
    ~PhonyClientAPI() override = default;

    [[nodiscard]] Response process(const Request& request) const override;
};

// *** Client (Composite) **********************************************************
// *****************************************************************************

class CompositeClientAPI : public ClientAPI {
public:
    ~CompositeClientAPI() override = default;

    void add(std::unique_ptr<ClientAPI>&& api);

    [[nodiscard]] Response process(const Request& request) const override;

private:
    std::vector<std::unique_ptr<ClientAPI>> apis_;
};

// *** Client Dispatch (Common) ************************************************
// *****************************************************************************

template <typename DISPATCHER>
class BaseRequestDispatcher : public RequestDispatcher {
public:
    explicit BaseRequestDispatcher(const ClientCfg& cfg) : cfg_{cfg}, response_{} {}

    Response call_dispatch(const Request& request) {
        DISPATCHER dispatcher{cfg_};
        request.dispatch(dispatcher);
        return response_;
    }

protected:
    const ClientCfg& cfg_;
    Response response_;
};

// *** Client Dispatch (CLI) ***************************************************
// *****************************************************************************

struct CLIDispatcher : public BaseRequestDispatcher<CLIDispatcher> {
public:
    explicit CLIDispatcher(const ClientCfg& cfg [[maybe_unused]]) :
        BaseRequestDispatcher<CLIDispatcher>(cfg), response_{} {}

    void dispatch_request(const UpdateNodeStatus& request [[maybe_unused]]) override {
        ECFLOW_LIGHT_THROW(NotImplemented, Message("CLIDispatcher::dispatch(const UpdateNodeStatus&) not supported"));
    }

    void dispatch_request(const UpdateNodeAttribute& request) override {
        std::ostringstream oss;
        oss << R"(ecflow_client --)" << request.options().get("command").value << R"(=)"
            << request.options().get("name").value << R"( ")" << request.options().get("value").value << R"(" &)";

        response_ = CLIDispatcher::exchange_request(cfg_, oss.str());
    };

private:
    static Response exchange_request(const ClientCfg& cfg, const std::string& request);

private:
    Response response_;
};

// *** Client (UDP Dispatch) ***************************************************
// *****************************************************************************

class UDPDispatcher : public BaseRequestDispatcher<UDPDispatcher> {
public:
    explicit UDPDispatcher(const ClientCfg& cfg) : BaseRequestDispatcher<UDPDispatcher>(cfg) {}

    void dispatch_request(const UpdateNodeStatus& request [[maybe_unused]]) override {
        ECFLOW_LIGHT_THROW(NotImplemented, Message("UDPDispatcher::dispatch(const UpdateNodeStatus&) not supported"));
    }

    void dispatch_request(const UpdateNodeAttribute& request) override {
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

        response_ = UDPDispatcher::exchange_request(cfg_, oss.str());
    };

private:
    static Response exchange_request(const ClientCfg& cfg, const std::string& request);

    static constexpr size_t UDPPacketMaximumSize = 65'507;
};

// *** Client (HTTP dispatch) **************************************************
// *****************************************************************************

class HTTPDispatcher : public BaseRequestDispatcher<HTTPDispatcher> {
public:
    explicit HTTPDispatcher(const ClientCfg& cfg) : BaseRequestDispatcher<HTTPDispatcher>(cfg) {}

    void dispatch_request(const UpdateNodeStatus& request) override {
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
        // Build Target
        std::ostringstream os;
        os << "/v1/suites" << request.environment().get("ECF_NAME").value << "/status";
        net::Target target{os.str()};

        net::Request<net::Method::PUT> low_level_request{target};
        low_level_request.add_header_field(net::Field{"Accept", "application/json"});
        low_level_request.add_header_field(net::Field{"Content-Type", "application/json"});
        low_level_request.add_header_field(net::Field{"charsets", "utf-8"});
        low_level_request.add_header_field(net::Field{"Authorization", "Bearer justworks"});
        // TODO: must take the token from configuration
        low_level_request.add_body(net::Body{body});

        response_ = HTTPDispatcher::exchange_request(cfg_, low_level_request);
    };

    void dispatch_request(const UpdateNodeAttribute& request) override {
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
        // Build Target
        std::ostringstream os;
        os << "/v1/suites" << request.environment().get("ECF_NAME").value << "/attributes";
        net::Target target{os.str()};

        net::Request<net::Method::PUT> low_level_request{target};
        low_level_request.add_header_field(net::Field{"Accept", "application/json"});
        low_level_request.add_header_field(net::Field{"Content-Type", "application/json"});
        low_level_request.add_header_field(net::Field{"charsets", "utf-8"});
        low_level_request.add_header_field(net::Field{"Authorization", "Bearer justworks"});
        // TODO: must take the token from configuration
        low_level_request.add_body(net::Body{body});

        response_ = HTTPDispatcher::exchange_request(cfg_, low_level_request);
    };

private:
    template <net::Method METHOD>
    static Response exchange_request(const ClientCfg& cfg, const net::Request<METHOD>& request) {
        net::Host host{cfg.host, cfg.port};

        Log::info() << "Dispatching HTTP Request: " << request.body().value() << " to host: " << host.str()
                    << " and target: " << request.header().target().str() << std::endl;

        net::TinyRESTClient rest;
        net::Response response = rest.handle(host, request);

        Log::info() << "Collected HTTP Response: "
                    << static_cast<std::underlying_type_t<net::Status::Code>>(response.header().status()) << std::endl;

        return Response{"OK"};
    }

private:
    Response response_;
};

// *** Client (Common) *********************************************************
// *****************************************************************************

template <typename Dispatcher>
class BaseClientAPI : public ClientAPI {
public:
    explicit BaseClientAPI(ClientCfg cfg, Environment env) : cfg{std::move(cfg)}, env{std::move(env)} {};
    ~BaseClientAPI() override = default;

    [[nodiscard]] Response process(const Request& request) const override {
        //        return Dispatcher::dispatch(cfg, request)
        Dispatcher dispatcher{cfg};
        return dispatcher.call_dispatch(request);
    }

private:
    ClientCfg cfg;
    Environment env;
};

using LibraryHTTPClientAPI    = BaseClientAPI<HTTPDispatcher>;
using LibraryUDPClientAPI     = BaseClientAPI<UDPDispatcher>;
using CommandLineTCPClientAPI = BaseClientAPI<CLIDispatcher>;

// *** Configured Client *******************************************************
// *****************************************************************************

class ConfiguredClient : public ClientAPI {
public:
    static ConfiguredClient& instance() {
        static ConfiguredClient theInstance;
        return theInstance;
    }

    [[nodiscard]] Response process(const Request& request) const override;

private:
    ConfiguredClient();

    CompositeClientAPI clients_;
    mutable std::mutex lock_;
};

}  // namespace ecflow::light

#endif
