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
#include <sstream>
#include <vector>

#include "ecflow/light/TinyCURL.hpp"

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct ClientCfg {

    static ClientCfg make_empty() { return ClientCfg{}; }

    static ClientCfg make_phony(std::string task_rid, std::string task_name, std::string task_password,
                                std::string task_try_no) {
        return ClientCfg{std::string(KindPhony), std::string(ProtocolNone), std::string(),
                         std::string(),          std::string("1.0"),        std::move(task_rid),
                         std::move(task_name),   std::move(task_password),  std::move(task_try_no)};
    }

    static ClientCfg make_cfg(std::string kind, std::string protocol, std::string host, std::string port,
                              std::string version, std::string task_rid, std::string task_name,
                              std::string task_password, std::string task_try_no) {
        return ClientCfg{std::move(kind),      std::move(protocol),      std::move(host),
                         std::move(port),      std::move(version),       std::move(task_rid),
                         std::move(task_name), std::move(task_password), std::move(task_try_no)};
    }

private:
    ClientCfg() :
        kind(), protocol(), host(), port(), version(), task_rid(), task_name(), task_password(), task_try_no() {}

    ClientCfg(std::string kind, std::string protocol, std::string host, std::string port, std::string version,
              std::string task_rid, std::string task_name, std::string task_password, std::string task_try_no) :
        kind(std::move(kind)),
        protocol(std::move(protocol)),
        host(std::move(host)),
        port(std::move(port)),
        version(std::move(version)),
        task_rid(std::move(task_rid)),
        task_name(std::move(task_name)),
        task_password(std::move(task_password)),
        task_try_no(std::move(task_try_no)) {}

public:
    std::string kind;
    std::string protocol;
    std::string host;
    std::string port;
    std::string version;

    std::string task_rid;
    std::string task_name;
    std::string task_password;
    std::string task_try_no;

    static constexpr const char* ProtocolHTTP = "http";
    static constexpr const char* ProtocolUDP  = "udp";
    static constexpr const char* ProtocolTCP  = "tcp";
    static constexpr const char* ProtocolNone = "none";

    static constexpr const char* KindLibrary = "library";
    static constexpr const char* KindCLI     = "cli";
    static constexpr const char* KindPhony   = "phony";
};

struct Configuration {
    std::vector<ClientCfg> clients;

    static Configuration make_cfg();
};


// *** Client ******************************************************************
// *****************************************************************************

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    virtual void update_meter(const std::string& name, int value) const                = 0;
    virtual void update_label(const std::string& name, const std::string& value) const = 0;
    virtual void update_event(const std::string& name, bool value) const               = 0;
};

// *** Client (Phony) **********************************************************
// *****************************************************************************

class PhonyClientAPI : public ClientAPI {
public:
    ~PhonyClientAPI() override = default;

    void update_meter(const std::string& name, int value) const override;
    void update_label(const std::string& name, const std::string& value) const override;
    void update_event(const std::string& name [[maybe_unused]], bool value) const override;
};

// *** Client (Composite) **********************************************************
// *****************************************************************************

class CompositeClientAPI : public ClientAPI {
public:
    ~CompositeClientAPI() override = default;

    void add(std::unique_ptr<ClientAPI>&& api);

    void update_meter(const std::string& name, int value) const override;
    void update_label(const std::string& name, const std::string& value) const override;
    void update_event(const std::string& name, bool value) const override;

private:
    std::vector<std::unique_ptr<ClientAPI>> apis_;
};

// *** Client (CLI) **********************************************************
// *****************************************************************************

struct CLIDispatcher {
    static void dispatch_request(const ClientCfg& cfg, const std::string& request);
};

struct CLIFormatter {
    template <typename T, std::enable_if_t<!std::is_same_v<bool, T>, bool> = true>
    static std::string format_request(const ClientCfg& cfg [[maybe_unused]], const std::string& command,
                                      const std::string& name, T value) {
        std::ostringstream oss;
        oss << R"(ecflow_client --)" << command << R"(=)" << name << R"( ")" << value << R"(" &)";
        return oss.str();
    }

