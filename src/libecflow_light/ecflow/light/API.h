/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_API_H
#define ECFLOW_LIGHT_API_H

#if defined(__cplusplus)
extern "C" {
#endif

int ecflow_light_update_meter(const char* name, int value);

int ecflow_light_update_label(const char* name, const char* value);

int ecflow_light_update_event(const char* name, int value);

#if defined(__cplusplus)
}
#endif

#endif
