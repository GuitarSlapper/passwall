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

#include "SecurityService.h"

#include <array>
#include <jwt-cpp/jwt.h>
#include <sodium.h>
#include <stdexcept>
#include <utility>

namespace passwall::server
{
bool SecurityService::isUuid(const std::string& id)
{
    if (id.size() != 36)
        return false;
    for (std::size_t i = 0; i < id.size(); ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (id[i] != '-')
                return false;
        }
        else if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f') ||
                   (id[i] >= 'A' && id[i] <= 'F')))
            return false;
    }
    return true;
}

std::string SecurityService::uuid()
{
    std::array<unsigned char, 16> bytes{};
    randombytes_buf(bytes.data(), bytes.size());
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            result += '-';
        result += hex[bytes[i] >> 4];
        result += hex[bytes[i] & 15];
    }
    return result;
}

std::string SecurityService::hashPassword(const std::string& password)
{
    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str_alg(hash, password.data(), password.size(), 3,
                              crypto_pwhash_MEMLIMIT_INTERACTIVE,
                              crypto_pwhash_ALG_ARGON2ID13) != 0)
        throw std::runtime_error("Password hashing failed");
    return hash;
}

SecurityService::SecurityService(AuthConfig config) : config_(std::move(config))
{
    if (config_.jwtSecret.size() < 32 || config_.issuer.empty() || config_.audience.empty() ||
        config_.tokenLifetime.count() <= 0 || config_.tokenLifetime > std::chrono::hours(24))
        throw std::invalid_argument("Invalid JWT configuration");
    if (sodium_init() < 0)
        throw std::runtime_error("Sodium initialization failed");
    dummyHash_ = hashPassword(uuid());
}

SecurityService::~SecurityService()
{
    sodium_memzero(config_.jwtSecret.data(), config_.jwtSecret.size());
}

WipeOnExit::~WipeOnExit()
{
    sodium_memzero(value.data(), value.size());
}

bool SecurityService::verifyPassword(const std::string& hash, const std::string& password) const
{
    const auto& stored = hash.empty() ? dummyHash_ : hash;
    return crypto_pwhash_str_verify(stored.c_str(), password.data(), password.size()) == 0;
}

IssuedToken SecurityService::issueToken(const std::string& userId) const
{
    const auto sessionId = uuid();
    const auto now = std::chrono::system_clock::now();
    const auto expires = now + config_.tokenLifetime;
    const auto token = jwt::create()
                           .set_type("JWT")
                           .set_issuer(config_.issuer)
                           .set_audience(config_.audience)
                           .set_subject(userId)
                           .set_id(sessionId)
                           .set_issued_at(now)
                           .set_expires_at(expires)
                           .sign(jwt::algorithm::hs256{config_.jwtSecret});
    return {token, sessionId, expires};
}

Identity SecurityService::verifyToken(const std::string& token) const
{
    try
    {
        const auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{config_.jwtSecret})
            .with_issuer(config_.issuer)
            .with_audience(config_.audience)
            .verify(decoded);
        // jwt-cpp validates time claims when present; require them too.
        if (!decoded.has_expires_at() || !decoded.has_issued_at() || !decoded.has_subject() ||
            !decoded.has_id())
            return {};
        const Identity identity{decoded.get_subject(), decoded.get_id()};
        if (!isUuid(identity.userId) || !isUuid(identity.sessionId))
            return {};
        return identity;
    }
    catch (const std::exception&)
    {
        return {};
    }
}
} // namespace passwall::server
