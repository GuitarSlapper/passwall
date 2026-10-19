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

#include "SessionAuthenticator.h"
#include "services/repositories/PasswordWalletRepository.h"

#include <stdexcept>
#include <utility>

namespace passwall::server
{
SessionAuthenticator::SessionAuthenticator(std::shared_ptr<PasswordWalletRepository> repository,
                                           std::shared_ptr<SecurityService> security)
    : repository_(std::move(repository)), security_(std::move(security))
{
    if (!repository_ || !security_)
        throw std::invalid_argument("Authentication dependencies are required");
}

Identity SessionAuthenticator::authenticate(const std::string& token) const
{
    const auto identity = security_->verifyToken(token);
    if (identity.userId.empty())
        return {};
    // DB failures propagate, so the service returns 500 rather than a false 401.
    return repository_->hasSession(identity.sessionId, identity.userId) ? identity : Identity{};
}
} // namespace passwall::server
