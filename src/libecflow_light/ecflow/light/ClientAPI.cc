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

namespace ecflow::light {

struct Exception : public std::runtime_error {
    template <typename... ARGS>
    explicit Exception(ARGS... args) : std::runtime_error(make_msg(args...)) {}

private:
    template <typename... ARGS>
    std::string make_msg(ARGS... args) {
        std::ostringstream oss;
        ((oss << args), ...);
        return oss.str();
    }
};

struct InvalidEnvironmentException : public Exception {
    template <typename... ARGS>
    explicit InvalidEnvironmentException(ARGS... args) : Exception(args...) {}
};

struct InvalidRequestException : public Exception {
    template <typename... ARGS>
    explicit InvalidRequestException(ARGS... args) : Exception(args...) {}
};

struct BadValueException : public Exception {
    template <typename... ARGS>
    explicit BadValueException(ARGS... args) : Exception(args...) {}
};

namespace /* __anonymous__ */ {

struct convert_rule {

    template <typename FROM, typename TO, std::enable_if_t<std::is_integral_v<TO>, bool> = true>
    static TO convert(FROM from) {
        TO to;
        auto [ptr, ec] = std::from_chars(from.data(), from.data() + from.size(), to);

        if (ptr == from.data() + from.size()) {  // Succeed only if all chars where used in conversion
            return to;
        }
        else {
            throw BadValueException("Unable to convert port '", from, "' to integral value");
        }
    }

    template <typename FROM, typename TO, std::enable_if_t<std::is_same_v<TO, std::string>, bool> = true>
    static TO convert(const FROM& from) {
        std::ostringstream oss;
        oss << from;
        return oss.str();
    }
};

template <typename TO, typename FROM>
TO convert_to(FROM from) {
    return convert_rule::convert<FROM, TO>(from);
};

struct ConfigurationOptions {

    ConfigurationOptions() {
        update_variable("ECF_UDP_HOST", host);
        update_variable("ECF_UDP_PORT", port);
    }

    std::string host = "localhost";
    std::string port = "8080";

private:
    static void update_variable(const char* variable_name, std::string& variable_value) {
        if (const char* value = ::getenv(variable_name); value) {
            variable_value = value;
        }
    }
};

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
            throw InvalidEnvironmentException("NO ", variable_name, " available");
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

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    virtual void child_update_meter(const std::string& name, int value)                = 0;
    virtual void child_update_label(const std::string& name, const std::string& value) = 0;
    virtual void child_update_event(const std::string& name, bool value)               = 0;
};

class UDPClientAPI : public ClientAPI {
public:
    explicit UDPClientAPI(ConfigurationOptions cfg) : cfg{std::move(cfg)} {}
    ~UDPClientAPI() override = default;

    void child_update_meter(const std::string& name, int value) override {
        // Environment options capture the relevant ECF_* environment variables
        EnvironmentOptions env;
        std::string request
            = format_request(env.task_rid, env.task_password, env.task_try_no, "meter", env.task_name, name, value);
        dispatch_request(cfg, request);
    }
    void child_update_label(const std::string& name, const std::string& value) override {
        // Environment options capture the relevant ECF_* environment variables
        EnvironmentOptions env;
        std::string request
            = format_request(env.task_rid, env.task_password, env.task_try_no, "label", env.task_name, name, value);
        dispatch_request(cfg, request);
    }
    void child_update_event(const std::string& name, bool value) override {
        // Environment options capture the relevant ECF_* environment variables
        EnvironmentOptions env;
        std::string request
            = format_request(env.task_rid, env.task_password, env.task_try_no, "event", env.task_name, name, value);
        dispatch_request(cfg, request);
    }

private:
    ConfigurationOptions cfg;

    static constexpr size_t UDP_PACKET_MAXIMUM_SIZE = 65'507;

    static void dispatch_request(const ConfigurationOptions& cfg, const std::string& request) {

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
};

std::unique_ptr<ClientAPI> CONFIGURED_API;

}  // namespace

// ****************************************************************************

void init() {
    ConfigurationOptions cfg;
    CONFIGURED_API = std::make_unique<UDPClientAPI>(cfg);
}

int child_update_meter(const std::string& name, int value) {
    assert(CONFIGURED_API);

    try {
        CONFIGURED_API->child_update_meter(name, value);
    }
    catch (Exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "ERROR: unknown" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int child_update_label(const std::string& name, const std::string& value) {
    assert(CONFIGURED_API);

    try {
        CONFIGURED_API->child_update_label(name, value);
    }
    catch (Exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "ERROR: unknown" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int child_update_event(const std::string& name, bool value) {
    assert(CONFIGURED_API);

    try {
        CONFIGURED_API->child_update_event(name, value);
    }
    catch (Exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "ERROR: unknown" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace ecflow::light
