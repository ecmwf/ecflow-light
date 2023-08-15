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

struct MockUDPDispatcher {
    static Response dispatch_request(const ClientCfg& cfg, const std::string& request) {
        MockUDPDispatcher::client  = cfg;
        MockUDPDispatcher::request = request;

        return Response{"OK"};
    }

    static ClientCfg client;
    static std::string request;
};

ClientCfg MockUDPDispatcher::client = ClientCfg::make_empty();
std::string MockUDPDispatcher::request;

CASE("test_udp_client__uses_provided_configuration_to_build_request") {
    ClientCfg cfg =
        ClientCfg::make_cfg(ClientCfg::KindPhony, ClientCfg::ProtocolNone, "custom_hostname", "custom_port", "99.0");

    Environment environment = Environment::an_environment()
                                  .with("ECF_RID", "12345")
                                  .with("ECF_NAME", "/path/to/task")
                                  .with("ECF_PASS", "custom_password")
                                  .with("ECF_TRYNO", "2");

//    {
//        Options options = Options::options().with("command", "meter").with("name", "meter_name").with("value", "42");
//        Request request = Request::make_request<UpdateNodeAttribute>(environment, options);
//
//        ecfl::BaseClientAPI<MockUDPDispatcher, UDPRequestBuilder> client(cfg, environment);
//        Response response = client.process(request);
//
//        EXPECT(response.response == "OK");
//
//        EXPECT(MockUDPDispatcher::client.host == cfg.host);
//        EXPECT(MockUDPDispatcher::client.port == cfg.port);
//
//        EXPECT(MockUDPDispatcher::request.find(R"("task_rid":"custom_rid")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("task_password":"custom_password")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("task_try_no":2)") != std::string::npos);
//
//        EXPECT(MockUDPDispatcher::request.find(R"("path":"/path/to/task")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("name":"meter_name")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("value":"42")") != std::string::npos);
//    }
//    {
//        Options options =
//            Options::options().with("command", "label").with("name", "label_name").with("value", "label_text");
//        Request request = Request::make_request<UpdateNodeAttribute>(environment, options);
//
//        ecfl::BaseClientAPI<MockUDPDispatcher, UDPRequestBuilder> client(cfg, environment);
//        Response response = client.process(request);
//
//        EXPECT(response.response == "OK");
//
//        EXPECT(MockUDPDispatcher::client.host == cfg.host);
//        EXPECT(MockUDPDispatcher::client.port == cfg.port);
//
//        EXPECT(MockUDPDispatcher::request.find(R"("task_rid":"custom_rid")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("path":"/path/to/task")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("task_password":"custom_password")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("task_try_no":2)") != std::string::npos);
//
//        EXPECT(MockUDPDispatcher::request.find(R"("path":"/path/to/task")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("name":"label_name")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("value":"label_text")") != std::string::npos);
//    }
//    {
//        Options options = Options::options().with("command", "event").with("name", "event_name").with("value", "true");
//        Request request = Request::make_request<UpdateNodeAttribute>(environment, options);
//
//        ecfl::BaseClientAPI<MockUDPDispatcher, UDPRequestBuilder> client(cfg, environment);
//        Response response = client.process(request);
//
//        EXPECT(response.response == "OK");
//
//        EXPECT(MockUDPDispatcher::client.host == cfg.host);
//        EXPECT(MockUDPDispatcher::client.port == cfg.port);
//
//        EXPECT(MockUDPDispatcher::request.find(R"("task_rid":"custom_rid")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("task_password":"custom_password")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("task_try_no":2)") != std::string::npos);
//
//        EXPECT(MockUDPDispatcher::request.find(R"("path":"/path/to/task")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("name":"event_name")") != std::string::npos);
//        EXPECT(MockUDPDispatcher::request.find(R"("value":"1")") != std::string::npos);
//    }
}

}  // namespace ecflow::light::testing

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
