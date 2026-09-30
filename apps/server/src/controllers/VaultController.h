#pragma once

#include "HttpSupport.h"
#include "services/VaultService.h"
#include <drogon/HttpController.h>
#include <memory>

namespace passwall
{
class VaultController final : public drogon::HttpController<VaultController, false>
{
  public:
    using Callback = http::Callback;
    explicit VaultController(std::shared_ptr<VaultService> service);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(VaultController::getVaultEntries, "/vault/entries", drogon::Get);
    ADD_METHOD_TO(VaultController::createVaultEntry, "/vault/entries", drogon::Post);
    ADD_METHOD_TO(VaultController::deleteVaultEntry, "/vault/entries/{1}", drogon::Delete);
    METHOD_LIST_END

    void getVaultEntries(const drogon::HttpRequestPtr &, Callback &&) const;
    void createVaultEntry(const drogon::HttpRequestPtr &, Callback &&) const;
    void deleteVaultEntry(const drogon::HttpRequestPtr &, Callback &&, std::string id) const;

  private:
    std::shared_ptr<VaultService> service_;
};
}  // namespace passwall
