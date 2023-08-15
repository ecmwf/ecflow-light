/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_DISPATCHER_H
#define ECFLOW_LIGHT_DISPATCHER_H

#include <filesystem>
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

namespace ecflow::light {

// *** Client Dispatcher (Common) **********************************************
// *****************************************************************************

template <typename DISPATCHER>
class BaseRequestDispatcher : public RequestDispatcher {
public:
    explicit BaseRequestDispatcher(const ClientCfg& cfg) : cfg_{cfg}, response_{} {}

    Response call_dispatch(const Request& request) {
        DISPATCHER dispatcher{cfg_};
        request.dispatch(dispatcher);
        return response_;
    }

protected:
    const ClientCfg& cfg_;
    Response response_;
};

// *** Client Dispatcher (CLI) *************************************************
// *****************************************************************************

struct CLIDispatcher : public BaseRequestDispatcher<CLIDispatcher> {
public:
    explicit CLIDispatcher(const ClientCfg& cfg);

    void dispatch_request(const UpdateNodeStatus& request [[maybe_unused]]) override;
    void dispatch_request(const UpdateNodeAttribute& request) override;

private:
    static Response exchange_request(const ClientCfg& cfg, const std::string& request);
};

// *** Client Dispatcher (UDP) *************************************************
// *****************************************************************************

class UDPDispatcher : public BaseRequestDispatcher<UDPDispatcher> {
public:
    explicit UDPDispatcher(const ClientCfg& cfg);

    void dispatch_request(const UpdateNodeStatus& request) override;
    void dispatch_request(const UpdateNodeAttribute& request) override;

private:
    static Response exchange_request(const ClientCfg& cfg, const std::string& request);

    static constexpr size_t UDPPacketMaximumSize = 65'507;
};

// *** Client Dispatcher (HTTP) ************************************************
// *****************************************************************************

class HTTPDispatcher : public BaseRequestDispatcher<HTTPDispatcher> {
public:
    explicit HTTPDispatcher(const ClientCfg& cfg);

    void dispatch_request(const UpdateNodeStatus& request) override;
    void dispatch_request(const UpdateNodeAttribute& request) override;

private:
    template <net::Method METHOD>
    static Response exchange_request(const ClientCfg& cfg, const net::Request<METHOD>& request) {
        net::Host host{cfg.host, cfg.port};

        Log::info() << "Dispatching HTTP Request: " << request.body().value() << " to host: " << host.str()
                    << " and target: " << request.header().target().str() << std::endl;

        net::TinyRESTClient rest;
        net::Response response = rest.handle(host, request);

        Log::info() << "Collected HTTP Response: "
                    << static_cast<std::underlying_type_t<net::Status::Code>>(response.header().status())
                    << ", body: " << response.body() << std::endl;

        return Response{"OK"};
    }
};

}  // namespace ecflow::light

#endif
