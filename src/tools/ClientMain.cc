/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <iostream>

#include "ecflow/light/Conversion.h"
#include "ecflow/light/LightAPI.h"

namespace ecfl = ecflow::light;

int main(int argc, char* argv[]) {

    if (argc != 4) {
        std::cout << "Error: Incorrect number of arguments.\n\n"
                     "  Usage: ecflow_light_client --(meter|label|event) <name> <value>\n\n";
        return EXIT_FAILURE;
    }

    ecfl::init();

    std::string option = argv[1];
    std::string name   = argv[2];
    std::string value  = argv[3];
    if (option == "--meter") {
        int meter_value = ecfl::convert_to<int>(value);
        ecfl::child_update_meter(name, meter_value);
    }
    else if (option == "--label") {
        ecfl::child_update_label(name, value);
    }
    else if (option == "--event") {
        if (value != "set" && value != "clear") {
            std::cout << "Error: Incorrect value. Expected either 'set' or 'clear'\n\n";
            return EXIT_FAILURE;
        }

        bool event_value = (value == "set");

        ecfl::child_update_event(name, event_value);
    }
    else {
        std::cout << "Error: Unknown option.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
