/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/API.h"
#include "ecflow/light/API.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>

#include "ecflow/light/ClientAPI.hpp"
#include "ecflow/light/Exception.hpp"

extern "C" {

int ecflow_light_update_meter(const char* name, int value) {
    return ecflow::light::update_meter(name, value);
}

int ecflow_light_update_label(const char* name, const char* value) {
    return ecflow::light::update_label(name, value);
}

int ecflow_light_update_event(const char* name, int value) {
    return ecflow::light::update_event(name, value);
}

}  // extern "C"

namespace ecflow::light {

namespace /* __anonymous__ */ {

/// Mutex mx is used to prevent reentrant calls to the API
std::mutex mx;

ClientAPI* the_configured_client() {
    static std::unique_ptr<ClientAPI> configured_client = nullptr;

    if (configured_client == nullptr) {
        Configuration cfg = Configuration::make_cfg();

        // Setup configured API based on the configuration
        if (cfg.protocol == Configuration::ProtocolUDP) {
            configured_client = std::make_unique<UDPClientAPI>(cfg);
        }
        else {
            throw BadValueException("Invalid value for 'protocol': ", cfg.protocol);
        }
    }

    return configured_client.get();
}

}  // namespace

int update_meter(const std::string& name, int value) {
    std::scoped_lock lock(mx);

    try {
        the_configured_client()->update_meter(name, value);
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

int update_label(const std::string& name, const std::string& value) {
    std::scoped_lock lock(mx);

    try {
        the_configured_client()->update_label(name, value);
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

int update_event(const std::string& name, bool value) {
    std::scoped_lock lock(mx);

    try {
        the_configured_client()->update_event(name, value);
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
