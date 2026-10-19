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
