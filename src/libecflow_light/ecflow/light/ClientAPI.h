/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_CLIENTAPI_H
#define ECFLOW_LIGHT_CLIENTAPI_H

#include <string>

namespace ecflow::light {

/// Initialize library based on given configuration (i.e. ECF_UDP_HOST, ECF_UDP_PORT)
void init();

int child_update_meter(const std::string& name, int value);
int child_update_label(const std::string& name, const std::string& value);
int child_update_event(const std::string& name, bool value);

}  // namespace ecflow::light

#endif
