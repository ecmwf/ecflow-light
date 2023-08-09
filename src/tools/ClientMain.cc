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
#include "ecflow/light/Options.h"
#include "ecflow/light/Requests.h"
#include "ecflow/light/Version.h"

#include <eckit/option/CmdArgs.h>
#include <eckit/option/Option.h>
#include <eckit/option/SimpleOption.h>
#include <eckit/runtime/Tool.h>

namespace ecfl = ecflow::light;

class ClientTool final : public eckit::Tool {
public:
    using options_t = std::vector<eckit::option::Option*>;

    static void print_usage(const std::string& name) { std::cout << "USAGE! " << name << "\n"; }

private:
    class Counter {
    public:
        using counter_t = int;

    public:
        explicit Counter(counter_t value, counter_t cycle = std::numeric_limits<counter_t>::max()) :
            value_{value}, cycle_{cycle} {}

        [[nodiscard]] counter_t value() const { return value_ + 1; }
        [[nodiscard]] counter_t cycle_number() const { return (value_ / cycle_) + 1; }
        [[nodiscard]] counter_t cycle_counter() const { return (value_ % cycle_) + 1; }

    private:
        int value_;
        int cycle_;
    };

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
            new eckit::option::SimpleOption<long>("iterations", "Number of iterations [default: 1]"),
            new eckit::option::SimpleOption<long>("wait", "Time to wait between iterations [ms; default: 1000ms]"),
            new eckit::option::SimpleOption<long>("cycle", "Cycle length [default: maxint]"),
            new eckit::option::SimpleOption<std::string>("label", "Name:Value of the label [value: a string]"),
            new eckit::option::SimpleOption<std::string>("meter", "Name:Value of the meter [value: an integer]"),
            new eckit::option::SimpleOption<std::string>("event", "Name:Value of the event [value: 0 or 1]"),
            new eckit::option::SimpleOption<std::string>("init", "Process ID"),
            new eckit::option::SimpleOption<bool>("complete", "X"),
            new eckit::option::SimpleOption<std::string>("abort", "Reason")};

        eckit::option::CmdArgs args(print_usage, options, 0, 0);

        if (args.has("version")) {
            std::cout << "\n  Using ecFlow Light (" << ecflow_light_version() << ")\n\n";
            return;
        }

        //
        // The following lambda is used to retrieve the option's value, or a provided default value.
        //
        auto getter = [&args](const std::string& name, auto default_value) -> decltype(default_value) {
            decltype(default_value) value = default_value;
            if (args.has(name)) {
                args.get(name, value);
            }
            return value;
        };

        long iterations = getter("iterations", 1L);
        long wait       = getter("wait", 1000L);
        int cycle       = getter("cycle", std::numeric_limits<int>::max());

        for (int i = 0; i < iterations; ++i) {
            Counter counter(i, cycle);
            handle_meter_option(args, counter);
            handle_label_option(args, counter);
            handle_event_option(args, counter);
            handle_init_option(args);
            handle_complete_option(args);
            handle_abort_option(args);

            // Don't sleep if on the last iteration!
            if (i + 1 < iterations) {
                std::this_thread::sleep_for(std::chrono::milliseconds(wait));
            }
        }
    }

