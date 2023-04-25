/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_TRACE_HPP
#define ECFLOW_LIGHT_TRACE_HPP

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#define ECFLOW_LIGHT_TRACE_FUNCTION0 \
    ecflow::light::ScopeTrace tracer_instancer_(ecflow::light::Location(__FILE__, __LINE__), __func__)

#define ECFLOW_LIGHT_TRACE_FUNCTION(...) \
    ecflow::light::ScopeTrace tracer_instancer_(ecflow::light::Location(__FILE__, __LINE__), __func__, __VA_ARGS__)

namespace ecflow::light {

// *** Trace *******************************************************************
// *****************************************************************************

namespace Log {

// clang-format off
enum class Level
{
    None, // used only to disable logging
    Error,
    Warn,
    Info,
    Debug
};
// clang-format on

namespace /* __anonymous__ */ {

std::pair<const char*, Level> NamedLevels[] = {{"None", Level::None},
                                               {"Error", Level::Error},
                                               {"Warn", Level::Warn},
                                               {"Info", Level::Info},
                                               {"Debug", Level::Debug}};

Level from_string(const std::string& new_level_name) {
    auto found
        = std::find_if(std::begin(NamedLevels), std::end(NamedLevels),
                       [&new_level_name](const auto& named_level) { return new_level_name == named_level.first; });

    // If an unknown level requested, we default to DEBUG level
    if (found == std::end(NamedLevels)) {
        return Level::Debug;
    }

    return found->second;
}

}  // namespace

inline Level enabled_level = Level::None;

template <Level level, typename... ARGS>
inline void log(ARGS... args) {
    std::ostream& o = std::cout;  // TODO: Allow other logging mechanism

    if (level <= enabled_level) {
        o << Log::NamedLevels[static_cast<size_t>(level)].first << ": ";
        ((o << args), ...);
        o << std::endl;
    }
}

inline void set_level(const std::string& level_name) {
    enabled_level = from_string(level_name);
}

}  // namespace Log

// *** Location*****************************************************************
// *****************************************************************************

class Location {
public:
    Location(std::string file, uint32_t line) : file_{std::move(file)}, line_{line} {}

private:
    friend std::ostream& operator<<(std::ostream& o, const Location& l);

    std::string file_;
    uint32_t line_;
};

inline std::ostream& operator<<(std::ostream& o, const Location& l) {
    o << l.file_ << ":" << l.line_;
    return o;
}

// *** Trace *******************************************************************
// *****************************************************************************

class ScopeTrace {
public:
    template <typename... ARGS>
    ScopeTrace(Location location, std::string scope, ARGS... args) :
        location_{std::move(location)}, scope_{std::move(scope)} {

        Log::log<Log::Level::Debug>(begin_msg(location_, scope_, args...));
    }
    ~ScopeTrace() { Log::log<Log::Level::Debug>(make_end_msg(location_, scope_)); }

private:
    template <typename... ARGS>
    static std::string begin_msg(const Location& loc, const std::string& name, ARGS... args) {
        std::ostringstream oss;
        oss << "Entering " << name << "(";
        ((oss << "<" << std::forward<ARGS>(args) << ">"), ...);
        oss << ") at " << loc;
        return oss.str();
    }

    static std::string make_end_msg(const Location& loc, const std::string& name) {
        std::ostringstream oss;
        oss << "Exiting " << name << " at " << loc;
        return oss.str();
    }

    Location location_;
    std::string scope_;
};

}  // namespace ecflow::light

#endif
