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
#include "ecflow/light/Dispatcher.h"
#include "ecflow/light/Options.h"
#include "ecflow/light/Requests.h"

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

CASE("test_api__can_set_event") {
    using namespace ecflow::light;

    std::string name = "event";
    bool value       = true;

    Options options = Options::options().with("command", "event").with("name", name).with("value", value ? "1" : "0");

    auto env = Environment::an_environment()
                   .with("ECF_NAME", "/path/to/task")
                   .with("ECF_PASS", "qwerty")
                   .with("ECF_TRYNO", "0")
                   .with("ECF_RID", "12345");

    auto request = UpdateNodeAttribute(env, options);

    UDPDispatcher dispatcher(ClientCfg::make_empty());

    auto contents = dispatcher.format_request(UpdateNodeAttribute(env, options));

    EXPECT(
        contents ==
        R"({"method":"put","version":"","header":{"task_rid":"12345","task_password":"qwerty","task_try_no":0},"payload":{"command":"event","path":"/path/to/task","name":"event","value":"1"}})");
}

CASE("test_api__can_clear_event") {
    using namespace ecflow::light;

    std::string name = "event";
    bool value       = false;

    Options options = Options::options().with("command", "event").with("name", name).with("value", value ? "1" : "0");

    auto env = Environment::an_environment()
                   .with("ECF_NAME", "/path/to/task")
                   .with("ECF_PASS", "qwerty")
                   .with("ECF_TRYNO", "0")
                   .with("ECF_RID", "12345");

    auto request = UpdateNodeAttribute(env, options);

    UDPDispatcher dispatcher(ClientCfg::make_empty());

    auto contents = dispatcher.format_request(UpdateNodeAttribute(env, options));

    EXPECT(
        contents ==
        R"({"method":"put","version":"","header":{"task_rid":"12345","task_password":"qwerty","task_try_no":0},"payload":{"command":"event","path":"/path/to/task","name":"event","value":"0"}})");
}

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
