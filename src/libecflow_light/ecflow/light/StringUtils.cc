/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/StringUtils.h"

namespace ecflow::light {

std::string trim(const std::string& source, const std::string& delim) {
    auto first = source.find_first_not_of(delim, 0);
    auto last  = source.find_last_not_of(delim, std::string::npos);

    if (first == std::string::npos && last == std::string::npos) {
        return {};
    }

    return std::string(source, first, last - first + 1);
}

std::vector<std::string> split(const std::string& source, const std::string& delim, bool allow_empty) {
    std::vector<std::string> tokens;

    for (std::string::size_type current = 0; current < source.length() + 1; /* ... */) {
        std::string::size_type pos = source.find_first_of(delim, current);

        // reached end of source string
        if (pos == std::string::npos) {
            pos = source.length();
        }

        // collect token
        if (pos != current || allow_empty) {
            tokens.emplace_back(source.data() + current, pos - current);
        }

        // advance
        current = pos + 1;
    }

    return tokens;
}

}  // namespace ecflow::light
