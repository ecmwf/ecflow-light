/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <thread>

#include "ecflow/light/ClientAPI.h"
#include "ecflow/light/Conversion.h"
#include "ecflow/light/Environment.h"
#include "ecflow/light/InternalAPI.h"
#include "ecflow/light/Log.h"
#include "ecflow/light/Options.h"
#include "ecflow/light/Requests.h"
#include "ecflow/light/Version.h"

#include <eckit/option/CmdArgs.h>
#include <eckit/option/MultiValueOption.h>
#include <eckit/option/Option.h>
#include <eckit/option/SimpleOption.h>
#include <eckit/runtime/Tool.h>

namespace ecfl = ecflow::light;

class ClientTool final : public eckit::Tool {
public:
    using options_t = std::vector<eckit::option::Option*>;

    static void print_usage(const std::string& name) { ecfl::Log::info() << "USAGE! " << name << "\n"; }

public:
    ClientTool(int argc, char* argv[]) : eckit::Tool(argc, argv) {}

    void run() final {

        // Important!
        //
        //  *** `new` cannot be avoided! ***
        //
        // Because CmdArgs is responsible for deleting the instances in `options`,
        // it is not possible to put these in a std::unique_ptr<eckit::Option> to
        // automatically destroy the options when leaving the scope
        //
        options_t options = {
            new eckit::option::SimpleOption<bool>("version", "Display version information"),
            new eckit::option::MultiValueOption("label", "Update label [label name: string] [label value: string]", 2),
            new eckit::option::MultiValueOption("meter", "Update meter [meter name: string] [meter value: integer]", 2),
            new eckit::option::MultiValueOption(
                "event", "Update event [event name: string] ([event value: 'set' or 'clear'])", 1, 1),
            new eckit::option::SimpleOption<std::string>("init", "Signal task initialisation [process id: string]"),
            new eckit::option::SimpleOption<bool>("complete", "Signal task completion"),
            new eckit::option::SimpleOption<std::string>("abort", "Signal task abortion [reason: string]")};

        eckit::option::CmdArgs args(print_usage, options, 0, 0);

        if (args.has("version")) {
            ecfl::Log::info() << "\n  Using ecFlow Light (" << ecflow_light_version() << ")\n\n";
            return;
        }

        handle_meter_option(args);
        handle_label_option(args);
        handle_event_option(args);
        handle_init_option(args);
        handle_complete_option(args);
        handle_abort_option(args);
    }

private:
    static void handle_meter_option(const eckit::option::CmdArgs& args) {
        using option_t = std::vector<std::string>;
        auto option    = get_option<option_t>(args, "meter");
        if (option) {
            const option_t& arguments = option.value();
            ASSERT(arguments.size() == 2);

            std::string meter_name  = arguments[0];
            std::string meter_value = arguments[1];

            auto actual_value = ecfl::convert_to<int>(meter_value);

            auto error = ecfl::update_meter(meter_name, actual_value);
            ecfl::Log::debug() << "Request 'update_meter' processed. Result: " << error << "\n";
        }
    }

    static void handle_label_option(const eckit::option::CmdArgs& args) {
        using option_t = std::vector<std::string>;
        auto option    = get_option<option_t>(args, "label");
        if (option) {
            const option_t& arguments = option.value();
            ASSERT(arguments.size() == 2);

            std::string label_name  = arguments[0];
            std::string label_value = arguments[1];

            auto error = ecfl::update_label(label_name, label_value);
            ecfl::Log::debug() << "Request 'update_label' processed. Result: " << error << "\n";
        }
    }

    static void handle_event_option(const eckit::option::CmdArgs& args) {
        using option_t = std::vector<std::string>;
        auto option    = get_option<option_t>(args, "event");
        if (option) {
            const option_t& arguments = option.value();
            ASSERT(arguments.size() >= 1);

            std::string event_name  = arguments[0];
            std::string event_value = "set";
            if (arguments.size() > 1) {
                event_value = arguments[1];
            }

            int actual_value;
            if (event_value == "clear") {
                actual_value = 0;
            }
            else if (event_value == "" || event_value == "set") {
                actual_value = 1;
            }
            else {
                ECFLOW_LIGHT_THROW(eckit::BadValue, ecfl::Message("Incorrect event value '", event_value,
                                                                  "' found. Expected either 'set' or 'clear'"));
            }

            auto error = ecfl::update_event(event_name, actual_value);
            ecfl::Log::debug() << "Request 'update_event' processed. Result: " << error << "\n";
        }
    }

    static void handle_init_option(const eckit::option::CmdArgs& args) {
        auto option = get_option<std::string>(args, "init");
        if (option) {
            try {
                const ecfl::Environment& environment = ecfl::Environment::environment();

                ecfl::Options options = ecfl::Options::options().with("action", "init");

                ecfl::Request request = ecfl::Request::make_request<ecfl::UpdateNodeStatus>(environment, options);

                ecfl::Response response = ecfl::ConfiguredClient::instance().process(request);

                ecfl::Log::debug() << "Response: " << response << std::endl;
            }
            catch (eckit::Exception& e) {
                ecfl::Log::error() << "Error detected: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (...) {
                ecfl::Log::error() << "Unknown error detected" << std::endl;
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    static void handle_complete_option(const eckit::option::CmdArgs& args) {
        auto option = get_option<std::string>(args, "complete");
        if (option) {
            try {
                const ecfl::Environment& environment = ecfl::Environment::environment();

                ecfl::Options options = ecfl::Options::options().with("action", "complete");

                ecfl::Request request = ecfl::Request::make_request<ecfl::UpdateNodeStatus>(environment, options);

                ecfl::Response response = ecfl::ConfiguredClient::instance().process(request);

                ecfl::Log::debug() << "Response: " << response << std::endl;
            }
            catch (eckit::Exception& e) {
                ecfl::Log::error() << "Error detected: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (...) {
                ecfl::Log::error() << "Unknown error detected" << std::endl;
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    static void handle_abort_option(const eckit::option::CmdArgs& args) {
        auto option = get_option<std::string>(args, "abort");
        if (option) {
            try {
                const ecfl::Environment& environment = ecfl::Environment::environment();

                ecfl::Options options =
                    ecfl::Options::options().with("action", "abort").with("abort_why", option.value());

                ecfl::Request request = ecfl::Request::make_request<ecfl::UpdateNodeStatus>(environment, options);

                ecfl::Response response = ecfl::ConfiguredClient::instance().process(request);

                ecfl::Log::debug() << "Response: " << response << std::endl;
            }
            catch (eckit::Exception& e) {
                ecfl::Log::error() << "Error detected: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (...) {
                ecfl::Log::error() << "Unknown error detected" << std::endl;
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    template <typename T>
    static std::optional<T> get_option(const eckit::option::CmdArgs& args, const std::string& option_name) {
        if (args.has(option_name)) {
            // Retrieve option value
            T option_value;
            args.get(option_name, option_value);

            return {option_value};
        }

        return std::nullopt;
    }
};

int main(int argc, char* argv[]) {
    try {
        ClientTool client(argc, argv);
        return client.start();
    }
    catch (...) {
        ecfl::Log::error() << "Error: Unknown problem detected.\n\n";
        return EXIT_FAILURE;
    }
}
