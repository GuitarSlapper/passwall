# Password Wallet API

## Overview

Password Wallet API provides server-side functionality for:

* user registration and authentication;
* storing encrypted vault entries;
* retrieving encrypted vault entries;
* deleting vault entries;
* server health and readiness checks.

The server does **not** store plaintext vault data.

Encryption and decryption of vault entries are performed on the client.

---

## Architecture

```text
┌──────────────────┐
│      Client      │
│                  │
│ Qt/QML / Web UI  │
└────────┬─────────┘
         │ HTTPS / JSON
         ▼
┌──────────────────┐
│       API        │
│                  │
│ Authentication   │
│ Vault            │
│ Health           │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│    PostgreSQL    │
│                  │
│ Users            │
│ Encrypted Vault  │
└──────────────────┘
```

The server is responsible for authentication, authorization, persistence, and synchronization.

The client is responsible for encryption and decryption of sensitive vault data.

---

# Authentication and Encryption

Two different passwords are used.

## Authentication Password

The authentication password is used to authenticate the user against the server.

```text
auth_password
      │
      ▼
    Server
      │
      ▼
password verification
      │
      ▼
 access token
```

The server stores only a password hash.

The authentication password does **not** encrypt vault entries.

---

## Master Password

The master password is used locally to derive a Key Encryption Key (KEK).

```text
master_password
       │
       │ + salt
       ▼
    Argon2id
       │
       ▼
      KEK
```

The master password and KEK are never sent to the server.

---

## Vault Key

During registration, the client generates a random Vault Key.

```text
CSPRNG
  │
  ▼
Vault Key
```

The Vault Key is used to encrypt and decrypt vault entries.

The Vault Key itself is encrypted using the KEK:

```text
Master Password
       │
       ▼
    Argon2id
       │
       ▼
      KEK
       │
       ▼
encrypt(Vault Key)
       │
       ▼
Encrypted Vault Key
```

Only the encrypted Vault Key is stored on the server.

Therefore, the server never receives:

```text
master_password
KEK
plaintext Vault Key
plaintext vault entries
```

---

# API

## Health

### `GET /health`

Checks whether the server process is running.

#### Response

```http
200 OK
```

```json
{
  "status": "ok",
  "version": "0.0.1"
}
```

Possible errors:

```text
500 Internal Server Error
```

---

## Readiness

### `GET /ready`

Checks whether the server is ready to process requests.

This endpoint can include checks for required dependencies such as PostgreSQL.

#### Ready

```http
200 OK
```

```json
{
  "status": "ready"
}
```

#### Not ready

```http
503 Service Unavailable
```

---

# Registration

## `POST /register`

Creates a new user account.

Before sending the request, the client:

1. Generates a random KDF salt.
2. Derives a KEK from the master password using Argon2id.
3. Generates a random Vault Key.
4. Encrypts the Vault Key using the KEK.
5. Sends only the encrypted Vault Key and KDF parameters to the server.

```text
CLIENT                                      SERVER

master_password
      │
      │ + kdf_salt
      ▼
   Argon2id
      │
      ▼
     KEK

CSPRNG
  │
  ▼
Vault Key
  │
  ▼
encrypt using KEK
  │
  ▼
encrypted_vault_key
  │
  │
  └────── POST /register ──────────────────►
```

### Request

```json
{
  "email": "user@example.com",
  "auth_password": "authentication-password",

  "kdf": {
    "algorithm": "argon2id",
    "salt": "...",
    "memory": 65536,
    "iterations": 3,
    "parallelism": 1
  },

  "encrypted_vault_key": "...",
  "vault_key_nonce": "..."
}
```

The server hashes `auth_password` before storing it.

The following values must never be sent to the server:

```text
master_password
KEK
Vault Key
```

### Success

```http
201 Created
```

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "email": "user@example.com"
}
```

### Errors

```text
400 Bad Request
409 Conflict
```

`409 Conflict` can be returned when an account with the supplied email already exists.

---

# Login

## `POST /login`

Authenticates a user.

### Request

```json
{
  "email": "user@example.com",
  "password": "authentication-password"
}
```

### Success

```http
200 OK
```

```json
{
  "access_token": "eyJhbGciOi...",
  "token_type": "Bearer"
}
```

The access token is used for protected endpoints:

```http
Authorization: Bearer <access_token>
```

### Errors

```text
400 Bad Request
401 Unauthorized
```

---

# Logout

## `POST /logout`

Invalidates the current authenticated session or token.

### Request

```http
POST /logout
Authorization: Bearer <access_token>
```

### Success

```http
204 No Content
```

### Errors

```text
401 Unauthorized
```

---

# Vault Key Information

## `GET /vault/key-info`

Returns the information required by the client to recover the Vault Key.

Authentication is required.

### Request

```http
GET /vault/key-info
Authorization: Bearer <access_token>
```

### Response

```json
{
  "kdf": {
    "algorithm": "argon2id",
    "salt": "...",
    "memory": 65536,
    "iterations": 3,
    "parallelism": 1
  },

  "encrypted_vault_key": "...",
  "vault_key_nonce": "..."
}
```

The client then performs:

```text
master_password
       │
       │ + KDF parameters
       ▼
    Argon2id
       │
       ▼
      KEK
       │
       ▼
