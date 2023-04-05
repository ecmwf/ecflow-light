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
#include <memory>

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

    // Optional variables
    cfg.no_ecf = Environment::get_variable("NO_ECF").has_value();

    // Mandatory variables -- will throw if not available
    cfg.task_rid      = Environment::get_mandatory_variable("ECF_RID");
    cfg.task_name     = Environment::get_mandatory_variable("ECF_NAME");
    cfg.task_password = Environment::get_mandatory_variable("ECF_PASS");
    cfg.task_try_no   = Environment::get_mandatory_variable("ECF_TRYNO");

    if (auto yaml_cfg_file = Environment::get_variable("IFS_ECF_CONFIG_PATH"); yaml_cfg_file) {
        // Attempt to use YAML configuration path, if provided
        try {
            eckit::YAMLConfiguration yaml_cfg{eckit::PathName(yaml_cfg_file.value())};

            if (yaml_cfg.has("protocol")) {
                cfg.protocol = yaml_cfg.getString("protocol");
                Log::log<Log::Level::Info>("Using protocol (from YAML): ", cfg.protocol);
            }

            if (yaml_cfg.has("host")) {
                cfg.host = yaml_cfg.getString("host");
                Log::log<Log::Level::Info>("Using host (from YAML):", cfg.host);
            }

            if (yaml_cfg.has("port")) {
                cfg.port = yaml_cfg.getString("port");
                Log::log<Log::Level::Info>("Using port (from YAML):", cfg.port);
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

    return cfg;
}

// *** Client ******************************************************************
// *****************************************************************************

void BaseUDPDispatcher::dispatch_request(const Configuration& cfg, const std::string& request) {
    int port                 = convert_to<int>(cfg.port);
    const size_t packet_size = request.size() + 1;

    Log::log<Log::Level::Info>("Request: ", request, ", sent to ", cfg.host, ":", cfg.port);

    if (packet_size > UDPPacketMaximumSize) {
        throw InvalidRequestException("Request too large. Maximum size expected is ", UDPPacketMaximumSize,
                                      ", but found: ", packet_size);
    }

    try {
        eckit::net::UDPClient client(cfg.host, port);
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
