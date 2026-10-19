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

#include "HealthController.h"

#include <stdexcept>
#include <utility>

namespace passwall::server
{
using namespace http;

HealthController::HealthController(std::shared_ptr<HealthService> service, std::string version)
    : service_(std::move(service)), version_(std::move(version))
{
    if (!service_)
        throw std::invalid_argument("HealthService is required");
}

void HealthController::getHealth(const drogon::HttpRequestPtr&, Callback&& callback) const
{
    Json::Value body(Json::objectValue);
    body["status"] = "ok";
    body["version"] = version_;
    respond(callback, drogon::k200OK, body);
}

void HealthController::getReady(const drogon::HttpRequestPtr&, Callback&& callback) const
{
    service_->checkReady(
        [callback = std::move(callback)](Result<Empty> result)
        {
            if (result.error != ServiceError::none)
            {
                error(callback, drogon::k503ServiceUnavailable, "not_ready");
                return;
            }
            Json::Value body(Json::objectValue);
            body["status"] = "ready";
            respond(callback, drogon::k200OK, body);
        });
}

} // namespace passwall::server
