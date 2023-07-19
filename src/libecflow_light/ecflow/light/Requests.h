/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_REQUESTS_H
#define ECFLOW_LIGHT_REQUESTS_H

#include "ecflow/light/Environment.h"
#include "ecflow/light/Options.h"

namespace ecflow::light {

// *** Request(s)***************************************************************
// *****************************************************************************

struct RequestMessage {
    virtual ~RequestMessage() = default;

    [[nodiscard]] virtual const Environment& environment() const = 0;
    [[nodiscard]] virtual const Options& options() const         = 0;

    virtual std::string str() = 0;
};

template <typename R>
struct DefaultRequestMessage : public RequestMessage {

    DefaultRequestMessage() : environment_{}, options_{} {}
    DefaultRequestMessage(Environment environment, Options options) :
        environment_{std::move(environment)}, options_{std::move(options)} {}

    [[nodiscard]] const Environment& environment() const final { return environment_; };
    [[nodiscard]] const Options& options() const final { return options_; };

    [[nodiscard]] std::string str() final { return static_cast<R*>(this)->as_string(); }

private:
    Environment environment_;
    Options options_;
};

struct CreateAttribute : DefaultRequestMessage<CreateAttribute> {

    CreateAttribute() : DefaultRequestMessage<CreateAttribute>{} {}
    CreateAttribute(Environment environment, Options options) :
        DefaultRequestMessage<CreateAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "CreateAttribute"; }
};

struct ReadAttribute : DefaultRequestMessage<ReadAttribute> {

    ReadAttribute() : DefaultRequestMessage<ReadAttribute>{} {}
    ReadAttribute(Environment environment, Options options) :
        DefaultRequestMessage<ReadAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "ReadAttribute"; }
};

struct UpdateAttribute : DefaultRequestMessage<UpdateAttribute> {

    UpdateAttribute() : DefaultRequestMessage<UpdateAttribute>{} {}
    UpdateAttribute(Environment environment, Options options) :
        DefaultRequestMessage<UpdateAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "UpdateAttribute"; }
};

struct DeleteAttribute : DefaultRequestMessage<DeleteAttribute> {

    DeleteAttribute() : DefaultRequestMessage<DeleteAttribute>{} {}
    DeleteAttribute(Environment environment, Options options) :
        DefaultRequestMessage<DeleteAttribute>{std::move(environment), std::move(options)} {}

    [[nodiscard]] std::string as_string() const { return "DeleteAttribute"; }
};

struct Request final {
    template <typename M, typename... ARGS>
    static Request make_request(ARGS&&... args) {
        return Request{std::make_unique<M>(std::forward<ARGS>(args)...)};
    }

    [[nodiscard]] std::string str() const { return message_->str(); }

    [[nodiscard]] std::string get_environment(const std::string& name) const {
        return message_->environment().get(name).value;
    }
    [[nodiscard]] std::string get_option(const std::string& name) const { return message_->options().get(name).value; }

private:
    explicit Request(std::unique_ptr<RequestMessage>&& message) : message_{std::move(message)} {}

    std::unique_ptr<RequestMessage> message_;
};

// *** Response(s) *************************************************************
// *****************************************************************************

struct Response final {
    std::string response;
};

std::ostream& operator<<(std::ostream& o, const Response& response);

}  // namespace ecflow::light

#endif