private:
    static void handle_meter_option(const eckit::option::CmdArgs& args, Counter counter) {
        auto option = get_option(args, "meter");
        if (option) {
            std::string meter_name  = option->first;
            std::string meter_value = interpolate(option->second, counter);

            auto actual_value = ecfl::convert_to<int>(meter_value);

            auto error = ecfl::update_meter(meter_name, actual_value);
            std::cout << "Request 'update_meter' processed. Result: " << error << "\n";
        }
    }

    static void handle_label_option(const eckit::option::CmdArgs& args, Counter counter) {
        auto option = get_option(args, "label");
        if (option) {
            std::string label_name  = option->first;
            std::string label_value = interpolate(option->second, counter);

            auto error = ecfl::update_label(label_name, label_value);
            std::cout << "Request 'update_label' processed. Result: " << error << "\n";
        }
    }

    static void handle_event_option(const eckit::option::CmdArgs& args, Counter counter) {
        auto option = get_option(args, "event");
        if (option) {
            std::string event_name  = option->first;
            std::string event_value = interpolate(option->second, counter);

            auto actual_value = ecfl::convert_to<int>(event_value);
            actual_value %= 2;

            auto error = ecfl::update_event(event_name, actual_value);
            std::cout << "Request 'update_event' processed. Result: " << error << "\n";
        }
    }

    static void handle_init_option(const eckit::option::CmdArgs& args) {
        auto option = get_option0(args, "init");
        if (option) {
            try {
                const ecfl::Environment& environment = ecfl::Environment::environment();

                ecfl::Options options = ecfl::Options::options().with("action", option->first);

                ecfl::Request request = ecfl::Request::make_request<ecfl::UpdateNodeStatus>(environment, options);

                ecfl::Response response = ecfl::ConfiguredClient::instance().process(request);

                std::cout << "Response: " << response << std::endl;
            }
            catch (eckit::Exception& e) {
                std::cout << "Error detected: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (...) {
                std::cout << "Unknown error detected" << std::endl;
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    static void handle_complete_option(const eckit::option::CmdArgs& args) {
        auto option = get_option0(args, "complete");
        if (option) {
            try {
                const ecfl::Environment& environment = ecfl::Environment::environment();

                ecfl::Options options = ecfl::Options::options().with("action", option->first);

                ecfl::Request request = ecfl::Request::make_request<ecfl::UpdateNodeStatus>(environment, options);

                ecfl::Response response = ecfl::ConfiguredClient::instance().process(request);

                std::cout << "Response: " << response << std::endl;
            }
            catch (eckit::Exception& e) {
                std::cout << "Error detected: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (...) {
                std::cout << "Unknown error detected" << std::endl;
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    static void handle_abort_option(const eckit::option::CmdArgs& args) {
        auto option = get_option0(args, "abort");
        if (option) {
            try {
                const ecfl::Environment& environment = ecfl::Environment::environment();

                ecfl::Options options =
                    ecfl::Options::options().with("action", option->first).with("abort_why", option->second);

                ecfl::Request request = ecfl::Request::make_request<ecfl::UpdateNodeStatus>(environment, options);

                ecfl::Response response = ecfl::ConfiguredClient::instance().process(request);

                std::cout << "Response: " << response << std::endl;
            }
            catch (eckit::Exception& e) {
                std::cout << "Error detected: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (...) {
                std::cout << "Unknown error detected" << std::endl;
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    static std::optional<std::pair<std::string, std::string>> get_option(const eckit::option::CmdArgs& args,
                                                                         const std::string& option_name) {
        if (args.has(option_name)) {
            // Retrieve option value
            std::string option_value;
            args.get(option_name, option_value);

            // Find location of separator of "name:value"
            auto found = std::find(option_value.begin(), option_value.end(), ':');
            if (found == option_value.end()) {
                return std::nullopt;
            }

            // Split "name:value"
            auto size         = std::distance(option_value.begin(), found);
            std::string name  = option_value.substr(0, size);
            std::string value = option_value.substr(size + 1);

            return {std::make_pair(name, value)};
        }

        return std::nullopt;
    }

    static std::optional<std::pair<std::string, std::string>> get_option0(const eckit::option::CmdArgs& args,
                                                                          const std::string& option_name) {
        if (args.has(option_name)) {
            // Retrieve option value
            std::string option_value;
            args.get(option_name, option_value);

            return {std::make_pair(option_name, option_value)};
        }

        return std::nullopt;
    }

    static std::string interpolate(const std::string& in, Counter counter) {
        std::string out = in;
        replace_all(out, "#cycle-number#", ecfl::convert_to<std::string>(counter.cycle_number()));
        replace_all(out, "#cycle-counter#", ecfl::convert_to<std::string>(counter.cycle_counter()));
        replace_all(out, "#counter#", ecfl::convert_to<std::string>(counter.value()));
        return out;
    }

    static std::size_t replace_all(std::string& inout, const std::string& what, const std::string& with) {
        std::size_t count{};
        for (std::string::size_type pos{}; std::string::npos != (pos = inout.find(what.data(), pos, what.length()));
             pos += with.length(), ++count)
            inout.replace(pos, what.length(), with.data(), with.length());
        return count;
    }
};

int main(int argc, char* argv[]) {
    try {
        ClientTool client(argc, argv);
        return client.start();
    }
    catch (...) {
        std::cout << "Error: Unknown problem detected.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
