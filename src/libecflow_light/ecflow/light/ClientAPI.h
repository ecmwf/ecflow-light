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

#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <eckit/exception/Exceptions.h>

#include "ecflow/light/Configuration.h"
#include "ecflow/light/Exception.h"
#include "ecflow/light/Log.h"
#include "ecflow/light/Requests.h"
#include "ecflow/light/Dispatcher.h"

namespace ecflow::light {

// *** Client ******************************************************************
// *****************************************************************************

class ClientAPI {
public:
    virtual ~ClientAPI() = default;

    [[nodiscard]] virtual Response process(const Request& request) const = 0;
};

// *** Client (Phony) **********************************************************
// *****************************************************************************

class PhonyClientAPI : public ClientAPI {
public:
    ~PhonyClientAPI() override = default;

    [[nodiscard]] Response process(const Request& request) const override;
};

// *** Client (Composite) **********************************************************
// *****************************************************************************

class CompositeClientAPI : public ClientAPI {
public:
    ~CompositeClientAPI() override = default;

    void add(std::unique_ptr<ClientAPI>&& api);

    [[nodiscard]] Response process(const Request& request) const override;

private:
    std::vector<std::unique_ptr<ClientAPI>> apis_;
};

// *** Client (Common) *********************************************************
// *****************************************************************************

template <typename Dispatcher>
class BaseClientAPI : public ClientAPI {
public:
    explicit BaseClientAPI(ClientCfg cfg, Environment env) : cfg{std::move(cfg)}, env{std::move(env)} {};
    ~BaseClientAPI() override = default;

    [[nodiscard]] Response process(const Request& request) const override {
        Dispatcher dispatcher{cfg};
        return dispatcher.call_dispatch(request);
    }

private:
    ClientCfg cfg;
    Environment env;
};

using LibraryHTTPClientAPI    = BaseClientAPI<HTTPDispatcher>;
using LibraryUDPClientAPI     = BaseClientAPI<UDPDispatcher>;
using CommandLineTCPClientAPI = BaseClientAPI<CLIDispatcher>;

// *** Configured Client *******************************************************
// *****************************************************************************

class ConfiguredClient : public ClientAPI {
public:
    static ConfiguredClient& instance() {
        static ConfiguredClient theInstance;
        return theInstance;
    }

    [[nodiscard]] Response process(const Request& request) const override;

private:
    ConfiguredClient();

    CompositeClientAPI clients_;
    mutable std::mutex lock_;
};

}  // namespace ecflow::light

#endif
