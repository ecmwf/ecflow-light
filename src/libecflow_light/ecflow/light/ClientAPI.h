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

#include <charconv>
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

    virtual void child_update_meter(const std::string& name, int value)                = 0;
    virtual void child_update_label(const std::string& name, const std::string& value) = 0;
    virtual void child_update_event(const std::string& name, bool value)               = 0;
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

class UDPClientAPI : public ClientAPI {
public:
    explicit UDPClientAPI(ConfigurationOptions cfg) : cfg{std::move(cfg)} {}
    ~UDPClientAPI() override = default;

    void child_update_meter(const std::string& name, int value) override;
    void child_update_label(const std::string& name, const std::string& value) override;
    void child_update_event(const std::string& name, bool value) override;

private:
    ConfigurationOptions cfg;

    static constexpr size_t UDP_PACKET_MAXIMUM_SIZE = 65'507;

    static void dispatch_request(const ConfigurationOptions& cfg, const std::string& request);
};

}  // namespace ecflow::light

#endif
