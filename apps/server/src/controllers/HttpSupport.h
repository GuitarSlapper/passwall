#pragma once

#include "services/ServiceTypes.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

namespace passwall::http
{
using Callback = std::function<void(const drogon::HttpResponsePtr &)>;

void respond(const Callback &, drogon::HttpStatusCode, const Json::Value &);
void error(const Callback &, drogon::HttpStatusCode, const char *code);
void noContent(const Callback &);
bool failed(ServiceError, const Callback &, bool allowConflict = false,
            bool allowNotFound = false);
bool stringField(const Json::Value &, const char *key);
bool validEmail(const Json::Value &);
bool validPassword(const Json::Value &, const char *key);
bool validKdf(const Json::Value &);
std::string bearer(const drogon::HttpRequestPtr &);
Json::Value serialize(const VaultEntry &);
}  // namespace passwall::http
