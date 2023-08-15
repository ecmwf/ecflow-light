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

#include "ecflow/light/Configuration.h"
#include "ecflow/light/Environment.h"
#include "ecflow/light/Options.h"
#include "ecflow/light/TinyREST.h"

namespace ecflow::light {

struct RequestDispatcher;

// *** Request(s)***************************************************************
// *****************************************************************************

struct RequestMessage {
    virtual ~RequestMessage() = default;

    [[nodiscard]] virtual const Environment& environment() const = 0;
    [[nodiscard]] virtual const Options& options() const         = 0;

    virtual std::string description() = 0;

    virtual void dispatch(RequestDispatcher& dispatcher) const = 0;
};

template <typename R>
struct DefaultRequestMessage : public RequestMessage {

    DefaultRequestMessage() : environment_{}, options_{} {}
    DefaultRequestMessage(Environment environment, Options options) :
        environment_{std::move(environment)}, options_{std::move(options)} {}

    [[nodiscard]] const Environment& environment() const final { return environment_; };
    [[nodiscard]] const Options& options() const final { return options_; };

    [[nodiscard]] std::string description() final { return static_cast<R*>(this)->as_string(); }

    void dispatch(RequestDispatcher& dispatcher) const final {
        return static_cast<const R*>(this)->call_dispatch(dispatcher);
    }

private:
    Environment environment_;
    Options options_;
};

struct UpdateNodeStatus : DefaultRequestMessage<UpdateNodeStatus> {

    UpdateNodeStatus() : DefaultRequestMessage<UpdateNodeStatus> {}
    {}
    UpdateNodeStatus(Environment environment, Options options) : DefaultRequestMessage<UpdateNodeStatus> {
        std::move(environment), std::move(options)
    }
    {}

    [[nodiscard]] std::string as_string() const {
        return Message("UpdateNodeStatus: new_status=?, at node=", environment().get("ECF_NAME").value).str();
    }

    void call_dispatch(RequestDispatcher& dispatcher) const;
};

struct UpdateNodeAttribute : DefaultRequestMessage<UpdateNodeAttribute> {

    UpdateNodeAttribute() : DefaultRequestMessage<UpdateNodeAttribute> {}
    {}
    UpdateNodeAttribute(Environment environment, Options options) : DefaultRequestMessage<UpdateNodeAttribute> {
        std::move(environment), std::move(options)
    }
    {}

    [[nodiscard]] std::string as_string() const {
        return Message("UpdateNodeAttribute: name=", options().get("name").value,
                       ", value=", options().get("value").value, ", at node=", environment().get("ECF_NAME").value)
            .str();
    }

    void call_dispatch(RequestDispatcher& dispatcher) const;
};

struct RequestDispatcher {
    virtual ~RequestDispatcher() = default;

    virtual void dispatch_request(const UpdateNodeStatus& request)    = 0;
    virtual void dispatch_request(const UpdateNodeAttribute& request) = 0;
};

struct Request final {
public:
    template <typename M, typename... ARGS>
    static Request make_request(ARGS&&... args) {
        return Request{std::make_unique<M>(std::forward<ARGS>(args)...)};
    }

    [[nodiscard]] std::string description() const { return message_->description(); }

    [[nodiscard]] std::string get_environment(const std::string& name) const {
        return message_->environment().get(name).value;
    }
    [[nodiscard]] std::string get_option(const std::string& name) const { return message_->options().get(name).value; }

    void dispatch(RequestDispatcher& dispatcher) const { return message_->dispatch(dispatcher); }

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
