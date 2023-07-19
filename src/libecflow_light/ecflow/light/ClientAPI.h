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

#include "ecflow/light/Exception.h"
#include "ecflow/light/Log.h"
#include "ecflow/light/TinyCURL.hpp"

namespace ecflow::light {

struct Variable {
    using name_t  = std::string;
    using value_t = std::string;

    name_t name;
    value_t value;
};

struct Environment0 {

    static std::optional<Variable> get_variable() { return {}; }

    template <typename S, typename... ARGS>
    static std::optional<Variable> get_variable(const S& variable_name, ARGS... other_variable_names) {
        if (auto variable = collect_variable(variable_name); variable) {
            return variable;
        }
        return get_variable(other_variable_names...);
    }

    static Variable get_mandatory_variable(const char* variable_name) {
        if (auto variable = collect_variable(variable_name); variable) {
            return variable.value();
        }
        else {
            ECFLOW_LIGHT_THROW(InvalidEnvironment, Message("NO ", variable_name, " available"));
        }
    }

private:
    static std::optional<Variable> collect_variable(const char* variable_name) {
        if (const char* variable_value = ::getenv(variable_name); variable_value) {
            return std::make_optional(Variable{variable_name, variable_value});
        }
        return {};
    }

    static std::optional<Variable> collect_variable(const std::string& variable_name) {
        if (const char* variable_value = ::getenv(variable_name.c_str()); variable_value) {
            return std::make_optional(Variable{variable_name, variable_value});
        }
        return {};
    }
};

struct Environment {
    using dict_t = std::unordered_map<Variable::name_t, Variable>;

    Environment() = default;

    static Environment an_environment() { return {}; }

    static const Environment& environment() {
        static Environment environment = Environment()
                                             .from_environment("ECF_NAME")
                                             .from_environment("ECF_PASS")
                                             .from_environment("ECF_RID")
                                             .from_environment("ECF_TRYNO")
                                             .from_environment("ECF_HOST")
                                             .from_environment("NO_ECF");
        return environment;
    }

    [[nodiscard]] Environment& from_environment(const Variable::name_t& variable_name) {
        std::optional<Variable> variable = Environment0::get_variable(variable_name);
        if (variable) {
            this->with(variable->name, variable->value);
        }
        return *this;
    }

    Environment& with(const Variable::name_t& name, const Variable::name_t& value) {
        environment_.insert_or_assign(name, Variable{name, value});
        return *this;
    }

    [[nodiscard]] const Variable& get(const Variable::name_t& name) const {
        auto found = environment_.find(name);
        if (found == std::end(environment_)) {
            throw std::runtime_error("No environment variable found");
        }
        return found->second;
    }

private:
    // information collected from the environment (e.g. environment variables)
    dict_t environment_;
};

struct Option {
    using name_t  = std::string;
    using value_t = std::string;

    name_t name;
    value_t value;
};

struct Options {

    using dict_t = std::unordered_map<std::string, Option>;

    Options() = default;

    static Options options() { return {}; };

    Options& with(const Option::name_t& name, const Option::value_t& value) {
        this->with(Option{name, value});
        return *this;
    }

    Options& with(const Option& option) {
        options_.insert_or_assign(option.name, option);
        return *this;
    }

    [[nodiscard]] const Option& get(const std::string& name) const {
        auto found = options_.find(name);
        if (found == std::end(options_)) {
            throw std::runtime_error("No option found");
        }
        return found->second;
    }

private:
    // information collected from the options (i.e. cli arguments)
    dict_t options_;
};

struct RequestMessage {
    virtual ~RequestMessage() = default;

    [[nodiscard]] virtual const Environment& environment() const = 0;
    [[nodiscard]] virtual const Options& options() const         = 0;

    virtual std::string str() = 0;
};

template <typename R>
struct DefaultRequestMessage : public RequestMessage {

    DefaultRequestMessage() : environment_{}, options_{} {}
    DefaultRequestMessage(Environment environment, Options options) :
        environment_{std::move(environment)}, options_{std::move(options)} {}

    [[nodiscard]] const Environment& environment() const final { return environment_; };
    [[nodiscard]] const Options& options() const final { return options_; };

    [[nodiscard]] std::string str() final { return static_cast<R*>(this)->as_string(); }

private:
    Environment environment_;
    Options options_;
};

struct CreateAttribute : DefaultRequestMessage<CreateAttribute> {

    CreateAttribute() : DefaultRequestMessage<CreateAttribute>{} {}
    CreateAttribute(Environment environment, Options options) :
        DefaultRequestMessage<CreateAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "CreateAttribute"; }
};

struct ReadAttribute : DefaultRequestMessage<ReadAttribute> {

    ReadAttribute() : DefaultRequestMessage<ReadAttribute>{} {}
    ReadAttribute(Environment environment, Options options) :
        DefaultRequestMessage<ReadAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "ReadAttribute"; }
};

struct UpdateAttribute : DefaultRequestMessage<UpdateAttribute> {

    UpdateAttribute() : DefaultRequestMessage<UpdateAttribute>{} {}
    UpdateAttribute(Environment environment, Options options) :
        DefaultRequestMessage<UpdateAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "UpdateAttribute"; }
};

struct DeleteAttribute : DefaultRequestMessage<DeleteAttribute> {

    DeleteAttribute() : DefaultRequestMessage<DeleteAttribute>{} {}
    DeleteAttribute(Environment environment, Options options) :
        DefaultRequestMessage<DeleteAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "DeleteAttribute"; }
};

struct Request final {
    template <typename M, typename... ARGS>
    static Request make_request(ARGS&&... args) {
        return Request{std::make_unique<M>(std::forward<ARGS>(args)...)};
    }

    [[nodiscard]] std::string str() const { return message_->str(); }

    [[nodiscard]] std::string get_environment(const std::string& name) const {
        return message_->environment().get(name).value;
    }
    [[nodiscard]] std::string get_option(const std::string& name) const { return message_->options().get(name).value; }

private:
    explicit Request(std::unique_ptr<RequestMessage>&& message) : message_{std::move(message)} {}

    std::unique_ptr<RequestMessage> message_;
};

struct Response final {
    std::string response;
};

std::ostream& operator<<(std::ostream& o, const Response& response);

// *** Configuration ***********************************************************
// *****************************************************************************

struct ClientCfg {

    static ClientCfg make_empty() { return ClientCfg{}; }

    static ClientCfg make_phony() {
        return ClientCfg{std::string(KindPhony), std::string(ProtocolNone), std::string(), std::string(),
                         std::string("1.0")};
    }

    static ClientCfg make_cfg(std::string kind, std::string protocol, std::string host, std::string port,
                              std::string version) {
        return ClientCfg{std::move(kind), std::move(protocol), std::move(host), std::move(port), std::move(version)};
    }

private:
    ClientCfg() : kind(), protocol(), host(), port(), version() {}

    ClientCfg(std::string kind, std::string protocol, std::string host, std::string port, std::string version) :
        kind(std::move(kind)),
        protocol(std::move(protocol)),
        host(std::move(host)),
        port(std::move(port)),
        version(std::move(version)) {}

public:
    std::string kind;
    std::string protocol;
    std::string host;
    std::string port;
    std::string version;

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
