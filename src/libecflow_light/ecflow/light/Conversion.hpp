/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_CONVERSION_HPP
#define ECFLOW_LIGHT_CONVERSION_HPP

#include <charconv>
#include <sstream>

#include "ecflow/light/Exception.hpp"

namespace ecflow::light {

// *** Type Conversion *********************************************************
// *****************************************************************************

namespace implementation_detail {

struct convert_rule {

    template <typename FROM, typename TO, std::enable_if_t<std::is_integral_v<TO>, bool> = true>
    static TO convert(FROM from) {
        TO to{};
        auto [ptr, ec] = std::from_chars(from.data(), from.data() + from.size(), to);

        if (ptr == from.data() + from.size()) {  // Succeed only if all chars where used in conversion
            return to;
        }
        else {
            ECFLOW_LIGHT_THROW(BadValue, Message("Unable to convert string '", from, "' to integral value"));
        }
    }

    template <typename FROM, typename TO, std::enable_if_t<std::is_same_v<TO, std::string>, bool> = true>
    static TO convert(const FROM& from) {
        std::ostringstream oss;
        oss << from;
        return oss.str();
    }
};

}  // namespace implementation_detail

template <typename TO, typename FROM>
TO convert_to(FROM from) {
    return implementation_detail::convert_rule::convert<FROM, TO>(from);
}

}  // namespace ecflow::light

#endif
