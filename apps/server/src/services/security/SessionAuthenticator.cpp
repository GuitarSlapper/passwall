#include "SessionAuthenticator.h"
#include "services/repositories/PasswordWalletRepository.h"

#include <stdexcept>
#include <utility>

namespace passwall
{
SessionAuthenticator::SessionAuthenticator(std::shared_ptr<PasswordWalletRepository> repository,
                                           std::shared_ptr<SecurityService> security)
    : repository_(std::move(repository)), security_(std::move(security))
{
    if (!repository_ || !security_) throw std::invalid_argument("Authentication dependencies are required");
}

Identity SessionAuthenticator::authenticate(const std::string &token) const
{
    const auto identity = security_->verifyToken(token);
    if (identity.userId.empty()) return {};
    // DB failures propagate, so the service returns 500 rather than a false 401.
    return repository_->hasSession(identity.sessionId, identity.userId) ? identity : Identity{};
}
}  // namespace passwall
