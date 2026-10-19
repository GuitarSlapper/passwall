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

#include "PostgresPasswordWalletRepository.h"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace passwall::server
{
namespace
{
VaultEntry entryFromRow(const drogon::orm::Row& row)
{
    return {row["id"].as<std::string>(), row["encrypted_data"].as<std::string>(),
            row["nonce"].as<std::string>(), row["created_at"].as<std::string>(),
            row["updated_at"].as<std::string>()};
}
} // namespace

PostgresPasswordWalletRepository::PostgresPasswordWalletRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
    if (!db_)
        throw std::invalid_argument("Database client is required");
    db_->setTimeout(5.0);
}

void PostgresPasswordWalletRepository::checkReady()
{
    db_->execSqlSync("SELECT u.id,u.email,u.password_hash,u.kdf,u.encrypted_vault_key,"
                     "u.vault_key_nonce,s.id,s.user_id,s.expires_at,v.id,v.user_id,"
                     "v.encrypted_data,v.nonce,v.created_at,v.updated_at "
                     "FROM users u, auth_sessions s, vault_entries v LIMIT 0");
}

bool PostgresPasswordWalletRepository::createUser(const NewUser& user)
{
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return !db_->execSqlSync(
                   "INSERT INTO "
                   "users(id,email,password_hash,kdf,encrypted_vault_key,vault_key_nonce) "
                   "VALUES($1::uuid,$2,$3,$4::jsonb,$5,$6) ON CONFLICT(email) DO NOTHING RETURNING "
                   "id",
                   user.id, user.email, user.passwordHash, Json::writeString(writer, user.kdf),
                   user.encryptedVaultKey, user.vaultKeyNonce)
                .empty();
}

std::optional<UserCredentials>
PostgresPasswordWalletRepository::findCredentials(const std::string& email)
{
    const auto rows = db_->execSqlSync("SELECT id,password_hash FROM users WHERE email=$1", email);
    if (rows.empty())
        return std::nullopt;
    return UserCredentials{rows[0]["id"].as<std::string>(),
                           rows[0]["password_hash"].as<std::string>()};
}

void PostgresPasswordWalletRepository::createSession(
    const std::string& id, const std::string& userId,
    std::chrono::system_clock::time_point expiresAt)
{
    const auto epoch =
        std::chrono::duration_cast<std::chrono::seconds>(expiresAt.time_since_epoch()).count();
    db_->execSqlSync("INSERT INTO auth_sessions(id,user_id,expires_at) "
                     "VALUES($1::uuid,$2::uuid,to_timestamp($3::bigint))",
                     id, userId, static_cast<std::int64_t>(epoch));
}

bool PostgresPasswordWalletRepository::hasSession(const std::string& id, const std::string& userId)
{
    return !db_->execSqlSync("SELECT 1 FROM auth_sessions WHERE id=$1::uuid AND user_id=$2::uuid "
                             "AND expires_at > CURRENT_TIMESTAMP",
                             id, userId)
                .empty();
}

bool PostgresPasswordWalletRepository::deleteSession(const std::string& id,
                                                     const std::string& userId)
{
    return !db_->execSqlSync("DELETE FROM auth_sessions WHERE id=$1::uuid AND user_id=$2::uuid "
                             "AND expires_at > CURRENT_TIMESTAMP RETURNING id",
                             id, userId)
                .empty();
}

std::vector<VaultEntry> PostgresPasswordWalletRepository::getEntries(const std::string& userId)
{
    const auto rows = db_->execSqlSync(R"SQL(
        SELECT id,encrypted_data,nonce,
            to_char(created_at AT TIME ZONE 'UTC','YYYY-MM-DD"T"HH24:MI:SS.US"Z"') AS created_at,
            to_char(updated_at AT TIME ZONE 'UTC','YYYY-MM-DD"T"HH24:MI:SS.US"Z"') AS updated_at
        FROM vault_entries WHERE user_id=$1::uuid ORDER BY created_at,id
    )SQL",
                                       userId);
    std::vector<VaultEntry> entries;
    entries.reserve(rows.size());
    for (const auto& row : rows)
        entries.push_back(entryFromRow(row));
    return entries;
}

VaultEntry PostgresPasswordWalletRepository::createEntry(const std::string& id,
                                                         const std::string& userId,
                                                         const std::string& encryptedData,
                                                         const std::string& nonce)
{
    const auto rows = db_->execSqlSync(R"SQL(
        INSERT INTO vault_entries(id,user_id,encrypted_data,nonce) VALUES($1::uuid,$2::uuid,$3,$4)
        RETURNING id,encrypted_data,nonce,
            to_char(created_at AT TIME ZONE 'UTC','YYYY-MM-DD"T"HH24:MI:SS.US"Z"') AS created_at,
            to_char(updated_at AT TIME ZONE 'UTC','YYYY-MM-DD"T"HH24:MI:SS.US"Z"') AS updated_at
    )SQL",
                                       id, userId, encryptedData, nonce);
    return entryFromRow(rows[0]);
}

bool PostgresPasswordWalletRepository::deleteEntry(const std::string& id, const std::string& userId)
{
    return !db_->execSqlSync(
                   "DELETE FROM vault_entries WHERE id=$1::uuid AND user_id=$2::uuid RETURNING id",
                   id, userId)
                .empty();
}
} // namespace passwall::server
