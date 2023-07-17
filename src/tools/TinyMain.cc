/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */


#include <cstdio>
#include <iostream>

#include "ecflow/light/TinyCURL.hpp"

int main(int argc [[maybe_unused]], char** argv [[maybe_unused]]) {

    std::cout << "In TinyMain...\n\n";

    try {
        ecflow::light::net::TinyCURL curl;
        // curl.get(URL("https://localhost:8080/v1/suites/tree"));
        // curl.get(URL("https://localhost:8080/v1/suites/mamb/practice/f1/t1/attributes"), "");
        curl.put(ecflow::light::net::URL("https://localhost:8080/v1/suites/mamb/practice/f1/t1/attributes"),
                 R"({"type":"event","name":"task_event","value":"clear"})");
        // curl.post(URL("https://localhost:8080/v1/suites/mamb/practice/f1/t1/attributes"), "");
    }
    catch (...) {
        std::cout << "Error: Unknown problem detected.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
