# Password Wallet server

Implements the eight operations in `api/openapi.yaml` with Drogon, PostgreSQL,
libsodium (Argon2id) and jwt-cpp (HS256). Requires C++17 or newer.

## Three-layer architecture

```text
apps/server/src/
  main.cpp                          # dependency wiring and HTTP server startup
  CMakeLists.txt                    # build targets for all three layers
  README.md
  controllers/                      # HTTP layer, grouped by OpenAPI tags
    HealthController.{h,cpp}
    AuthenticationController.{h,cpp}
    VaultController.{h,cpp}
    HttpSupport.{h,cpp}              # JSON validation, Bearer parsing, responses
  services/                         # application/business layer
    HealthService.{h,cpp}
    AuthenticationService.{h,cpp}
    VaultService.{h,cpp}
    ServiceTypes.h                  # shared request, result and entity types
    AsyncExecutor.{h,cpp}            # bounded background task execution
    security/
      SecurityService.{h,cpp}        # Argon2id, UUIDs and JWT cryptography
      SessionAuthenticator.{h,cpp}   # JWT validation plus session checks
    repositories/                   # data access layer, under services as requested
      PasswordWalletRepository.h    # abstract persistence contract and records
      PostgresPasswordWalletRepository.{h,cpp}
      schema.sql
  tests/
    integration.py
```

The request flow is **controller → service → repository**. Controllers own HTTP
routing, input validation and response serialization. Services implement use
cases, authorization and error outcomes; they use the repository interface and
contain no SQL or Drogon HTTP/ORM code. The PostgreSQL adapter owns SQL and row
mapping, and receives password hashes rather than plaintext passwords.

`main.cpp` constructs the repository, shared security components, executor and
three services, then registers three controllers before starting Drogon on
`127.0.0.1:8080`. Dependencies are injected through constructors. Background tasks
retain their dependencies until completion; shutdown drains the shared executor.

The CMake targets are `passwall_contracts`, `passwall_repositories`,
`passwall_services`, `passwall_controllers` and the `passwall_server` executable.
The service target does not link the concrete PostgreSQL adapter. The executable
selects that adapter, so another repository implementation can be substituted.

| OpenAPI tag | Controller | Service | Routes |
| --- | --- | --- | --- |
| Health | HealthController | HealthService | `GET /health`, `GET /ready` |
| Authentication | AuthenticationController | AuthenticationService | `POST /register`, `POST /login`, `POST /logout` |
| Vault | VaultController | VaultService | `GET /vault/entries`, `POST /vault/entries`, `DELETE /vault/entries/{id}` |

## Build and run

Install Drogon **with PostgreSQL support**, jwt-cpp, OpenSSL, JsonCpp and libsodium.
Make their CMake packages available using your package manager/toolchain or
`CMAKE_PREFIX_PATH`. This directory is a standalone CMake project:

```powershell
cmake -S apps/server/src -B build/server-layered
cmake --build build/server-layered --config Release
psql -v ON_ERROR_STOP=1 -d passwall -f apps/server/src/services/repositories/schema.sql
$env:PASSWALL_DATABASE_URL = "host=127.0.0.1 port=5432 dbname=passwall user=passwall connect_timeout=5"
# Supply PASSWALL_JWT_SECRET from a secret store: at least 32 random bytes.
./build/server-layered/Release/passwall_server.exe
```

For single-configuration generators, the executable is directly in `build/server-layered`.
Use a fresh build directory: an existing cache configured from `src/controllers`
still points to the previous source directory. The refactor does not change the
database schema; do not reapply the initial SQL to an already initialized database.
Use PostgreSQL's password file or deployment secrets for database credentials.
The schema is deliberately not applied automatically at startup.

For an existing application, link `passwall_controllers` and your repository
implementation, then register each controller with
`drogon::app().registerController(...)` before `run()`, as shown in `main.cpp`.
Do not also include this `main.cpp` in an existing executable.

## Behavior and boundaries

- Passwords use Argon2id, three passes and 64 MiB, with a random salt. Unknown
  users still perform password verification against a dummy hash.
- JWTs expire after 15 minutes and require HS256, issuer, audience, subject,
  issue time, expiration and session ID. Sessions persist in PostgreSQL;
  logout deletes the current session. Subsequent requests using it return 401.
  Requests already authorized before concurrent logout may finish.
- Vault queries always use the authenticated user's ID. Foreign/missing entry
  IDs and malformed UUIDs return 404 after authentication.
- Database calls and password hashing run on two workers with a queue limited
  to 128 jobs. Database operations have a five-second timeout.
- `/health` checks process liveness; `/ready` checks connectivity and the schema.
- Backend faults return sanitized JSON 500 responses. These fallback responses
  are necessary but are not listed on every operation in the current OpenAPI.
- The HTTP body limit is 1 MiB. Apply HTTPS and login rate limits at your reverse
  proxy before exposing the server remotely. Email uniqueness is case-sensitive.
- `encrypted_data` and `nonce` are opaque strings. The client chooses encryption.
  Client KDF parameters are stored, never used for server password hashing.
- Expired sessions are rejected immediately. Periodically clean them with:
  `DELETE FROM auth_sessions WHERE expires_at <= CURRENT_TIMESTAMP;`.

`doc/api.md` also describes `/vault/key-info`, but `api/openapi.yaml` does not.
This server follows OpenAPI and does not add that route. A client on a new device
will need that contract extended to retrieve the encrypted vault key.

## Integration tests

Use a disposable database with `services/repositories/schema.sql` applied and start this server:

```powershell
python apps/server/src/tests/integration.py
```

Tests create uniquely named accounts and exercise all routes, invalid requests,
JWT tampering, owner isolation, logout and concurrent duplicate registration.
If the test process has the same `PASSWALL_JWT_SECRET` as the server, it also
checks correctly signed tokens with missing/invalid claims and disallowed
algorithms. Otherwise that test is skipped.
They leave test users in the database; do not run against production.

Implementation references: [Drogon controllers](https://github.com/drogonframework/drogon/wiki/ENG-04-2-Controller-HttpController),
[Drogon database client](https://github.com/drogonframework/drogon/wiki/ENG-08-1-Database-DbClient),
[libsodium password hashing](https://doc.libsodium.org/password_hashing/default_phf),
[jwt-cpp](https://github.com/Thalhammer/jwt-cpp).
