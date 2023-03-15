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

#include <iostream>
#include <sstream>

#include <eckit/net/UDPClient.h>

namespace ecflow::light {

constexpr size_t UDP_PACKET_MAXIMUM_SIZE = 65'507;

struct InvalidRequestException : public std::runtime_error {
    template <typename... ARGS>
    explicit InvalidRequestException(ARGS... args) : std::runtime_error(make_msg(args...)) {}

private:
    template <typename... ARGS>
    std::string make_msg(ARGS... args) {
        std::ostringstream oss;
        ((oss << args), ...);
        return oss.str();
    }
};

namespace /* __anonymous__ */ {

struct ConfigurationOptions {
    std::string host = "localhost";
    int port         = 8080;
};

ConfigurationOptions CFG;

struct EnvironmentOptions {

    EnvironmentOptions() :
        task_rid{EnvironmentOptions::get_variable("ECF_RID")},
        task_name{EnvironmentOptions::get_variable("ECF_NAME")},
        task_password{EnvironmentOptions::get_variable("ECF_PASS")},
        task_try_no{EnvironmentOptions::get_variable("ECF_TRYNO")} {}

    std::string task_rid;
    std::string task_name;
    std::string task_password;
    std::string task_try_no;

private:
    static std::string get_variable(const char* variable_name) {
        if (const char* variable_value = ::getenv(variable_name); variable_value) {
            return variable_value;
        }
        else {
            throw InvalidRequestException("NO ", variable_name, " available");
        }
    }
};

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

void dispatch_request(const ConfigurationOptions& cfg, const std::string& request) {
    std::cout << "INFO: Request: " << request << ", sent to " << cfg.host << ":" << cfg.port << std::endl;

    const size_t packet_size = request.size() + 1;

    if (packet_size > UDP_PACKET_MAXIMUM_SIZE) {
        throw InvalidRequestException("Request too large. Maximum size expected is ", UDP_PACKET_MAXIMUM_SIZE,
                                      ", but found: ", packet_size);
    }

    try {
        eckit::net::UDPClient client(cfg.host, cfg.port);
        client.send(request.data(), packet_size);
    }
    catch (std::exception& e) {
        throw InvalidRequestException("Unable to send request: ", e.what());
    }
    catch (...) {
        throw InvalidRequestException("Unable to send request, due to unknown error");
    }
}

}  // namespace

void init() {
    if (const char* var = ::getenv("ECF_UDP_HOST"); var) {
        CFG.host = var;
    }

    if (const char* var = ::getenv("ECF_UDP_PORT"); var) {
        CFG.port = std::atoi(var);
    }
}

int child_update_meter(const std::string& name, int value) {
    try {
        EnvironmentOptions env;
        std::string request
            = format_request(env.task_rid, env.task_password, env.task_try_no, "meter", env.task_name, name, value);
        dispatch_request(CFG, request);
    }
    catch (InvalidRequestException& e) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int child_update_label(const std::string& name, const std::string& value) {
    try {
        EnvironmentOptions env;
        std::string request
            = format_request(env.task_rid, env.task_password, env.task_try_no, "label", env.task_name, name, value);
        dispatch_request(CFG, request);
    }
    catch (InvalidRequestException& e) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int child_update_event(const std::string& name, bool value) {
    try {
        EnvironmentOptions env;
        std::string request
            = format_request(env.task_rid, env.task_password, env.task_try_no, "event", env.task_name, name, value);
        dispatch_request(CFG, request);
    }
    catch (InvalidRequestException& e) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace ecflow::light
