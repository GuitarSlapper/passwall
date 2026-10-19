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

#include "controllers/AuthenticationController.h"
#include "controllers/HealthController.h"
#include "controllers/VaultController.h"
#include "services/AsyncExecutor.h"
#include "services/repositories/PostgresPasswordWalletRepository.h"
#include "services/security/SessionAuthenticator.h"

#include <drogon/drogon.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace
{
std::string requiredEnv(const char* name)
{
    const auto value = std::getenv(name);
    if (!value || !*value)
        throw std::runtime_error(std::string("Set ") + name);
    return value;
}
} // namespace

int main()
{
    try
    {
        auto db = drogon::orm::DbClient::newPgClient(requiredEnv("PASSWALL_DATABASE_URL"), 4);
        passwall::server::AuthConfig auth;
        auth.jwtSecret = requiredEnv("PASSWALL_JWT_SECRET");
        // Composition root: only startup knows the concrete persistence adapter.
        auto repository = std::make_shared<passwall::server::PostgresPasswordWalletRepository>(db);
        auto security = std::make_shared<passwall::server::SecurityService>(std::move(auth));
        auto authenticator =
            std::make_shared<passwall::server::SessionAuthenticator>(repository, security);
        auto executor = std::make_shared<passwall::server::AsyncExecutor>();
        auto health = std::make_shared<passwall::server::HealthService>(repository, executor);
        auto authentication = std::make_shared<passwall::server::AuthenticationService>(
            repository, executor, security, authenticator);
        auto vault =
            std::make_shared<passwall::server::VaultService>(repository, executor, authenticator);
        drogon::app().registerController(
            std::make_shared<passwall::server::HealthController>(health));
        drogon::app().registerController(
            std::make_shared<passwall::server::AuthenticationController>(authentication));
        drogon::app().registerController(
            std::make_shared<passwall::server::VaultController>(vault));
        // Local default; terminate TLS at a reverse proxy for remote clients.
        drogon::app()
            .addListener("127.0.0.1", 8080)
            .setThreadNum(2)
            .setClientMaxBodySize(1024 * 1024)
            .setLogLevel(trantor::Logger::kWarn)
            .run();
        executor->shutdown();
    }
    catch (const std::exception&)
    {
        // Backend exceptions may include connection strings; do not print them.
        std::cerr << "Server startup failed. Check dependencies, PASSWALL_DATABASE_URL "
                     "and PASSWALL_JWT_SECRET (at least 32 random bytes).\n";
        return EXIT_FAILURE;
    }
}
