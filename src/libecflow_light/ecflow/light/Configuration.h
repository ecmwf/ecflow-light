/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_CONFIGURATION_H
#define ECFLOW_LIGHT_CONFIGURATION_H

#include <string>
#include <vector>

namespace ecflow::light {

// *** Configuration ***********************************************************
// *****************************************************************************

struct ClientCfg {

    static ClientCfg make_empty() { return ClientCfg{}; }

    static ClientCfg make_phony() {
        return ClientCfg{std::string(KindPhony), std::string(ProtocolNone), std::string(), std::string(),
                         std::string("1.0")};
    }

    static ClientCfg make_cfg(std::string kind, std::string protocol, std::string host, std::string port,
                              std::string version) {
        return ClientCfg{std::move(kind), std::move(protocol), std::move(host), std::move(port), std::move(version)};
    }

private:
    ClientCfg() : kind(), protocol(), host(), port(), version() {}

    ClientCfg(std::string kind, std::string protocol, std::string host, std::string port, std::string version) :
        kind(std::move(kind)),
        protocol(std::move(protocol)),
        host(std::move(host)),
        port(std::move(port)),
        version(std::move(version)) {}

public:
    std::string kind;
    std::string protocol;
    std::string host;
    std::string port;
    std::string version;

    static constexpr const char* ProtocolHTTP = "http";
    static constexpr const char* ProtocolUDP  = "udp";
    static constexpr const char* ProtocolTCP  = "tcp";
    static constexpr const char* ProtocolNone = "none";

    static constexpr const char* KindLibrary = "library";
    static constexpr const char* KindCLI     = "cli";
    static constexpr const char* KindPhony   = "phony";
};

struct Configuration {
    std::vector<ClientCfg> clients;

    static Configuration make_cfg();
};

}  // namespace ecflow::light

#endif
