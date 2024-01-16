/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/ClientAPI.h"

#include <cstdlib>
#include <memory>
#include <optional>
#include <regex>

#include <eckit/config/LocalConfiguration.h>
#include <eckit/config/YAMLConfiguration.h>
#include <eckit/exception/Exceptions.h>
#include <eckit/filesystem/PathName.h>
#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.h"
#include "ecflow/light/Exception.h"
#include "ecflow/light/Log.h"

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

std::ostream& operator<<(std::ostream& os, const ClientCfg& cfg) {
    os << R"({)";
    os << R"("kind":")" << cfg.kind << R"(",)";
    os << R"("protocol":")" << cfg.protocol << R"(",)";
    os << R"("host":")" << cfg.host << R"(",)";
    os << R"("port":")" << cfg.port << R"(",)";
    os << R"("version":")" << cfg.version << R"(")";
    // Omitting task specific configuration parameters
    os << R"(})";
    return os;
}

struct Variable {
    std::string variable_name;
    std::string variable_value;
};

struct Environment {

    static std::optional<Variable> get_variable() { return {}; }

    template <typename... ARGS>
    static std::optional<Variable> get_variable(const char* variable_name, ARGS... other_variable_names) {
        if (auto variable = collect_variable(variable_name); variable) {
            return variable;
        }
        return get_variable(other_variable_names...);
    }

