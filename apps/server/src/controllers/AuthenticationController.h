#pragma once

#include "HttpSupport.h"
#include "services/AuthenticationService.h"
#include <drogon/HttpController.h>
#include <memory>

namespace passwall
{
class AuthenticationController final : public drogon::HttpController<AuthenticationController, false>
{
  public:
    using Callback = http::Callback;
    explicit AuthenticationController(std::shared_ptr<AuthenticationService> service);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthenticationController::registerUser, "/register", drogon::Post);
    ADD_METHOD_TO(AuthenticationController::login, "/login", drogon::Post);
    ADD_METHOD_TO(AuthenticationController::logout, "/logout", drogon::Post);
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr &, Callback &&) const;
    void login(const drogon::HttpRequestPtr &, Callback &&) const;
    void logout(const drogon::HttpRequestPtr &, Callback &&) const;

  private:
    std::shared_ptr<AuthenticationService> service_;
};
}  // namespace passwall
