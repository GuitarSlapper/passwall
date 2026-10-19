// MIT License
//
// Copyright (c) 2026 GuitarSlapper
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "HttpSupport.h"
#include "services/AuthenticationService.h"
#include <drogon/HttpController.h>
#include <memory>

namespace passwall::server
{
class AuthenticationController final
    : public drogon::HttpController<AuthenticationController, false>
{
public:
    using Callback = http::Callback;
    explicit AuthenticationController(std::shared_ptr<AuthenticationService> service);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthenticationController::registerUser, "/register", drogon::Post);
    ADD_METHOD_TO(AuthenticationController::login, "/login", drogon::Post);
    ADD_METHOD_TO(AuthenticationController::logout, "/logout", drogon::Post);
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr&, Callback&&) const;
    void login(const drogon::HttpRequestPtr&, Callback&&) const;
    void logout(const drogon::HttpRequestPtr&, Callback&&) const;

private:
    std::shared_ptr<AuthenticationService> service_;
};
} // namespace passwall::server
