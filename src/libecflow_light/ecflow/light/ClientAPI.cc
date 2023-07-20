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

#include <cstdlib>
#include <memory>
#include <optional>
#include <regex>

#include <eckit/net/UDPClient.h>

#include "ecflow/light/Conversion.h"
#include "ecflow/light/Exception.h"
#include "ecflow/light/Log.h"
#include "ecflow/light/TinyREST.hpp"

namespace ecflow::light {

// *** Client (Phony) **********************************************************
// *****************************************************************************

Response PhonyClientAPI::process(const Request& request) const {
    Log::info() << "Dispatching Phony Request: '" << request.description() << std::endl;
    return Response{"OK"};
};

// *** Client (Composite) **********************************************************
// *****************************************************************************

void CompositeClientAPI::add(std::unique_ptr<ClientAPI>&& api) {
    apis_.push_back(std::move(api));
}

Response CompositeClientAPI::process(const Request& request) const {
    std::vector<Response> responses;
    std::for_each(std::begin(apis_), std::end(apis_), [&](const auto& api) {
        Response response = api->process(request);
        responses.push_back(response);
    });
    if (responses.empty()) {
        throw std::runtime_error("No Responses available");
    }
    return responses.back();  // TODO: What should happen in this case?!
};

// *** Client (CLI) ************************************************************
// *****************************************************************************

Response CLIDispatcher::dispatch_request(const ClientCfg& cfg [[maybe_unused]], const std::string& request) {
    Log::info() << "Dispatching CLI Request: " << request << std::endl;
    ::system(request.c_str());

    return Response{"OK"};
};

// *** Client (UDP) ************************************************************
// *****************************************************************************

Response UDPDispatcher::dispatch_request(const ClientCfg& cfg, const std::string& request) {
    Log::info() << "Dispatching UDP Request: " << request << ", to " << cfg.host << ":" << cfg.port << std::endl;

    const size_t packet_size = request.size() + 1;
    if (packet_size > UDPPacketMaximumSize) {
        ECFLOW_LIGHT_THROW(InvalidRequest, Message("Request too large. Maximum size expected is ", UDPPacketMaximumSize,
                                                   ", but found: ", packet_size));
    }

    int port = convert_to<int>(cfg.port);
    eckit::net::UDPClient client(cfg.host, port);
    client.send(request.data(), packet_size);

    return Response{"OK"};
};

// *** Client (HTTP) ************************************************************
// *****************************************************************************

Response HTTPDispatcher::dispatch_request(const ClientCfg& cfg [[maybe_unused]], const net::Request& request) {
    Log::info() << "Dispatching HTTP Request: " << request.body().value()
                << " on url: " << request.header().url().as_string() << std::endl;

    net::TinyRESTClient rest;
    net::Response response = rest.handle(request);

    Log::info() << "Collected HTTP Response: "
                << static_cast<std::underlying_type_t<net::Status::Code>>(response.header().status()) << std::endl;

    return Response{"OK"};
};

// *** Configured Client *******************************************************
// *****************************************************************************

ConfiguredClient::ConfiguredClient() : clients_{}, lock_{} {
    Configuration cfg = Configuration::make_cfg();

    const Environment& environment = Environment::environment();

    // Setup configured API based on the configuration
    if (cfg.clients.empty()) {
        Log::warning() << "No Clients registered";
    }
    else {
        for (const auto& client : cfg.clients) {
            if (client.kind == ClientCfg::KindLibrary && client.protocol == ClientCfg::ProtocolUDP) {
                Log::debug() << "Library (UDP) Client registered" << std::endl;
                clients_.add(std::make_unique<LibraryUDPClientAPI>(client, environment));
            }
            else if (client.kind == ClientCfg::KindLibrary && client.protocol == ClientCfg::ProtocolHTTP) {
                Log::debug() << "Library (HTTP) Client registered" << std::endl;
                clients_.add(std::make_unique<LibraryHTTPClientAPI>(client, environment));
            }
            else if (client.kind == ClientCfg::KindCLI && client.protocol == ClientCfg::ProtocolTCP) {
                Log::debug() << "CLI (TCP) Client registered" << std::endl;
                clients_.add(std::make_unique<CommandLineTCPClientAPI>(client, environment));
            }
            else if (client.kind == ClientCfg::KindPhony && client.protocol == ClientCfg::ProtocolNone) {
                Log::debug() << "(Phony) Client registered" << std::endl;
                clients_.add(std::make_unique<PhonyClientAPI>());
            }
            else {
                Log::error() << "Invalid client '" << client.kind << "' detected, using protocol '" << client.protocol
                             << "'. Ignored!..." << std::endl;
            }
        }
    }
}

Response ConfiguredClient::process(const Request& request) const {
    std::scoped_lock lock(lock_);
    return clients_.process(request);
}

}  // namespace ecflow::light
