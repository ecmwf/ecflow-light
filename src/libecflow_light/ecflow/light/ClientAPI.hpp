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
#include <mutex>
#include <sstream>
#include <vector>

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct Connection {
    std::string protocol = ProtocolUDP;
    std::string host;
    std::string port;

    std::string task_rid;
    std::string task_name;
    std::string task_password;
    std::string task_try_no;

    static constexpr const char* ProtocolUDP = "udp";
    static constexpr const char* ProtocolCLI = "cli";
};

struct Configuration {

    std::vector<Connection> connections;

    std::string log_level = "None";

    static Configuration make_cfg();
};


// *** Client ******************************************************************
// *****************************************************************************

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    virtual void update_meter(const std::string& name, int value) const                = 0;
    virtual void update_label(const std::string& name, const std::string& value) const = 0;
    virtual void update_event(const std::string& name, bool value) const               = 0;
};

// *** Client (No OP) **********************************************************
// *****************************************************************************

class NullClientAPI : public ClientAPI {
public:
    ~NullClientAPI() override = default;

    void update_meter(const std::string& name [[maybe_unused]], int value [[maybe_unused]]) const override{};
    void update_label(const std::string& name [[maybe_unused]],
                      const std::string& value [[maybe_unused]]) const override{};
    void update_event(const std::string& name [[maybe_unused]], bool value [[maybe_unused]]) const override{};
};

// *** Client (Composite) **********************************************************
// *****************************************************************************

class CompositeClientAPI : public ClientAPI {
public:
    ~CompositeClientAPI() override = default;

    void add(std::unique_ptr<ClientAPI>&& api);

    void update_meter(const std::string& name, int value) const override;
    void update_label(const std::string& name, const std::string& value) const override;
    void update_event(const std::string& name, bool value) const override;

private:
    std::vector<std::unique_ptr<ClientAPI>> apis_;
};

// *** Client (CLI) **********************************************************
// *****************************************************************************

struct CLIDispatcher {
    static void dispatch_request(const Connection& connection, const std::string& request);
};

struct CLIFormatter {
    template <typename T, std::enable_if_t<!std::is_same_v<bool, T>, bool> = true>
    static std::string format_request(const std::string& task_remote_id [[maybe_unused]],
                                      const std::string& task_password [[maybe_unused]],
                                      const std::string& task_try_no [[maybe_unused]], const std::string& command,
                                      const std::string& path [[maybe_unused]], const std::string& name, T value) {
        std::ostringstream oss;
        oss << R"(ecflow_client --)" << command << R"(=)" << name << R"( ")" << value << R"(" &)";
        return oss.str();
    }

    template <typename T, std::enable_if_t<std::is_same_v<bool, T>, bool> = true>
    static std::string format_request(const std::string& task_remote_id [[maybe_unused]],
                                      const std::string& task_password [[maybe_unused]],
                                      const std::string& task_try_no [[maybe_unused]], const std::string& command,
                                      const std::string& path [[maybe_unused]], const std::string& name, T value) {

        std::string parameter = value ? "set" : "clear";

        std::ostringstream oss;
        oss << R"(ecflow_client --)" << command << R"(=)" << name << R"( ")" << parameter << R"(" &)";
        return oss.str();
    }
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

struct UDPDispatcher {
    static void dispatch_request(const Connection& connection, const std::string& request);

    static constexpr size_t UDPPacketMaximumSize = 65'507;
};

struct UDPFormatter {
    template <typename T>
    static std::string format_request(const std::string& task_remote_id, const std::string& task_password,
                                      const std::string& task_try_no, const std::string& command,
                                      const std::string& path, const std::string& name, T value) {
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
};

// *** Client (Common) *********************************************************
// *****************************************************************************

template <typename Dispatcher, typename Formatter>
class BaseClientAPI : public ClientAPI {
public:
    explicit BaseClientAPI(Connection connection) : connection{std::move(connection)} {}
    ~BaseClientAPI() override = default;

    void update_meter(const std::string& name, int value) const override {
        Dispatcher::dispatch_request(
            connection, Formatter::format_request(connection.task_rid, connection.task_password, connection.task_try_no,
                                                  "meter", connection.task_name, name, value));
    }
    void update_label(const std::string& name, const std::string& value) const override {
        Dispatcher::dispatch_request(
            connection, Formatter::format_request(connection.task_rid, connection.task_password, connection.task_try_no,
                                                  "label", connection.task_name, name, value));
    }
    void update_event(const std::string& name, bool value) const override {
        Dispatcher::dispatch_request(
            connection, Formatter::format_request(connection.task_rid, connection.task_password, connection.task_try_no,
                                                  "event", connection.task_name, name, value));
    }

private:
    Connection connection;
};

using UDPClientAPI = BaseClientAPI<UDPDispatcher, UDPFormatter>;
using CLIClientAPI = BaseClientAPI<CLIDispatcher, CLIFormatter>;

// *** Configured Client *******************************************************
// *****************************************************************************

class ConfiguredClient : public ClientAPI {
public:
    static ConfiguredClient& instance() {
        static ConfiguredClient theInstance;
        return theInstance;
    }

    void update_meter(const std::string& name, int value) const override;
    void update_label(const std::string& name, const std::string& value) const override;
    void update_event(const std::string& name, bool value) const override;

private:
    ConfiguredClient();

    CompositeClientAPI clients_;
    mutable std::mutex lock_;
};

}  // namespace ecflow::light

#endif
