/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecflow/light/API.h"
#include "ecflow/light/Version.h"

static int are_equal(const char* fst, const char* snd) {
    return strcmp(fst, snd) == 0;
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("Error: Incorrect number of arguments.\n\n");
        printf("  Usage: ecflow_light_client --(meter|label|event) <name> <value>\n\n");
        return EXIT_FAILURE;
    }

    printf("\n  Using ecFlow Light (%s)\n\n", ecflow_light_version());

    const char* option = argv[1];
    const char* name   = argv[2];
    const char* value  = argv[3];
    if (are_equal(option, "--meter")) {
        int meter_value = atoi(value);
        ecflow_light_update_meter(name, meter_value);
    }
    else if (are_equal(option, "--label")) {
        ecflow_light_update_label(name, value);
    }
    else if (are_equal(option, "--event")) {
        if (!are_equal(value, "set") && !are_equal(value, "clear")) {
            printf("Error: Incorrect value. Expected either 'set' or 'clear'\n\n");
            return EXIT_FAILURE;
        }

        int event_value = (strcmp(value, "set") == 0);

        ecflow_light_update_event(name, event_value);
    }
    else {
        printf("Error: Unknown option.\n\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