encrypted_vault_key
       │
       ▼
 decrypt locally
       │
       ▼
    Vault Key
```

The master password and decrypted Vault Key never leave the client.

---

# Vault Entries

Vault entries contain encrypted data.

The server treats `encrypted_data` as opaque data and does not attempt to decrypt it.

---

## `GET /vault/entries`

Returns all vault entries belonging to the authenticated user.

### Request

```http
GET /vault/entries
Authorization: Bearer <access_token>
```

### Response

```http
200 OK
```

```json
[
  {
    "id": "a6d57a2e-...",
    "encrypted_data": "...",
    "nonce": "...",
    "created_at": "2026-09-24T19:00:00Z",
    "updated_at": "2026-09-24T19:00:00Z"
  }
]
```

The client decrypts `encrypted_data` locally using the Vault Key.

### Errors

```text
401 Unauthorized
```

---

## `POST /vault/entries`

Creates a new encrypted vault entry.

The client first creates the plaintext entry:

```json
{
  "site": "example.com",
  "username": "user@example.com",
  "password": "secret",
  "notes": "Personal account"
}
```

This object is serialized and encrypted locally:

```text
Vault Entry
     │
     ▼
 serialization
     │
     ▼
   plaintext
     │
     │ + Vault Key
     ▼
AEAD encryption
     │
     ▼
ciphertext + nonce
```

Only encrypted data is sent to the server.

### Request

```http
POST /vault/entries
Authorization: Bearer <access_token>
Content-Type: application/json
```

```json
{
  "encrypted_data": "...",
  "nonce": "..."
}
```

### Success

```http
201 Created
```

```json
{
  "id": "a6d57a2e-...",
  "encrypted_data": "...",
  "nonce": "...",
  "created_at": "2026-09-24T19:00:00Z",
  "updated_at": "2026-09-24T19:00:00Z"
}
```

### Errors

```text
400 Bad Request
401 Unauthorized
```

---

## `DELETE /vault/entries/{id}`

Deletes a vault entry.

### Request

```http
DELETE /vault/entries/a6d57a2e-...
Authorization: Bearer <access_token>
```

The entry identifier is a UUID.

The server must verify that the requested entry belongs to the authenticated user.

Conceptually:

```sql
DELETE FROM vault_entries
WHERE id = :entry_id
  AND user_id = :authenticated_user_id;
```

### Success

```http
204 No Content
```

### Errors

```text
401 Unauthorized
404 Not Found
```

---

# Authorization

Protected endpoints use Bearer authentication.

```http
Authorization: Bearer <access_token>
```

Currently protected endpoints are:

```text
POST   /logout

GET    /vault/key-info

GET    /vault/entries
POST   /vault/entries
DELETE /vault/entries/{id}
```

Public endpoints are:

```text
GET  /health
GET  /ready

POST /register
POST /login
```

---

# Security Boundary

The main security boundary is between the client and server.

```text
                  CLIENT                         SERVER

auth_password ───────────────────────────────► password hash

master_password ──X
KEK             ──X
Vault Key       ──X
plaintext data  ──X

encrypted Vault Key ─────────────────────────► PostgreSQL
encrypted entries   ─────────────────────────► PostgreSQL
```

The server can authenticate users and store their vaults, but it should not possess enough information to decrypt vault contents.

---

# Endpoint Summary

| Method   | Endpoint              | Authentication | Description                                |
| -------- | --------------------- | -------------- | ------------------------------------------ |
| `GET`    | `/health`             | No             | Server health                              |
| `GET`    | `/ready`              | No             | Server readiness                           |
| `POST`   | `/register`           | No             | Register user                              |
| `POST`   | `/login`              | No             | Authenticate user                          |
| `POST`   | `/logout`             | Bearer         | Logout                                     |
| `GET`    | `/vault/key-info`     | Bearer         | Get encrypted Vault Key and KDF parameters |
| `GET`    | `/vault/entries`      | Bearer         | Get encrypted vault entries                |
| `POST`   | `/vault/entries`      | Bearer         | Create encrypted vault entry               |
| `DELETE` | `/vault/entries/{id}` | Bearer         | Delete vault entry                         |
