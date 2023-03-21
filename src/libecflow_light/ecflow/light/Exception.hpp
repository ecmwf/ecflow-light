/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_EXCEPTION_HPP
#define ECFLOW_LIGHT_EXCEPTION_HPP

#include <charconv>
#include <sstream>

namespace ecflow::light {

// *** Exceptions **************************************************************
// *****************************************************************************

struct Exception : public std::runtime_error {
    template <typename... ARGS>
    explicit Exception(ARGS... args) : std::runtime_error(make_msg(args...)) {}

private:
    template <typename... ARGS>
    std::string make_msg(ARGS... args) {
        std::ostringstream oss;
        ((oss << args), ...);
        return oss.str();
    }
};

struct InvalidEnvironmentException : public Exception {
    template <typename... ARGS>
    explicit InvalidEnvironmentException(ARGS... args) : Exception(args...) {}
};

struct InvalidRequestException : public Exception {
    template <typename... ARGS>
    explicit InvalidRequestException(ARGS... args) : Exception(args...) {}
};

struct BadValueException : public Exception {
    template <typename... ARGS>
    explicit BadValueException(ARGS... args) : Exception(args...) {}
};

struct NotImplementedYet : public Exception {
    template <typename... ARGS>
    explicit NotImplementedYet(ARGS... args) : Exception(args...) {}
};

}  // namespace ecflow::light

#endif
