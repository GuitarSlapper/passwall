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
