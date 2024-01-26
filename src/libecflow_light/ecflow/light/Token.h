/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_TOKEN_H
#define ECFLOW_LIGHT_TOKEN_H

#include <fstream>
#include <string>

#include "ecflow/light/Environment.h"
#include "ecflow/light/Filesystem.h"
#include "ecflow/light/Log.h"

#include "eckit/parser/JSONParser.h"


namespace ecflow::light {

struct UnableToLoadSecretToken : public eckit::Exception {
    UnableToLoadSecretToken(const std::string& msg, const eckit::CodeLocation& loc) :
        eckit::Exception(msg, loc, true) {}
    UnableToLoadSecretToken(const std::string& msg, const eckit::Exception& reason, const eckit::CodeLocation& loc) :
        eckit::Exception(Message(msg, ", due to: ", reason.what()).str(), loc, true) {}
};

struct Token {
    std::string url;
    std::string key;
    std::string email;
};

struct Tokens {
public:
    using storage_t = std::vector<Token>;

    Tokens() : Tokens(Tokens::load()) {}

    std::optional<Token> secret(const std::string& url) {
        try {
            auto found = std::find_if(std::begin(tokens_), std::end(tokens_),
                                      [&url](const Token& token) { return token.url == url; });
            if (found == std::end(tokens_)) {
                Log::error() << "No secret token found for URL: " << url;
                return {};
            }

            return *found;
        }
        catch (const eckit::Exception& e) {
            Log::error() << "Unable to load secret token, due to: " << e.what();
            return {};
        }
    }

private:
    static storage_t load() {
        auto environment = Environment().from_environment("HOME");

        auto home = environment.get_optional("HOME");
        if (home) {
            auto home_var      = home.value();
            fs::path home_path = home_var.value;
            auto cfg_path      = home_path / ".ecflowrc" / "ssl" / "api-tokens.json";

            std::ifstream ifs(cfg_path);
            if (!ifs.is_open()) {
                ECFLOW_LIGHT_THROW(UnableToLoadSecretToken, Message("Unable to open file: '", cfg_path, "'"));
            }

            return Tokens::read(ifs);
        }

        ECFLOW_LIGHT_THROW(EnvironmentVariableNotFound, Message("Unable to find environment variable 'HOME'"));
    }

    static storage_t read(std::ifstream& ifs) {
        auto parser      = eckit::JSONParser(ifs);
        auto token_array = parser.parse();

        storage_t tokens;
        for (size_t i = 0; i != token_array.size(); ++i) {
            auto token_value = token_array[i];
            auto token_url   = token_value["url"].as<std::string>();
            auto token_key   = token_value["key"].as<std::string>();
            auto token_email = token_value["email"].as<std::string>();
            tokens.push_back(Token{token_url, token_key, token_email});
        }
        return tokens;
    }

private:
    explicit Tokens(storage_t tokens) : tokens_{std::move(tokens)} {}

    storage_t tokens_;
};

}  // namespace ecflow::light

#endif