    static std::string get_mandatory_variable(const char* variable_name) {
        if (auto variable = collect_variable(variable_name); variable) {
            return variable->variable_value;
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
};

static std::string replace_env_var(const std::string& value) {
    static std::regex regex(R"(\$ENV\{([^}]*)\})");
    std::smatch match;
    if (bool found = std::regex_match(value, match, regex); found) {
        std::string name = match[1];
        if (std::optional<Variable> variable = Environment::get_variable(name.c_str()); variable) {
            return variable->variable_value;
        }
        else {
            Log::warning() << Message("Environment variable '", name, "' not found. Replacement not possible...").str()
                           << std::endl;
        }
    }
    return value;
}

Configuration Configuration::make_cfg() {
    Configuration cfg{};

    // Load Environment Variables
    //  - Mandatory variables -- Important: will throw if not available!
    std::string task_rid      = Environment::get_mandatory_variable("ECF_RID");
    std::string task_name     = Environment::get_mandatory_variable("ECF_NAME");
    std::string task_password = Environment::get_mandatory_variable("ECF_PASS");
    std::string task_try_no   = Environment::get_mandatory_variable("ECF_TRYNO");

    //  - Optional variables
    auto variable     = Environment::get_variable("NO_ECF", "NO_SMS", "NOECF", "NOSMS");
    bool skip_clients = variable.has_value();
    if (skip_clients) {
        Log::warning()
            << Message("'", variable->variable_name, "' environment variable detected. Configuring Phony client.").str()
            << std::endl;

        cfg.clients.push_back(ClientCfg::make_phony(task_rid, task_name, task_password, task_try_no));
        return cfg;
    }

    // Load Configuration from YAML
    if (auto yaml_cfg_file = Environment::get_variable("IFS_ECF_CONFIG_PATH"); yaml_cfg_file) {
        // Attempt to use YAML configuration path, if provided
        Log::info() << "YAML defined by IFS_ECF_CONFIG_PATH: '" << yaml_cfg_file->variable_value << "'" << std::endl;
        eckit::YAMLConfiguration yaml_cfg{eckit::PathName(yaml_cfg_file->variable_value)};

        auto clients = yaml_cfg.getSubConfigurations("clients");
        for (const auto& client : clients) {

            auto get = [&client](const std::string& name, const std::string& default_value = std::string()) {
                std::string value = default_value;
                if (client.has(name)) {
                    client.get(name, value);
                }
                return value;
            };

            std::string kind     = get("kind");
            std::string protocol = get("protocol");
            std::string host     = get("host");
            std::string port     = get("port");
            std::string version  = get("version", "1.0");

            // Replace environment variables
            host = replace_env_var(host);
            port = replace_env_var(port);

            cfg.clients.push_back(ClientCfg::make_cfg(kind, protocol, host, port, version, task_rid, task_name,
                                                      task_password, task_try_no));

            Log::info() << "Client configuration: " << cfg.clients.back() << std::endl;
        }
    }
    else {
        ECFLOW_LIGHT_THROW(InvalidEnvironment,
                           Message("Unable to load YAML configuration as 'IFS_ECF_CONFIG_PATH' is not defined"));
    }

    return cfg;
}

// *** Client (Phony) **********************************************************
// *****************************************************************************

void PhonyClientAPI::update_meter(const std::string& name, int value) const {
    Log::info() << "Dispatching Phony Request: meter '" << name << "' set to '" << value << "'" << std::endl;
}
void PhonyClientAPI::update_label(const std::string& name, const std::string& value) const {
    Log::info() << "Dispatching Phony Request: label '" << name << "' set to '" << value << "'" << std::endl;
}
void PhonyClientAPI::update_event(const std::string& name, bool value) const {
    Log::info() << "Dispatching Phony Request: event '" << name << "' set to '" << value << "'" << std::endl;
}

// *** Client (Composite) **********************************************************
// *****************************************************************************

void CompositeClientAPI::add(std::unique_ptr<ClientAPI>&& api) {
    apis_.push_back(std::move(api));
}

void CompositeClientAPI::update_meter(const std::string& name, int value) const {
    std::for_each(std::begin(apis_), std::end(apis_), [&](const auto& api) { api->update_meter(name, value); });
}
void CompositeClientAPI::update_label(const std::string& name, const std::string& value) const {
    std::for_each(std::begin(apis_), std::end(apis_), [&](const auto& api) { api->update_label(name, value); });
}
void CompositeClientAPI::update_event(const std::string& name, bool value) const {
    std::for_each(std::begin(apis_), std::end(apis_), [&](const auto& api) { api->update_event(name, value); });
}

// *** Client (CLI) ************************************************************
// *****************************************************************************

void CLIDispatcher::dispatch_request(const ClientCfg& cfg [[maybe_unused]], const std::string& request) {
    Log::info() << "Dispatching CLI Request: " << request << std::endl;
    ::system(request.c_str());
}

// *** Client (UDP) ************************************************************
// *****************************************************************************

void UDPDispatcher::dispatch_request(const ClientCfg& cfg, const std::string& request) {

    Log::info() << "Dispatching UDP Request: " << request << ", to " << cfg.host << ":" << cfg.port << std::endl;

    const size_t packet_size = request.size() + 1;
    if (packet_size > UDPPacketMaximumSize) {
        ECFLOW_LIGHT_THROW(InvalidRequest, Message("Request too large. Maximum size expected is ", UDPPacketMaximumSize,
                                                   ", but found: ", packet_size));
    }

    int port = convert_to<int>(cfg.port);
    eckit::net::UDPClient client(cfg.host, port);
    client.send(request.data(), packet_size);
}

// *** Configured Client *******************************************************
// *****************************************************************************

ConfiguredClient::ConfiguredClient() : clients_{}, lock_{} {
    Configuration cfg = Configuration::make_cfg();

    // Setup configured API based on the configuration
    if (cfg.clients.empty()) {
        Log::warning() << "No Clients registered";
    }
    else {
        for (const auto& client : cfg.clients) {
            if (client.kind == ClientCfg::KindLibrary && client.protocol == ClientCfg::ProtocolUDP) {
                Log::debug() << "Library (UDP) Client registered" << std::endl;
                clients_.add(std::make_unique<LibraryUDPClientAPI>(client));
            }
            else if (client.kind == ClientCfg::KindCLI && client.protocol == ClientCfg::ProtocolTCP) {
                Log::debug() << "CLI (TCP) Client registered" << std::endl;
                clients_.add(std::make_unique<CommandLineTCPClientAPI>(client));
            }
            else if (client.kind == ClientCfg::KindPhony && client.protocol == ClientCfg::ProtocolNone) {
                Log::debug() << "(Phony) Client registered" << std::endl;
                clients_.add(std::make_unique<PhonyClientAPI>());
            }
            else {
                Log::error() << "Invalid client '" << client.kind << "' detected, using protocol '" << client.protocol
                             << "'. Ignored!..." << std::endl;
            }
        }
    }
}

void ConfiguredClient::update_meter(const std::string& name, int value) const {
    std::scoped_lock lock(lock_);
    clients_.update_meter(name, value);
}
void ConfiguredClient::update_label(const std::string& name, const std::string& value) const {
    std::scoped_lock lock(lock_);
    clients_.update_label(name, value);
}
void ConfiguredClient::update_event(const std::string& name, bool value) const {
    std::scoped_lock lock(lock_);
    clients_.update_event(name, value);
}

}  // namespace ecflow::light
