#pragma once

#include <json/json.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace passwall
{
struct RegisterRequest
{
    std::string email;
    std::string authPassword;
    Json::Value kdf;
    std::string encryptedVaultKey;
    std::string vaultKeyNonce;
};

struct RegisteredUser
{
    std::string id;  // UUID
    std::string email;
};

struct VaultEntry
{
    std::string id;  // UUID
    std::string encryptedData;
    std::string nonce;
    std::string createdAt;  // RFC 3339, UTC
    std::string updatedAt;  // RFC 3339, UTC
};

enum class ServiceError
{
    none,
    emailAlreadyExists,
    invalidCredentials,
    notFound,
    unavailable,
    internalError
};

template <typename T>
struct Result
{
    T value{};
    ServiceError error{ServiceError::none};

    static Result success(T value = {})
    {
        return {std::move(value), ServiceError::none};
    }

    static Result failure(ServiceError error)
    {
        return {{}, error};
    }
};

struct Empty
{
};

template <typename T>
using Completion = std::function<void(Result<T>)>;

}  // namespace passwall
