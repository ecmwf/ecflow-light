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

#include <cassert>
#include <charconv>
#include <iostream>
#include <memory>
#include <sstream>

#include <eckit/config/YAMLConfiguration.h>
#include <eckit/exception/Exceptions.h>
#include <eckit/filesystem/PathName.h>
#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.h"
#include "ecflow/light/Exception.h"

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

ConfigurationOptions::ConfigurationOptions() {
    if (const char* value = ::getenv("IFS_ECF_CONFIG_PATH"); value) {
        // 1. Attempt to use YAML configuration path, if provided
        std::filesystem::path yaml_cfg_file = value;
        load_cfg_from_file(yaml_cfg_file);
    }
    else {
        // 2. Otherwise, defined configuration based on environment variables
        // TODO: decide how to choose between UDP and HTTP when YAML is not provided
        update_variable("ECF_UDP_HOST", host);
        update_variable("ECF_UDP_PORT", port);
    }
}

void ConfigurationOptions::load_cfg_from_file(const std::filesystem::path& cfg_file) {
    try {
        eckit::PathName cfg_file_path(cfg_file);
        eckit::YAMLConfiguration cfg(cfg_file_path);

        if (cfg.has("protocol")) {
            this->host = cfg.getString("protocol");
            std::cout << "INFO: Using PROTOCOL (from YAML):" << this->host << "\n";
        }

        if (cfg.has("host")) {
            this->host = cfg.getString("host");
            std::cout << "INFO: Using ECF_*_HOST (from YAML):" << this->host << "\n";
        }

        if (cfg.has("port")) {
            this->port = cfg.getString("port");
            std::cout << "INFO: Using ECF_*_HOST (from YAML):" << this->port << "\n";
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

void ConfigurationOptions::update_variable(const char* variable_name, std::string& variable_value) {
    if (const char* value = ::getenv(variable_name); value) {
        variable_value = value;
        std::cout << "INFO: Using " << variable_name << " (from ENVIRONMENT):" << variable_value << "\n";
    }
}

// *** Environment *************************************************************
// *****************************************************************************

EnvironmentOptions::EnvironmentOptions() : task_rid{}, task_name{}, task_password{}, task_try_no{} {}

void EnvironmentOptions::init() {
    task_rid      = EnvironmentOptions::get_variable("ECF_RID");
    task_name     = EnvironmentOptions::get_variable("ECF_NAME");
    task_password = EnvironmentOptions::get_variable("ECF_PASS");
    task_try_no   = EnvironmentOptions::get_variable("ECF_TRYNO");
}

std::string EnvironmentOptions::get_variable(const char* variable_name) {
    if (const char* variable_value = ::getenv(variable_name); variable_value) {
        return variable_value;
    }
    else {
        throw InvalidEnvironmentException("NO ", variable_name, " available");
    }
}

// *** Client ******************************************************************
// *****************************************************************************

void BaseUDPDispatcher::dispatch_request(const ConfigurationOptions& cfg, const std::string& request) {
    int port                 = convert_to<int>(cfg.port);
    const size_t packet_size = request.size() + 1;

    std::cout << "INFO: Request: " << request << ", sent to " << cfg.host << ":" << cfg.port << "\n";

    if (packet_size > UDP_PACKET_MAXIMUM_SIZE) {
        throw InvalidRequestException("Request too large. Maximum size expected is ", UDP_PACKET_MAXIMUM_SIZE,
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
