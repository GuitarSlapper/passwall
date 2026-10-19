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

#include "services/ServiceTypes.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

namespace passwall::server::http
{
using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

void respond(const Callback&, drogon::HttpStatusCode, const Json::Value&);
void error(const Callback&, drogon::HttpStatusCode, const char* code);
void noContent(const Callback&);
bool failed(ServiceError, const Callback&, bool allowConflict = false, bool allowNotFound = false);
bool stringField(const Json::Value&, const char* key);
bool validEmail(const Json::Value&);
bool validPassword(const Json::Value&, const char* key);
bool validKdf(const Json::Value&);
std::string bearer(const drogon::HttpRequestPtr&);
Json::Value serialize(const VaultEntry&);
} // namespace passwall::server::http
