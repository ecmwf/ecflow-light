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

#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.h"
#include "ecflow/light/Exception.h"

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

ConfigurationOptions::ConfigurationOptions() {
    update_variable("ECF_UDP_HOST", host);
    update_variable("ECF_UDP_PORT", port);
}

void ConfigurationOptions::update_variable(const char* variable_name, std::string& variable_value) {
    if (const char* value = ::getenv(variable_name); value) {
        variable_value = value;
    }
}

// *** Environment *************************************************************
// *****************************************************************************

EnvironmentOptions::EnvironmentOptions() :
    task_rid{EnvironmentOptions::get_variable("ECF_RID")},
    task_name{EnvironmentOptions::get_variable("ECF_NAME")},
    task_password{EnvironmentOptions::get_variable("ECF_PASS")},
    task_try_no{EnvironmentOptions::get_variable("ECF_TRYNO")} {}

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

namespace implementation_detail /* __anonymous__ */ {

template <typename T>
std::string format_request(const std::string& task_remote_id, const std::string& task_password,
                           const std::string& task_try_no, const std::string& command, const std::string& path,
                           const std::string& name, T value) {
    std::ostringstream oss;
    // clang-format off
        oss << R"({"method":"put","header":{"task_rid":")"
            << task_remote_id
            << R"(","task_password":")"
            << task_password
            << R"(","task_try_no":)"
            << task_try_no
            << R"(},"payload":{"command":")"
            << command
            << R"(","path":")"
            << path
            << R"(","name":")"
            << name
            << R"(","value":")"
            << value
            << R"("}})";
    // clang-format on
    return oss.str();
}

}  // namespace implementation_detail

void UDPClientAPI::child_update_meter(const std::string& name, int value) {
    // Environment options capture the relevant ECF_* environment variables
    EnvironmentOptions env;
    std::string request = implementation_detail::format_request(env.task_rid, env.task_password, env.task_try_no,
                                                                "meter", env.task_name, name, value);
    dispatch_request(cfg, request);
}
void UDPClientAPI::child_update_label(const std::string& name, const std::string& value) {
    // Environment options capture the relevant ECF_* environment variables
    EnvironmentOptions env;
    std::string request = implementation_detail::format_request(env.task_rid, env.task_password, env.task_try_no,
                                                                "label", env.task_name, name, value);
    dispatch_request(cfg, request);
}
void UDPClientAPI::child_update_event(const std::string& name, bool value) {
    // Environment options capture the relevant ECF_* environment variables
    EnvironmentOptions env;
    std::string request = implementation_detail::format_request(env.task_rid, env.task_password, env.task_try_no,
                                                                "event", env.task_name, name, value);
    dispatch_request(cfg, request);
}

void UDPClientAPI::dispatch_request(const ConfigurationOptions& cfg, const std::string& request) {

    int port                 = convert_to<int>(cfg.port);
    const size_t packet_size = request.size() + 1;

    std::cout << "INFO: Request: " << request << ", sent to " << cfg.host << ":" << cfg.port << std::endl;

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
