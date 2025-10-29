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

#include "ecflow/light/ClientAPI.h"

namespace ecfl = ecflow::light;


namespace ecflow::light::testing {

CASE("test_environment__can_handle_cached_environment_variables") {
    Environment environment = Environment::an_environment()
                                  .with("ECF_RID", "12345")
                                  .with("ECF_NAME", "/path/to/task")
                                  .with("ECF_PASS", "custom_password");

    auto rid = environment.get_optional("ECF_RID");
    EXPECT(rid && rid->value == "12345");
    auto name = environment.get_optional("ECF_NAME");
    EXPECT(name && name->value == "/path/to/task");
    auto pass = environment.get_optional("ECF_PASS");
    EXPECT(pass && pass->value == "custom_password");
}

CASE("test_environment__can_handle_non_cached_environment_variables") {
    Environment environment = Environment::an_environment()
                                  .with("ECF_RID", "12345")
                                  .with("ECF_NAME", "/path/to/task")
                                  .with("ECF_PASS", "custom_password");

    auto nonexistent = environment.get_optional("__NONEXISTENT__");
    EXPECT(!nonexistent);
}

CASE("test_environment__can_replace_environment_variables") {
    Environment environment = Environment::an_environment()
                                  .with("ECF_RID", "12345")
                                  .with("ECF_NAME", "/path/to/task")
                                  .with("ECF_PASS", "custom_password");

    {
        // No replacement actually necessary
        auto result = replace_env_var("somevalue", environment);
        EXPECT(result == "somevalue");
    }
    {
        // Replace variable, based on 'cached' environment
        auto result = replace_env_var("$ENV{ECF_NAME}", environment);
        EXPECT(result == "/path/to/task");
    }
    {
        // Replace variable, based on 'real' environment
        // The following 'SOME_VARIABLE' is set on the environment by CMake
        auto result = replace_env_var("$ENV{ECF_SOME_VARIABLE}", environment);
        EXPECT(result == "1500");
    }
}

}  // namespace ecflow::light::testing

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
