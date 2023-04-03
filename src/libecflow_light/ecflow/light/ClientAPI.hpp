/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_CLIENTAPI_HPP
#define ECFLOW_LIGHT_CLIENTAPI_HPP

#include <filesystem>
#include <sstream>

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct Configuration {

    bool no_ecf = false;

    std::string protocol = ProtocolUDP;
    std::string host     = "localhost";
    std::string port     = "8080";

    std::string task_rid;
    std::string task_name;
    std::string task_password;
    std::string task_try_no;

    static constexpr const char* ProtocolUDP = "udp";

    static Configuration make_cfg();
};


// *** Client ******************************************************************
// *****************************************************************************

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    virtual void update_meter(const std::string& name, int value)                = 0;
    virtual void update_label(const std::string& name, const std::string& value) = 0;
    virtual void update_event(const std::string& name, bool value)               = 0;
};

// *** Client (No OP) **********************************************************
// *****************************************************************************

class NullClientAPI : public ClientAPI {
public:
    ~NullClientAPI() override = default;

    void update_meter(const std::string& name [[maybe_unused]], int value [[maybe_unused]]) override{};
    void update_label(const std::string& name [[maybe_unused]], const std::string& value [[maybe_unused]]) override{};
    void update_event(const std::string& name [[maybe_unused]], bool value [[maybe_unused]]) override{};
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

struct BaseUDPDispatcher {
    static void dispatch_request(const Configuration& cfg, const std::string& request);

    static constexpr size_t UDPPacketMaximumSize = 65'507;
};

template <typename Dispatcher>
class BaseUDPClientAPI : public ClientAPI {
public:
    explicit BaseUDPClientAPI(Configuration cfg) : cfg{std::move(cfg)} {}
    ~BaseUDPClientAPI() override = default;

    void update_meter(const std::string& name, int value) override;
    void update_label(const std::string& name, const std::string& value) override;
    void update_event(const std::string& name, bool value) override;

private:
    Configuration cfg;
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

template <typename Dispatcher>
void BaseUDPClientAPI<Dispatcher>::update_meter(const std::string& name, int value) {
    Dispatcher::dispatch_request(
        cfg, implementation_detail::format_request(cfg.task_rid, cfg.task_password, cfg.task_try_no, "meter",
                                                   cfg.task_name, name, value));
}

template <typename Dispatcher>
void BaseUDPClientAPI<Dispatcher>::update_label(const std::string& name, const std::string& value) {
    Dispatcher::dispatch_request(
        cfg, implementation_detail::format_request(cfg.task_rid, cfg.task_password, cfg.task_try_no, "label",
                                                   cfg.task_name, name, value));
}

template <typename Dispatcher>
void BaseUDPClientAPI<Dispatcher>::update_event(const std::string& name, bool value) {
    Dispatcher::dispatch_request(
        cfg, implementation_detail::format_request(cfg.task_rid, cfg.task_password, cfg.task_try_no, "event",
                                                   cfg.task_name, name, value));
}

using UDPClientAPI = BaseUDPClientAPI<BaseUDPDispatcher>;

}  // namespace ecflow::light

#endif
