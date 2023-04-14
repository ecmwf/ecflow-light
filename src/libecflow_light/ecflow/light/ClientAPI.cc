/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/ClientAPI.hpp"

#include <cassert>
#include <charconv>
#include <cstdlib>
#include <memory>

#include <eckit/config/LocalConfiguration.h>
#include <eckit/config/YAMLConfiguration.h>
#include <eckit/exception/Exceptions.h>
#include <eckit/filesystem/PathName.h>
#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.hpp"
#include "ecflow/light/Exception.hpp"
#include "ecflow/light/Trace.hpp"

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct Environment {
    static std::optional<std::string> get_variable(const char* variable_name) {
        if (const char* variable_value = ::getenv(variable_name); variable_value) {
            return variable_value;
        }
        return {};
    }

    static std::string get_mandatory_variable(const char* variable_name) {
        if (const char* variable_value = ::getenv(variable_name); variable_value) {
            return variable_value;
        }
        else {
            throw InvalidEnvironmentException("NO ", variable_name, " available");
        }
    }
};

Configuration Configuration::make_cfg() {
    Configuration cfg;

    // Load Environment Variables
    //  - Optional variables
    bool skip_connections = Environment::get_variable("NO_ECF").has_value();

    //  - Mandatory variables -- Important: will throw if not available!
    std::string task_rid      = Environment::get_mandatory_variable("ECF_RID");
    std::string task_name     = Environment::get_mandatory_variable("ECF_NAME");
    std::string task_password = Environment::get_mandatory_variable("ECF_PASS");
    std::string task_try_no   = Environment::get_mandatory_variable("ECF_TRYNO");

    // Load Configuration from YAML
    if (auto yaml_cfg_file = Environment::get_variable("IFS_ECF_CONFIG_PATH"); yaml_cfg_file) {
        // Attempt to use YAML configuration path, if provided
        try {
            eckit::YAMLConfiguration yaml_cfg{eckit::PathName(yaml_cfg_file.value())};

            auto connections = yaml_cfg.getSubConfigurations("connections");
            for (const auto& connection : connections) {
                if (connection.has("protocol") && !skip_connections) {
                    std::string protocol = connection.getString("protocol");

                    std::string host;
                    if (connection.has("host")) {
                        host = connection.getString("host");
                    }
                    std::string port;
                    if (connection.has("port")) {
                        port = connection.getString("port");
                    }

                    cfg.connections.push_back(
                        Connection{protocol, host, port, task_rid, task_name, task_password, task_try_no});
                }
            }

            if (yaml_cfg.has("log_level")) {
                cfg.log_level = yaml_cfg.getString("log_level");
                Log::log<Log::Level::Info>("Using log level (from YAML):", cfg.log_level);
            }
        }
        catch (eckit::CantOpenFile& e) {
            Log::log<Log::Level::Warn>(
                "Unable to open YAML configuration file - using default configuration parameters");
            // TODO: rethrow error opening configuration? Or should we silently ignore the lack of a YAML file?
        }
        catch (... /* + eckit::BadConversion& e */) {
            Log::log<Log::Level::Warn>("Unable to open YAML configuration file, due to unknown issue");
            // TODO: Unable to catch a BadConversion since it is not defined in any ecKit header
        }
    }
    else {
        Log::log<Log::Level::Warn>("No connection configured as no YAML configuration was provided.");
    }


    return cfg;
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

void CLIDispatcher::dispatch_request(const Connection& connection [[maybe_unused]], const std::string& request) {
    Log::log<Log::Level::Info>("Dispatching CLI Request: ", request);
    ::system(request.c_str());
}

// *** Client (UDP) ************************************************************
// *****************************************************************************

void UDPDispatcher::dispatch_request(const Connection& connection, const std::string& request) {
    int port                 = convert_to<int>(connection.port);
    const size_t packet_size = request.size() + 1;

    Log::log<Log::Level::Info>("Dispatching UDP Request: ", request, ", to ", connection.host, ":", connection.port);

    if (packet_size > UDPPacketMaximumSize) {
        throw InvalidRequestException("Request too large. Maximum size expected is ", UDPPacketMaximumSize,
                                      ", but found: ", packet_size);
    }

    try {
        eckit::net::UDPClient client(connection.host, port);
        client.send(request.data(), packet_size);
    }
    catch (std::exception& e) {
        throw InvalidRequestException("Unable to send request: ", e.what());
    }
    catch (...) {
        throw InvalidRequestException("Unable to send request, due to unknown error");
    }
}

}  // namespace ecflow::light
