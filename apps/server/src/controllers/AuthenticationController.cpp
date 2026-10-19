#include "AuthenticationController.h"

#include <stdexcept>
#include <utility>

namespace passwall::server
{
using namespace http;

AuthenticationController::AuthenticationController(std::shared_ptr<AuthenticationService> service)
    : service_(std::move(service))
{
    if (!service_)
        throw std::invalid_argument("AuthenticationService is required");
}

void AuthenticationController::registerUser(const drogon::HttpRequestPtr& request,
                                            Callback&& callback) const
{
    const auto body = request->getJsonObject();
    if (!body || !validEmail(*body) || !validPassword(*body, "auth_password") || !validKdf(*body) ||
        !stringField(*body, "encrypted_vault_key") || !stringField(*body, "vault_key_nonce"))
    {
        error(callback, drogon::k400BadRequest, "invalid_request");
        return;
    }
    RegisterRequest input{(*body)["email"].asString(), (*body)["auth_password"].asString(),
                          (*body)["kdf"], (*body)["encrypted_vault_key"].asString(),
                          (*body)["vault_key_nonce"].asString()};
    service_->registerUser(std::move(input),
                           [callback = std::move(callback)](Result<RegisteredUser> result)
                           {
                               if (failed(result.error, callback, true))
                                   return;
                               Json::Value response(Json::objectValue);
                               response["id"] = result.value.id;
                               response["email"] = result.value.email;
                               respond(callback, drogon::k201Created, response);
                           });
}

void AuthenticationController::login(const drogon::HttpRequestPtr& request,
                                     Callback&& callback) const
{
    const auto body = request->getJsonObject();
    if (!body || !validEmail(*body) || !validPassword(*body, "password"))
    {
        error(callback, drogon::k400BadRequest, "invalid_request");
        return;
    }
    service_->login((*body)["email"].asString(), (*body)["password"].asString(),
                    [callback = std::move(callback)](Result<std::string> result)
                    {
                        if (failed(result.error, callback))
                            return;
                        Json::Value response(Json::objectValue);
                        response["access_token"] = result.value;
                        response["token_type"] = "Bearer";
                        respond(callback, drogon::k200OK, response);
                    });
}

void AuthenticationController::logout(const drogon::HttpRequestPtr& request,
                                      Callback&& callback) const
{
    auto token = bearer(request);
    if (token.empty())
    {
        error(callback, drogon::k401Unauthorized, "invalid_credentials");
        return;
    }
    service_->logout(std::move(token),
                     [callback = std::move(callback)](Result<Empty> result)
                     {
                         if (!failed(result.error, callback))
                             noContent(callback);
                     });
}

} // namespace passwall::server
