/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_STRINGUTILS_H
#define ECFLOW_LIGHT_STRINGUTILS_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ecflow::light {

template <typename... ARGS>
std::string stringify(ARGS... args) {
    std::ostringstream os;
    ((os << args), ...);
    return os.str();
}

}  // namespace ecflow::light

#endif  // ECFLOW_LIGHT_STRINGUTILS_H
