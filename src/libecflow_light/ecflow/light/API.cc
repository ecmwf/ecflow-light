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
#include <memory>

#include "ecflow/light/ClientAPI.hpp"
#include "ecflow/light/Exception.hpp"
#include "ecflow/light/Trace.hpp"

extern "C" {

int ecflow_light_update_meter(const char* name, int value) {
    assert(name);

    ECFLOW_LIGHT_TRACE_FUNCTION(name, value);
    return ecflow::light::update_meter(name, value);
}

int ecflow_light_update_label(const char* name, const char* value) {
    assert(name);
    assert(value);

    ECFLOW_LIGHT_TRACE_FUNCTION(name, value);
    return ecflow::light::update_label(name, value);
}

int ecflow_light_update_event(const char* name, int value) {
    assert(name);

    ECFLOW_LIGHT_TRACE_FUNCTION(name, value);
    return ecflow::light::update_event(name, value);
}

}  // extern "C"

namespace ecflow::light {

int update_meter(const std::string& name, int value) {
    try {
        ConfiguredClient::instance().update_meter(name, value);
    }
    catch (eckit::Exception& e) {
        Log::log<Log::Level::Error>(e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        Log::log<Log::Level::Error>("Unknown error detected");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int update_label(const std::string& name, const std::string& value) {
    try {
        ConfiguredClient::instance().update_label(name, value);
    }
    catch (eckit::Exception& e) {
        Log::log<Log::Level::Error>(e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        Log::log<Log::Level::Error>("Unknown error detected");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int update_event(const std::string& name, bool value) {
    try {
        ConfiguredClient::instance().update_event(name, value);
    }
    catch (eckit::Exception& e) {
        Log::log<Log::Level::Error>(e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        Log::log<Log::Level::Error>("Unknown error detected");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace ecflow::light
