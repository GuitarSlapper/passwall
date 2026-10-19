#pragma once

#include <chrono>
#include <string>

namespace passwall::server
{
struct AuthConfig
{
    std::string jwtSecret; // At least 32 random bytes from deployment secrets.
    std::string issuer{"passwall"};
    std::string audience{"passwall-client"};
    std::chrono::seconds tokenLifetime{900};
};

struct Identity
{
    std::string userId;
    std::string sessionId;
};

struct IssuedToken
{
    std::string token;
    std::string sessionId;
    std::chrono::system_clock::time_point expiresAt;
};

// Cryptography only: no HTTP or database access.
class SecurityService final
{
public:
    explicit SecurityService(AuthConfig config);
    ~SecurityService();
    SecurityService(const SecurityService&) = delete;
    SecurityService& operator=(const SecurityService&) = delete;

    static std::string uuid();
    static bool isUuid(const std::string&);
    static std::string hashPassword(const std::string& password);
    // An empty hash still performs dummy password verification.
    bool verifyPassword(const std::string& hash, const std::string& password) const;
    IssuedToken issueToken(const std::string& userId) const;
    Identity verifyToken(const std::string& token) const;

private:
    AuthConfig config_;
    std::string dummyHash_;
};

struct WipeOnExit
{
    std::string& value;
    ~WipeOnExit();
};
} // namespace passwall::server