    template <typename T, std::enable_if_t<std::is_same_v<bool, T>, bool> = true>
    static std::string format_request(const ClientCfg& cfg [[maybe_unused]], const std::string& command,
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
    static void dispatch_request(const ClientCfg& cfg, const std::string& request);

    static constexpr size_t UDPPacketMaximumSize = 65'507;
};

struct UDPFormatter {
    template <typename T>
    static std::string format_request(const ClientCfg& cfg, const std::string& command, const std::string& name,
                                      T value) {
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("method":"put",)"
                << R"("version":")" << cfg.version << R"(",)"
                << R"("header":)"
                << R"({)"
                    << R"("task_rid":")" << cfg.task_rid << R"(",)"
                    << R"("task_password":")" << cfg.task_password << R"(",)"
                    << R"("task_try_no":)" << cfg.task_try_no
                << R"(},)"
                << R"("payload":)"
                << R"({)"
                    << R"("command":")" << command << R"(",)"
                    << R"("path":")" << cfg.task_name << R"(",)"
                    << R"("name":")" << name << R"(",)"
                    << R"("value":")"<< value << R"(")"
                << R"(})"
            << R"(})";
        // clang-format on
        return oss.str();
    }
};

// *** Client (HTTP) ************************************************************
// *****************************************************************************

#define NEW_CURL_API

struct HTTPDispatcher {
#if defined(NEW_CURL_API)
    static void dispatch_request(const ClientCfg& cfg, const net::Request& request);
#else
    static void dispatch_request(const ClientCfg& cfg, const std::string& request);
#endif
};

struct HTTPFormatter {
#if defined(NEW_CURL_API)
    template <typename T>
    static net::Request format_request(const ClientCfg& cfg, const std::string& command, const std::string& name, T value){
#else
    template <typename T>
    static std::string format_request(const ClientCfg& cfg [[maybe_unused]], const std::string& command,
                                      const std::string& name, T value) {
#endif

        std::string actual_value;
    if constexpr (std::is_integral_v<T>) {
        if (command == "event") {
            actual_value = value ? "set" : "clear";
        }
        if (command == "meter") {
            actual_value = std::to_string(value);
        }
    }
    else {
        actual_value = value;
    }

    // Build body
    std::ostringstream oss;
    // clang-format off
        oss << R"({)"
                << R"("ECF_NAME":")" << cfg.task_name << R"(",)"
                << R"("ECF_PASS":")" << cfg.task_password << R"(",)"
                << R"("ECF_RID":")" << cfg.task_rid << R"(",)"
                << R"("ECF_TRYNO":")" << cfg.task_try_no << R"(",)"
                << R"("type":")" << command << R"(",)"
                << R"("name":")" << name << R"(",)"
                << R"("value":")" << actual_value << R"(")"
            << R"(})";
    // clang-format on
    auto body = oss.str();

#if defined(NEW_CURL_API)
    // Build URL
    std::ostringstream os;
    os << "https://"
       << "localhost"
       << ":" << cfg.port << "/v1/suites" << cfg.task_name << "/attributes";
    net::URL url(os.str());

    net::Request request{url, net::Method::PUT};
    request.add_header_field(net::Field{"Content-Type", "application/json"});
    request.add_header_field(net::Field{"Authorization", "Bearer justworks"});
    request.add_body(net::Body{body});
    return request;
#else
        return body;
#endif
}
};  // namespace ecflow::light

// *** Client (Common) *********************************************************
// *****************************************************************************

template <typename Dispatcher, typename Formatter>
class BaseClientAPI : public ClientAPI {
public:
    explicit BaseClientAPI(ClientCfg cfg) : cfg{std::move(cfg)} {}
    ~BaseClientAPI() override = default;

    void update_meter(const std::string& name, int value) const override {
        Dispatcher::dispatch_request(cfg, Formatter::format_request(cfg, "meter", name, value));
    }
    void update_label(const std::string& name, const std::string& value) const override {
        Dispatcher::dispatch_request(cfg, Formatter::format_request(cfg, "label", name, value));
    }
    void update_event(const std::string& name, bool value) const override {
        Dispatcher::dispatch_request(cfg, Formatter::format_request(cfg, "event", name, value));
    }

private:
    ClientCfg cfg;
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

    void update_meter(const std::string& name, int value) const override;
    void update_label(const std::string& name, const std::string& value) const override;
    void update_event(const std::string& name, bool value) const override;

private:
    ConfiguredClient();

    CompositeClientAPI clients_;
    mutable std::mutex lock_;
};

}  // namespace ecflow::light

#endif
