/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_OPTIONS_H
#define ECFLOW_LIGHT_OPTIONS_H

#include <optional>
#include <string>
#include <unordered_map>

#include "ecflow/light/Exception.h"

namespace ecflow::light {

struct OptionNotFound : public eckit::Exception {
    OptionNotFound(const std::string& msg, const eckit::CodeLocation& loc) : eckit::Exception(msg, loc) {}
};

struct Option {
    using name_t  = std::string;
    using value_t = std::string;

    name_t name;
    value_t value;
};

/**
 * Options stores the information collected from the program options (i.e. cli arguments)
 */
class Options {
public:
    using dict_t = std::unordered_map<std::string, Option>;

    Options() = default;

    static Options options() { return {}; };

    [[nodiscard]] Options& with(const Option::name_t& name, const Option::value_t& value) {
        return this->with(Option{name, value});
    }

    [[nodiscard]] Options& with(const Option& option) {
        options_.insert_or_assign(option.name, option);
        return *this;
    }

    [[nodiscard]] const Option& get(const std::string& name) const {
        auto found = options_.find(name);
        if (found == std::end(options_)) {
            ECFLOW_LIGHT_THROW(OptionNotFound, Message("Option '", name, "' not found"));
        }
        return found->second;
    }

    [[nodiscard]] std::optional<Option> find_value(const std::string& name) const {
        if (auto found = options_.find(name); found != std::end(options_)) {
            return std::make_optional(found->second);
        }
        return std::nullopt;
    }

private:
    dict_t options_;
};

}  // namespace ecflow::light

#endif
