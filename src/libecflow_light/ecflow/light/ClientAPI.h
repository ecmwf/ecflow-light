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
#include "ecflow/light/TinyCURL.hpp"

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

// *** Client (CLI) **********************************************************
// *****************************************************************************

struct CLIDispatcher {
    static Response dispatch_request(const ClientCfg& cfg, const std::string& request);
};

struct CLIFormatter {
    static std::string format_request(const ClientCfg& cfg [[maybe_unused]], const Request& request) {
        std::ostringstream oss;
        oss << R"(ecflow_client --)" << request.get_option("command") << R"(=)" << request.get_option("name") << R"( ")"
            << request.get_option("value") << R"(" &)";
        return oss.str();
    }

    template <typename T, std::enable_if_t<!std::is_same_v<bool, T>, bool> = true>
    static std::string format_request(const ClientCfg& cfg [[maybe_unused]],
                                      const Environment& environment [[maybe_unused]], const std::string& command,
                                      const std::string& name, T value) {
        std::ostringstream oss;
        oss << R"(ecflow_client --)" << command << R"(=)" << name << R"( ")" << value << R"(" &)";
        return oss.str();
    }

    template <typename T, std::enable_if_t<std::is_same_v<bool, T>, bool> = true>
    static std::string format_request(const ClientCfg& cfg [[maybe_unused]],
                                      const Environment& environment [[maybe_unused]], const std::string& command,
                                      const std::string& name, T value) {

        std::string parameter = value ? "set" : "clear";

        std::ostringstream oss;
        oss << R"(ecflow_client --)" << command << R"(=)" << name << R"( ")" << parameter << R"(" &)";
        return oss.str();
    }
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

struct UDPDispatcher {
    static Response dispatch_request(const ClientCfg& cfg, const std::string& request);

    static constexpr size_t UDPPacketMaximumSize = 65'507;
};

struct UDPFormatter {
    static std::string format_request(const ClientCfg& cfg, const Request& request) {
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("method":"put",)"
                << R"("version":")" << cfg.version << R"(",)"
                << R"("header":)"
                << R"({)"
                    << R"("task_rid":")" << request.get_environment("ECF_RID") << R"(",)"
                    << R"("task_password":")" << request.get_environment("ECF_PASS") << R"(",)"
                    << R"("task_try_no":)" << request.get_environment("ECF_TRYNO")
                << R"(},)"
                << R"("payload":)"
                << R"({)"
                    << R"("command":")" << request.get_option("command") << R"(",)"
                    << R"("path":")" << request.get_environment("ECF_NAME") << R"(",)"
                    << R"("name":")" << request.get_option("name") << R"(",)"
                    << R"("value":")"<< request.get_option("value") << R"(")"
                << R"(})"
            << R"(})";
        // clang-format on
        return oss.str();
    }
};

// *** Client (HTTP) ************************************************************
// *****************************************************************************

struct HTTPDispatcher {
    static Response dispatch_request(const ClientCfg& cfg, const net::Request& request);
};

struct HTTPFormatter {
    static net::Request format_request(const ClientCfg& cfg, const Request& request) {

        // Build body
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("ECF_NAME":")" << request.get_environment("ECF_NAME") << R"(",)"
                << R"("ECF_PASS":")" << request.get_environment("ECF_PASS") << R"(",)"
                << R"("ECF_RID":")" << request.get_environment("ECF_RID") << R"(",)"
                << R"("ECF_TRYNO":")" << request.get_environment("ECF_TRYNO") << R"(",)"
                << R"("type":")" << request.get_option("command") << R"(",)"
                << R"("name":")" << request.get_option("name") << R"(",)"
                << R"("value":")" << request.get_option("value") << R"(")"
            << R"(})";
        // clang-format on
        auto body = oss.str();
        // Build URL
        std::ostringstream os;
        os << "https://"
           << "localhost"
           << ":" << cfg.port << "/v1/suites" << request.get_environment("ECF_NAME") << "/attributes";
        net::URL url(os.str());

        net::Request http_request{url, net::Method::PUT};
        http_request.add_header_field(net::Field{"Content-Type", "application/json"});
        http_request.add_header_field(net::Field{"Authorization", "Bearer justworks"});
        http_request.add_body(net::Body{body});
        return http_request;
    }

};  // namespace ecflow::light

// *** Client (Common) *********************************************************
// *****************************************************************************

template <typename Dispatcher, typename Formatter>
class BaseClientAPI : public ClientAPI {
public:
    explicit BaseClientAPI(ClientCfg cfg, Environment env) : cfg{std::move(cfg)}, env{std::move(env)} {};
    ~BaseClientAPI() override = default;

    [[nodiscard]] Response process(const Request& request) const override {
        return Dispatcher::dispatch_request(cfg, Formatter::format_request(cfg, request));
    }

private:
    ClientCfg cfg;
    Environment env;
};

using LibraryHTTPClientAPI    = BaseClientAPI<HTTPDispatcher, HTTPFormatter>;
using LibraryUDPClientAPI     = BaseClientAPI<UDPDispatcher, UDPFormatter>;
using CommandLineTCPClientAPI = BaseClientAPI<CLIDispatcher, CLIFormatter>;

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
