/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <eckit/testing/Test.h>

#include "ecflow/light/API.h"


CASE("test_api__fails_when_passed_null_string_parameter") {
    {
        auto ret = ecflow_light_update_event(nullptr, 42);
        EXPECT(ret == EXIT_FAILURE);
    }
    {
        auto ret = ecflow_light_update_label(nullptr, "label-value");
        EXPECT(ret == EXIT_FAILURE);
    }
    {
        auto ret = ecflow_light_update_label("label-name", nullptr);
        EXPECT(ret == EXIT_FAILURE);
    }
    {
        auto ret = ecflow_light_update_meter(nullptr, 0);
        EXPECT(ret == EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
