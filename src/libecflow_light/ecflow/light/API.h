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

/**
 * Informs the ecFlow server that the named meter has been updated to the given value.
 *
 * @param name the name of the meter to be updated
 * @param value the new value of the meter (i.e. an integer, expected to be in the meter range)
 * @return EXIT_FAILURE if communication failed; EXIT_SUCCESS, otherwise
 */
int ecflow_light_update_meter(const char* name, int value);

/**
 * Informs the ecFlow server that the named label has been updated to the given value.
 *
 * @param name the name of the label to be updated
 * @param value the new value of the label (i.e. a string)
 * @return EXIT_FAILURE if communication failed; EXIT_SUCCESS, otherwise
 */
int ecflow_light_update_label(const char* name, const char* value);

/**
 * Informs the ecFlow server that the named event has been updated to the given value.
 *
 * @param name the name of the label to be updated
 * @param value the new value of the label (i.e. 0 to clear the event; any other value to set the event)
 * @return EXIT_FAILURE if communication failed; EXIT_SUCCESS, otherwise
 */
int ecflow_light_update_event(const char* name, int value);

#if defined(__cplusplus)
}
#endif

#endif
