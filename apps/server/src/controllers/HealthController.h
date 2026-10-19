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
#include "services/HealthService.h"
#include <drogon/HttpController.h>
#include <memory>

namespace passwall::server
{
class HealthController final : public drogon::HttpController<HealthController, false>
{
public:
    using Callback = http::Callback;
    explicit HealthController(std::shared_ptr<HealthService> service,
                              std::string version = "0.0.1");

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::getHealth, "/health", drogon::Get);
    ADD_METHOD_TO(HealthController::getReady, "/ready", drogon::Get);
    METHOD_LIST_END

    void getHealth(const drogon::HttpRequestPtr&, Callback&&) const;
    void getReady(const drogon::HttpRequestPtr&, Callback&&) const;

private:
    std::shared_ptr<HealthService> service_;
    std::string version_;
};
} // namespace passwall::server
