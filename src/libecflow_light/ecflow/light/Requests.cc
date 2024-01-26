/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/light/Requests.h"

namespace ecflow::light {

// *** Request(s) **************************************************************
// *****************************************************************************

void UpdateNodeStatus::call_dispatch(RequestDispatcher& dispatcher) const {
    dispatcher.dispatch_request(*this);
}

void UpdateNodeAttribute::call_dispatch(RequestDispatcher& dispatcher) const {
    dispatcher.dispatch_request(*this);
}

// *** Response(s) *************************************************************
// *****************************************************************************

std::ostream& operator<<(std::ostream& o, const Response& response) {
    o << "{" << response.response << "}";
    return o;
}

}  // namespace ecflow::light
