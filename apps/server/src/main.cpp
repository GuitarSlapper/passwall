#include "controllers/HealthController.h"
#include "controllers/AuthenticationController.h"
#include "controllers/VaultController.h"
#include "services/AsyncExecutor.h"
#include "services/security/SessionAuthenticator.h"
#include "services/repositories/PostgresPasswordWalletRepository.h"

#include <drogon/drogon.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace
{
std::string requiredEnv(const char *name)
{
    const auto value = std::getenv(name);
    if (!value || !*value) throw std::runtime_error(std::string("Set ") + name);
    return value;
}
}

int main()
{
    try
    {
        auto db = drogon::orm::DbClient::newPgClient(requiredEnv("PASSWALL_DATABASE_URL"), 4);
        passwall::AuthConfig auth;
        auth.jwtSecret = requiredEnv("PASSWALL_JWT_SECRET");
        // Composition root: only startup knows the concrete persistence adapter.
        auto repository = std::make_shared<passwall::PostgresPasswordWalletRepository>(db);
        auto security = std::make_shared<passwall::SecurityService>(std::move(auth));
        auto authenticator = std::make_shared<passwall::SessionAuthenticator>(repository, security);
        auto executor = std::make_shared<passwall::AsyncExecutor>();
        auto health = std::make_shared<passwall::HealthService>(repository, executor);
        auto authentication = std::make_shared<passwall::AuthenticationService>(
            repository, executor, security, authenticator);
        auto vault = std::make_shared<passwall::VaultService>(repository, executor, authenticator);
        drogon::app().registerController(std::make_shared<passwall::HealthController>(health));
        drogon::app().registerController(std::make_shared<passwall::AuthenticationController>(authentication));
        drogon::app().registerController(std::make_shared<passwall::VaultController>(vault));
        // Local default; terminate TLS at a reverse proxy for remote clients.
        drogon::app().addListener("127.0.0.1", 8080)
            .setThreadNum(2)
            .setClientMaxBodySize(1024 * 1024)
            .setLogLevel(trantor::Logger::kWarn)
            .run();
        executor->shutdown();
    }
    catch (const std::exception &)
    {
        // Backend exceptions may include connection strings; do not print them.
        std::cerr << "Server startup failed. Check dependencies, PASSWALL_DATABASE_URL "
                     "and PASSWALL_JWT_SECRET (at least 32 random bytes).\n";
        return EXIT_FAILURE;
    }
}
