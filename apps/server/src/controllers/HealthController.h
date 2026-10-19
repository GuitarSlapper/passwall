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
