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
#include <thread>

#include "ecflow/light/API.hpp"

namespace ecfl = ecflow::light;

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Error: Incorrect number of arguments.\n\n"
                     "  Usage: ecflow_light_multiple_client <n_steps> <step_size>]\n\n";
        return EXIT_FAILURE;
    }

    int n_steps   = atoi(argv[1]);
    int step_size = atoi(argv[2]);

    for (int i = 0; i < n_steps; ++i) {
        int minute = (i / 60);
        int second = (i % 60);

        std::cout << ">>> minute: " << minute << ", second: " << second << std::endl;

        // send meter
        int meter_value = second;
        ecfl::update_meter("task_meter", meter_value);

        // send label
        std::string label_value = "Going on <" + std::to_string(minute) + "><" + std::to_string(second) + ">";
        ecfl::update_label("task_label", label_value);

        // send event
        bool event_value = (second % 2 == 0);
        ecfl::update_event("task_event", event_value);

        std::this_thread::sleep_for(std::chrono::seconds(step_size));
    }

    return EXIT_SUCCESS;
}
