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

#include <sstream>

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct ConfigurationOptions {

    ConfigurationOptions();

    std::string host = "localhost";
    std::string port = "8080";

private:
    static void update_variable(const char* variable_name, std::string& variable_value);
};

// *** Environment *************************************************************
// *****************************************************************************

struct EnvironmentOptions {

    EnvironmentOptions();

    void init();

    std::string task_rid;
    std::string task_name;
    std::string task_password;
    std::string task_try_no;

private:
    static std::string get_variable(const char* variable_name);
};

// *** Client ******************************************************************
// *****************************************************************************

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    virtual void child_update_meter(
        const EnvironmentOptions& env, const std::string& name, int value) = 0;
    virtual void child_update_label(
        const EnvironmentOptions& env, const std::string& name, const std::string& value) = 0;
    virtual void child_update_event(
        const EnvironmentOptions& env, const std::string& name, bool value) = 0;
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

struct BaseUDPDispatcher {
    static void dispatch_request(const ConfigurationOptions& cfg, const std::string& request);

    static constexpr size_t UDP_PACKET_MAXIMUM_SIZE = 65'507;
};

template <typename DISPATCHER>
class BaseUDPClientAPI : public ClientAPI {
public:
    explicit BaseUDPClientAPI(ConfigurationOptions cfg) : cfg{std::move(cfg)} {}
    ~BaseUDPClientAPI() override = default;

    void child_update_meter(const EnvironmentOptions& env, const std::string& name, int value) override;
    void child_update_label(const EnvironmentOptions& env, const std::string& name, const std::string& value) override;
    void child_update_event(const EnvironmentOptions& env, const std::string& name, bool value) override;

private:
    ConfigurationOptions cfg;
};

namespace implementation_detail /* __anonymous__ */ {

template <typename T>
std::string format_request(const std::string& task_remote_id, const std::string& task_password,
                           const std::string& task_try_no, const std::string& command, const std::string& path,
                           const std::string& name, T value) {
    std::ostringstream oss;
    // clang-format off
    oss << R"({)"
            << R"("method":"put",)"
            << R"("header":)"
            << R"({)"
                << R"("task_rid":")" << task_remote_id << R"(",)"
                << R"("task_password":")" << task_password << R"(",)"
                << R"("task_try_no":)" << task_try_no
            << R"(},)"
            << R"("payload":)"
            << R"({)"
                << R"("command":")" << command << R"(",)"
                << R"("path":")" << path << R"(",)"
                << R"("name":")" << name << R"(",)"
                << R"("value":")"<< value << R"(")"
            << R"(})"
        << R"(})";
    // clang-format on
    return oss.str();
}

}  // namespace implementation_detail

template <typename DISPATCHER>
void BaseUDPClientAPI<DISPATCHER>::child_update_meter(const EnvironmentOptions& env, const std::string& name,
                                                      int value) {
    std::string request = implementation_detail::format_request(env.task_rid, env.task_password, env.task_try_no,
                                                                "meter", env.task_name, name, value);
    DISPATCHER::dispatch_request(cfg, request);
}

template <typename DISPATCHER>
void BaseUDPClientAPI<DISPATCHER>::child_update_label(const EnvironmentOptions& env, const std::string& name,
                                                      const std::string& value) {
    std::string request = implementation_detail::format_request(env.task_rid, env.task_password, env.task_try_no,
                                                                "label", env.task_name, name, value);
    DISPATCHER::dispatch_request(cfg, request);
}

template <typename DISPATCHER>
void BaseUDPClientAPI<DISPATCHER>::child_update_event(const EnvironmentOptions& env, const std::string& name,
                                                      bool value) {
    std::string request = implementation_detail::format_request(env.task_rid, env.task_password, env.task_try_no,
                                                                "event", env.task_name, name, value);
    DISPATCHER::dispatch_request(cfg, request);
}

using UDPClientAPI = BaseUDPClientAPI<BaseUDPDispatcher>;

}  // namespace ecflow::light

#endif
