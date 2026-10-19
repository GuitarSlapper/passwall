#include "HttpSupport.h"

#include <algorithm>
#include <cctype>

namespace passwall::server::http
{

void respond(const Callback& callback, drogon::HttpStatusCode status, const Json::Value& body)
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(status);
    response->addHeader("Cache-Control", "no-store");
    if (status == drogon::k401Unauthorized)
        response->addHeader("WWW-Authenticate", "Bearer");
    callback(response);
}

void error(const Callback& callback, drogon::HttpStatusCode status, const char* code)
{
    Json::Value body(Json::objectValue);
    body["error"] = code;
    respond(callback, status, body);
}

void noContent(const Callback& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k204NoContent);
    response->addHeader("Cache-Control", "no-store");
    callback(response);
}

bool failed(ServiceError result, const Callback& callback, bool allowConflict, bool allowNotFound)
{
    switch (result)
    {
    case ServiceError::none:
        return false;
    case ServiceError::invalidCredentials:
        error(callback, drogon::k401Unauthorized, "invalid_credentials");
        return true;
    case ServiceError::emailAlreadyExists:
        if (allowConflict)
        {
            error(callback, drogon::k409Conflict, "email_already_exists");
            return true;
        }
        break;
    case ServiceError::notFound:
        if (allowNotFound)
        {
            error(callback, drogon::k404NotFound, "not_found");
            return true;
        }
        break;
    default:
        break;
    }
    error(callback, drogon::k500InternalServerError, "internal_error");
    return true;
}

bool stringField(const Json::Value& body, const char* key)
{
    return body.isObject() && body.isMember(key) && body[key].isString();
}

bool validEmail(const Json::Value& body)
{
    if (!stringField(body, "email"))
        return false;
    const auto email = body["email"].asString();
    const auto at = email.find('@');
    return at != std::string::npos && at != 0 && at + 1 < email.size() &&
           email.find('@', at + 1) == std::string::npos &&
           std::none_of(email.begin(), email.end(),
                        [](unsigned char c) { return c <= 32 || c == 127; });
}

bool validPassword(const Json::Value& body, const char* key)
{
    if (!stringField(body, key))
        return false;
    const auto password = body[key].asString();
    // JSON Schema minLength counts Unicode code points, not UTF-8 bytes.
    std::size_t count = 0;
    for (std::size_t i = 0; i < password.size(); ++count)
    {
        const auto c = static_cast<unsigned char>(password[i++]);
        unsigned trailing = 0;
        unsigned code = c;
        unsigned minimum = 0;
        if (c < 0x80)
        {
        }
        else if (c >= 0xc2 && c <= 0xdf)
        {
            trailing = 1;
            code = c & 31;
            minimum = 0x80;
        }
        else if (c >= 0xe0 && c <= 0xef)
        {
            trailing = 2;
            code = c & 15;
            minimum = 0x800;
        }
        else if (c >= 0xf0 && c <= 0xf4)
        {
            trailing = 3;
            code = c & 7;
            minimum = 0x10000;
        }
        else
            return false;
        for (unsigned j = 0; j < trailing; ++j)
        {
            if (i == password.size())
                return false;
            const auto next = static_cast<unsigned char>(password[i++]);
            if ((next & 0xc0) != 0x80)
                return false;
            code = (code << 6) | (next & 63);
        }
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
            return false;
    }
    return count >= 8;
}

bool validKdf(const Json::Value& body)
{
    if (!body.isMember("kdf") || !body["kdf"].isObject())
        return false;
    const auto& kdf = body["kdf"];
    if (!stringField(kdf, "algorithm") || kdf["algorithm"] != "argon2id" ||
        !stringField(kdf, "salt"))
        return false;
    for (const auto* key : {"memory", "iterations", "parallelism"})
        if (!kdf[key].isUInt64() || kdf[key].asUInt64() < 1)
            return false;
    return true;
}

std::string bearer(const drogon::HttpRequestPtr& request)
{
    const auto& header = request->getHeader("authorization");
    const auto space = header.find(' ');
    if (space != 6)
        return {};
    auto scheme = header.substr(0, space);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (scheme != "bearer")
        return {};
    const auto start = header.find_first_not_of(' ', space);
    if (start == std::string::npos)
        return {};
    auto token = header.substr(start);
    if (std::any_of(token.begin(), token.end(),
                    [](unsigned char c) { return c <= 32 || c == 127; }))
        return {};
    return token;
}

Json::Value serialize(const VaultEntry& entry)
{
    Json::Value body(Json::objectValue);
    body["id"] = entry.id;
    body["encrypted_data"] = entry.encryptedData;
    body["nonce"] = entry.nonce;
    body["created_at"] = entry.createdAt;
    body["updated_at"] = entry.updatedAt;
    return body;
}
} // namespace passwall::server::http
