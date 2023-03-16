/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/Version.h"

#include <sstream>

namespace ecflow::light {

std::string ecflow_light_version() {
    std::ostringstream oss;
    int major = ecflow_light_VERSION_MAJOR;
    int minor = ecflow_light_VERSION_MINOR;
    int patch = ecflow_light_VERSION_PATCH;
    oss << major << "." << minor << "." << patch;
    return oss.str();
}

}  // namespace ecflow::light
