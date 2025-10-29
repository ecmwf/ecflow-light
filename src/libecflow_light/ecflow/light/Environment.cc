/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/Environment.h"

#include <regex>

#include "ecflow/light/Log.h"

namespace ecflow::light {

std::string replace_env_var(const std::string& parameter, const Environment& environment) {
    static std::regex regex(R"(\$ENV\{([^}]*)\})");
    std::smatch match;
    if (bool found = std::regex_match(parameter, match, regex); found) {
        std::string name = match[1];

        // Retrieve the variable from the 'cached' Environment
        if (std::optional<Variable> variable = environment.get_optional(name); variable) {
            return variable->value;
        }

        // Retrieve the variable from the 'OS' Environment
        if (std::optional<Variable> variable = implementation_detail::Environment0::get_variable(name); variable) {
            return variable->value;
        }

        Log::warning() << Message("Environment variable '", name, "' not found. Replacement not possible...").str()
                       << std::endl;
    }
    return parameter;
}

}  // namespace ecflow::light
