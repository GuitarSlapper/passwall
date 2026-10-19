#include "VaultController.h"

#include <stdexcept>
#include <utility>

namespace passwall::server
{
using namespace http;

VaultController::VaultController(std::shared_ptr<VaultService> service)
    : service_(std::move(service))
{
    if (!service_)
        throw std::invalid_argument("VaultService is required");
}

void VaultController::getVaultEntries(const drogon::HttpRequestPtr& request,
                                      Callback&& callback) const
{
    auto token = bearer(request);
    if (token.empty())
    {
        error(callback, drogon::k401Unauthorized, "invalid_credentials");
        return;
    }
    service_->getVaultEntries(
        std::move(token),
        [callback = std::move(callback)](Result<std::vector<VaultEntry>> result)
        {
            if (failed(result.error, callback))
                return;
            Json::Value response(Json::arrayValue);
            for (const auto& entry : result.value)
                response.append(serialize(entry));
            respond(callback, drogon::k200OK, response);
        });
}

void VaultController::createVaultEntry(const drogon::HttpRequestPtr& request,
                                       Callback&& callback) const
{
    auto token = bearer(request);
    if (token.empty())
    {
        error(callback, drogon::k401Unauthorized, "invalid_credentials");
        return;
    }
    const auto body = request->getJsonObject();
    if (!body || !stringField(*body, "encrypted_data") || !stringField(*body, "nonce"))
    {
        error(callback, drogon::k400BadRequest, "invalid_request");
        return;
    }
    service_->createVaultEntry(
        std::move(token), (*body)["encrypted_data"].asString(), (*body)["nonce"].asString(),
        [callback = std::move(callback)](Result<VaultEntry> result)
        {
            if (!failed(result.error, callback))
                respond(callback, drogon::k201Created, serialize(result.value));
        });
}

void VaultController::deleteVaultEntry(const drogon::HttpRequestPtr& request, Callback&& callback,
                                       std::string id) const
{
    auto token = bearer(request);
    if (token.empty())
    {
        error(callback, drogon::k401Unauthorized, "invalid_credentials");
        return;
    }
    service_->deleteVaultEntry(std::move(token), std::move(id),
                               [callback = std::move(callback)](Result<Empty> result)
                               {
                                   if (!failed(result.error, callback, false, true))
                                       noContent(callback);
                               });
}
} // namespace passwall::server
