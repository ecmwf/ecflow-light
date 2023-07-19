/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_EXCEPTION_H
#define ECFLOW_LIGHT_EXCEPTION_H

#include <sstream>

#include <eckit/exception/Exceptions.h>

namespace ecflow::light {

// *** Message *****************************************************************
// *****************************************************************************

class Message {
public:
    template <typename... ARGS>
    explicit Message(ARGS&&... args) : str_(stringify(std::forward<ARGS>(args)...)) {}

    [[nodiscard]] const std::string& str() const { return str_; }

private:
    template <typename... ARGS>
    static std::string stringify(ARGS&&... args) {
        std::ostringstream oss;
        ((oss << args), ...);
        return oss.str();
    }

    std::string str_;
};

// *** Exceptions **************************************************************
// *****************************************************************************

using eckit::BadValue;
using eckit::NotImplemented;

#define ECFLOW_LIGHT_THROW(EXCEPTION, MSG)  \
    do {                                    \
        throw EXCEPTION(MSG.str(), Here()); \
    } while (0)

}  // namespace ecflow::light

#endif
