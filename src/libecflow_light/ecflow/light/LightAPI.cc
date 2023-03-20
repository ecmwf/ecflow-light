/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/LightAPI.h"

#include <cassert>
#include <iostream>
#include <memory>

#include "ecflow/light/ClientAPI.h"
#include "ecflow/light/Exception.h"

namespace ecflow::light {

static std::unique_ptr<ClientAPI> CONFIGURED_API;

void init() {
    ConfigurationOptions cfg;

    // Setup configured API based on the configuration
    if (cfg.protocol == ConfigurationOptions::PROTOCOL_UDP) {
        CONFIGURED_API = std::make_unique<UDPClientAPI>(cfg);
    }
    else if (cfg.protocol == ConfigurationOptions::PROTOCOL_HTTP) {
        CONFIGURED_API = nullptr;
        assert(false);  // TODO: implement support for HTTP; currently missing child commands in REST API
    }
    else {
        throw BadValueException("Invalid value for 'protocol': ", cfg.protocol);
    }
}

int child_update_meter(const std::string& name, int value) {
    assert(CONFIGURED_API);

    // Capture Environment options (i.e relevant ECF_* environment variables)
    EnvironmentOptions env;
    env.init();

    try {
        CONFIGURED_API->child_update_meter(env, name, value);
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

    // Capture Environment options (i.e relevant ECF_* environment variables)
    EnvironmentOptions env;
    env.init();

    try {
        CONFIGURED_API->child_update_label(env, name, value);
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

    // Capture Environment options (i.e relevant ECF_* environment variables)
    EnvironmentOptions env;
    env.init();

    try {
        CONFIGURED_API->child_update_event(env, name, value);
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
