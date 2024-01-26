/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_ENVIRONMENT_H
#define ECFLOW_LIGHT_ENVIRONMENT_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecflow/light/Exception.h"

namespace ecflow::light {

struct EnvironmentVariableNotFound : public eckit::Exception {
    EnvironmentVariableNotFound(const std::string& msg, const eckit::CodeLocation& loc) : eckit::Exception(msg, loc) {}
};

/**
 * Variable represents the information of an environment variable
 */
struct Variable {
    using name_t  = std::string;
    using value_t = std::string;

    name_t name;
    value_t value;
};

namespace implementation_detail {

struct Environment0 {

    static std::optional<Variable> get_variable() { return {}; }

    template <typename S, typename... ARGS>
    static std::optional<Variable> get_variable(const S& variable_name, ARGS... other_variable_names) {
        if (auto variable = collect_variable(variable_name); variable) {
            return variable;
        }
        return get_variable(other_variable_names...);
    }

private:
    static std::optional<Variable> collect_variable(const char* variable_name) {
        if (const char* variable_value = ::getenv(variable_name); variable_value) {
            return std::make_optional(Variable{variable_name, variable_value});
        }
        return {};
    }

    static std::optional<Variable> collect_variable(const std::string& variable_name) {
        if (const char* variable_value = ::getenv(variable_name.c_str()); variable_value) {
            return std::make_optional(Variable{variable_name, variable_value});
        }
        return {};
    }
};

}  // namespace implementation_detail

/**
 * Environment stores information collected from the environment (e.g. environment variables)
 */
class Environment {
public:
    using dict_t = std::unordered_map<Variable::name_t, Variable>;

    Environment() = default;

    static Environment an_environment() { return {}; }

    static const Environment& environment() {
        static Environment environment = Environment()
                                             .from_environment("ECF_NAME")
                                             .from_environment("ECF_PASS")
                                             .from_environment("ECF_RID")
                                             .from_environment("ECF_TRYNO")
                                             .from_environment("ECF_HOST")
                                             .from_environment("NO_ECF")
                                             .from_environment("IFS_ECF_CONFIG_PATH");
        return environment;
    }

    [[nodiscard]] Environment& from_environment(const Variable::name_t& variable_name) {
        std::optional<Variable> variable = implementation_detail::Environment0::get_variable(variable_name);
        if (variable) {
            return this->with(variable->name, variable->value);
        }
        return *this;
    }

    [[nodiscard]] Environment& with(const Variable::name_t& name, const Variable::name_t& value) {
        environment_.insert_or_assign(name, Variable{name, value});
        return *this;
    }

    [[nodiscard]] const Variable& get(const Variable::name_t& name) const {
        auto found = environment_.find(name);
        if (found == std::end(environment_)) {
            ECFLOW_LIGHT_THROW(EnvironmentVariableNotFound, Message("Environment Variable '", name, "' not found"));
        }
        return found->second;
    }

    [[nodiscard]] std::optional<Variable> get_optional(const Variable::name_t& name) const {
        auto found = environment_.find(name);
        if (found == std::end(environment_)) {
            return std::nullopt;
        }
        return {found->second};
    }

#if __GNUC__ >= 8 or __clang_major__ >= 5
    template <typename... Ns>
    [[nodiscard]] std::optional<Variable> get_optionals(const Ns&... names) const {
        std::vector<Variable> variables;

        // Loop over all names, and collect found variables...
        (
            [&] {
                if (auto variable = get_optional(names); variable) {
                    variables.push_back(variable.value());
                }
            }(),
            ...);

        if (variables.empty()) {
            return std::nullopt;
        }

        // Take the first found variable as result
        return {variables.front()};
    }
#else
    [[nodiscard]] std::optional<Variable> get_optionals(const std::vector<std::string>& names) const {
        std::vector<Variable> variables;

        // Loop over all names, and collect found variables...
        for (const auto& name : names) {
            if (auto variable = get_optional(name); variable) {
                variables.push_back(variable.value());
            }
        }
        if (variables.empty()) {
            return std::nullopt;
        }

        // Take the first found variable as result
        return {variables.front()};
    }
#endif

private:
    dict_t environment_;
};

}  // namespace ecflow::light

#endif
