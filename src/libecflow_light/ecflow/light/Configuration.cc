/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/Configuration.h"

#include <regex>

#include <eckit/config/LocalConfiguration.h>
#include <eckit/config/YAMLConfiguration.h>
#include <eckit/filesystem/PathName.h>

#include "ecflow/light/Environment.h"
#include "ecflow/light/Exception.h"
#include "ecflow/light/Log.h"

namespace ecflow::light {

struct InvalidEnvironment : public eckit::Exception {
    InvalidEnvironment(const std::string& msg, const eckit::CodeLocation& loc) : eckit::Exception(msg, loc) {}
};

std::ostream& operator<<(std::ostream& os, const ClientCfg& cfg) {
    os << R"({)";
    os << R"("kind":")" << cfg.kind << R"(",)";
    os << R"("protocol":")" << cfg.protocol << R"(",)";
    os << R"("host":")" << cfg.host << R"(",)";
    os << R"("port":")" << cfg.port << R"(",)";
    os << R"("version":")" << cfg.version << R"(")";
    // Omitting task specific configuration parameters
    os << R"(})";
    return os;
}

Configuration Configuration::make_cfg() {
    Configuration cfg{};

    // Load Environment Variables
    Environment environment = Environment::environment();
    //  - Check Optional variables
    using namespace std::string_literals;
#if __GNUC__ >= 8 or __clang_major__ >= 5
    auto variable     = environment.get_optionals("NO_ECF"s, "NO_SMS"s, "NOECF"s, "NOSMS"s);
#else
    auto variable     = environment.get_optionals(std::vector{"NO_ECF"s, "NO_SMS"s, "NOECF"s, "NOSMS"s});
#endif
    bool skip_clients = variable.has_value();
    if (skip_clients) {
        Log::warning()
            << Message("'", variable->name, "' environment variable detected. Configuring Phony client.").str()
            << std::endl;

        cfg.clients.push_back(ClientCfg::make_phony());
        return cfg;
    }

    // Load Configuration from YAML
    if (auto yaml_cfg_file = environment.get_optional("IFS_ECF_CONFIG_PATH"); yaml_cfg_file) {
        // Attempt to use YAML configuration path, if provided
        Log::debug() << "YAML defined by IFS_ECF_CONFIG_PATH: '" << yaml_cfg_file->value << "'" << std::endl;
        eckit::YAMLConfiguration yaml_cfg{eckit::PathName(yaml_cfg_file->value)};

        auto clients = yaml_cfg.getSubConfigurations("clients");
        for (const auto& client : clients) {

            auto get = [&client](const std::string& name, const std::string& default_value = std::string()) {
                std::string value = default_value;
                if (client.has(name)) {
                    client.get(name, value);
                }
                return value;
            };

            std::string kind     = get("kind");
            std::string protocol = get("protocol");
            std::string host     = get("host");
            std::string port     = get("port");
            std::string version  = get("version", "1.0");

            // Replace environment variables
            host = replace_env_var(host, environment);
            port = replace_env_var(port, environment);

            cfg.clients.push_back(ClientCfg::make_cfg(kind, protocol, host, port, version));

            Log::debug() << "Client configuration: " << cfg.clients.back() << std::endl;
        }
    }
    else {
        ECFLOW_LIGHT_THROW(InvalidEnvironment,
                           Message("Unable to load YAML configuration as 'IFS_ECF_CONFIG_PATH' is not defined"));
    }

    return cfg;
}

}  // namespace ecflow::light
