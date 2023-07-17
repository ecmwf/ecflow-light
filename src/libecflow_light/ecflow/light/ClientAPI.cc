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
#include "ecflow/light/TinyCURL.hpp"
#include "ecflow/light/TinyREST.hpp"

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

static std::string replace_env_var(const std::string& value) {
    static std::regex regex(R"(\$ENV\{([^}]*)\})");
    std::smatch match;
    if (bool found = std::regex_match(value, match, regex); found) {
        std::string name = match[1];
        if (std::optional<Variable> variable = Environment0::get_variable(name.c_str()); variable) {
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
    //  - Check Optional variables
    auto variable     = Environment0::get_variable("NO_ECF", "NO_SMS", "NOECF", "NOSMS");
    bool skip_clients = variable.has_value();
    if (skip_clients) {
        Log::warning()
            << Message("'", variable->variable_name, "' environment variable detected. Configuring Phony client.").str()
            << std::endl;

        cfg.clients.push_back(ClientCfg::make_phony());
        return cfg;
    }

    // Load Configuration from YAML
    if (auto yaml_cfg_file = Environment0::get_variable("IFS_ECF_CONFIG_PATH"); yaml_cfg_file) {
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

            cfg.clients.push_back(ClientCfg::make_cfg(kind, protocol, host, port, version));

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

Response PhonyClientAPI::process(const Request& request) const {
    Log::info() << "Dispatching Phony Request: '" << request.str() << std::endl;
    return Response{"OK"};
};

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

Response CompositeClientAPI::process(const Request& request) const {
    std::vector<Response> responses;
    std::for_each(std::begin(apis_), std::end(apis_), [&](const auto& api) {
        Response response = api->process(request);
        responses.push_back(response);
    });
    if (responses.empty()) {
        throw std::runtime_error("No Responses available");
    }
    return responses.back();  // TODO: What should happen in this case?!
};

// *** Client (CLI) ************************************************************
// *****************************************************************************

Response CLIDispatcher::dispatch_request(const ClientCfg& cfg [[maybe_unused]], const std::string& request) {
    Log::info() << "Dispatching CLI Request: " << request << std::endl;
    ::system(request.c_str());

    return Response{"OK"};
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

Response UDPDispatcher::dispatch_request(const ClientCfg& cfg, const std::string& request) {

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
};

// *** Client (HTTP) ************************************************************
// *****************************************************************************

#if defined(NEW_CURL_API)
Response HTTPDispatcher::dispatch_request(const ClientCfg& cfg, const net::Request& request) {

    //    Log::info() << "Dispatching HTTP Request: " << request << ", to " << cfg.host << ":" << cfg.port << std::endl;
    Log::info() << "Dispatching HTTP Request: " << request.body().value() << ", to " << cfg.host << ":" << cfg.port
                << std::endl;

    net::TinyRESTClient rest;
    net::Response response = rest.handle(request);

    Log::info() << "Collected HTTP Response: "
                << static_cast<std::underlying_type_t<net::Status::Code>>(response.header().status()) << std::endl;

    return Response{"OK"};
};
#else
void HTTPDispatcher::dispatch_request(const ClientCfg& cfg, const std::string& request) {

    Log::info() << "Dispatching HTTP Request: " << request << ", to " << cfg.host << ":" << cfg.port << std::endl;

    // Build URL
    std::ostringstream os;
    os << "https://"
       << "localhost"
       << ":" << cfg.port << "/v1/suites" << cfg.task_name << "/attributes";
    URL url(os.str());

    TinyCURL curl;
    curl.put(url, request);
}
#endif

// *** Configured Client *******************************************************
// *****************************************************************************

ConfiguredClient::ConfiguredClient() : clients_{}, lock_{} {
    Configuration cfg = Configuration::make_cfg();

    Environment environment = Environment::load();

    // Setup configured API based on the configuration
    if (cfg.clients.empty()) {
        Log::warning() << "No Clients registered";
    }
    else {
        for (const auto& client : cfg.clients) {
            if (client.kind == ClientCfg::KindLibrary && client.protocol == ClientCfg::ProtocolUDP) {
                Log::debug() << "Library (UDP) Client registered" << std::endl;
                clients_.add(std::make_unique<LibraryUDPClientAPI>(client, environment));
            }
            else if (client.kind == ClientCfg::KindLibrary && client.protocol == ClientCfg::ProtocolHTTP) {
                Log::debug() << "Library (HTTP) Client registered" << std::endl;
                clients_.add(std::make_unique<LibraryHTTPClientAPI>(client, environment));
            }
            else if (client.kind == ClientCfg::KindCLI && client.protocol == ClientCfg::ProtocolTCP) {
                Log::debug() << "CLI (TCP) Client registered" << std::endl;
                clients_.add(std::make_unique<CommandLineTCPClientAPI>(client, environment));
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

Response ConfiguredClient::process(const Request& request) const {
    std::scoped_lock lock(lock_);
    return clients_.process(request);
}

}  // namespace ecflow::light
