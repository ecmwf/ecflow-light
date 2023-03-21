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
#include <iostream>
#include <memory>
#include <sstream>

#include <eckit/config/YAMLConfiguration.h>
#include <eckit/exception/Exceptions.h>
#include <eckit/filesystem/PathName.h>
#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.hpp"
#include "ecflow/light/Exception.hpp"

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct Environment {
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

    // Mandatory variables -- will throw if not available
    cfg.task_rid      = Environment::get_mandatory_variable("ECF_RID");
    cfg.task_name     = Environment::get_mandatory_variable("ECF_NAME");
    cfg.task_password = Environment::get_mandatory_variable("ECF_PASS");
    cfg.task_try_no   = Environment::get_mandatory_variable("ECF_TRYNO");

    if (const char* yaml_cfg_file = ::getenv("IFS_ECF_CONFIG_PATH"); yaml_cfg_file) {
        // Attempt to use YAML configuration path, if provided
        try {
            eckit::YAMLConfiguration yaml_cfg{eckit::PathName(yaml_cfg_file)};

            if (yaml_cfg.has("protocol")) {
                cfg.protocol = yaml_cfg.getString("protocol");
                std::cout << "INFO: Using protocol (from YAML):" << cfg.protocol << "\n";
            }

            if (yaml_cfg.has("host")) {
                cfg.host = yaml_cfg.getString("host");
                std::cout << "INFO: Using host (from YAML):" << cfg.host << "\n";
            }

            if (yaml_cfg.has("port")) {
                cfg.port = yaml_cfg.getString("port");
                std::cout << "INFO: Using port (from YAML):" << cfg.port << "\n";
            }
        }
        catch (eckit::CantOpenFile& e) {
            std::cout << "ERROR: Unable to open YAML configuration file - using default configuration parameters\n";
            // TODO: rethrow error opening configuration? Or should we silently ignore the lack of a YAML file?
        }
        catch (... /* eckit::BadConversion& e */) {
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

    std::cout << "INFO: Request: " << request << ", sent to " << cfg.host << ":" << cfg.port << "\n";

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
