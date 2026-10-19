# Passwall Server

## Overview

Drogon HTTP server implementing the eight operations in the
[OpenAPI contract](../../api/openapi.yaml). PostgreSQL stores users, sessions and
client-encrypted vault data. See [Architecture](ARCHITECTURE.md) for layer boundaries.

## Requirements

- CMake 3.20+ and a C++23 compiler.
- Drogon built with PostgreSQL support, jwt-cpp, JsonCpp, OpenSSL and libsodium.
- PostgreSQL and `psql` for initial schema setup.
- Python 3 for integration tests.

## Build

Run commands from the repository root. Make dependencies available through your
package-manager toolchain or `CMAKE_PREFIX_PATH`.

```powershell
cmake -S apps/server -B build/server -DCMAKE_BUILD_TYPE=Release
cmake --build build/server --config Release
```

The executable is `passwall_server` (`.exe` on Windows). Multi-configuration
generators place it under `build/server/Release`. Add dependency DLL directories
to `PATH` or deploy the required DLLs beside the executable.

Use a fresh build directory if an old cache points to `apps/server/src` or
`apps/server/src/controllers`. The project CMake entry point is `apps/server`.

## Configuration

| Variable | Default | Purpose |
| --- | --- | --- |
| `PASSWALL_DATABASE_URL` | Required | PostgreSQL connection string understood by libpq. |
| `PASSWALL_JWT_SECRET` | Required | HS256 signing secret containing at least 32 random bytes. |
| `PASSWALL_TEST_URL` | `http://127.0.0.1:8080` | Server address used by integration tests. |

The listener is `127.0.0.1:8080`. JWT issuer, audience and lifetime default to
`passwall`, `passwall-client` and 900 seconds in `AuthConfig`. These are C++
configuration values, not environment variables.

## Run

Apply the initial schema once to a new database:

```powershell
psql -v ON_ERROR_STOP=1 -d passwall -f apps/server/src/services/repositories/schema.sql
$env:PASSWALL_DATABASE_URL = "host=127.0.0.1 port=5432 dbname=passwall user=passwall connect_timeout=5"
# Supply PASSWALL_JWT_SECRET through your deployment secret configuration.
./build/server/passwall_server.exe
```

The database must already exist. Use PostgreSQL's password file or deployment
secrets for credentials. Schema creation is not automatic; do not reapply the
initial SQL to an initialized database. Terminate HTTPS at a reverse proxy and
configure authentication rate limits there for remote deployments.

## Test

Start the server against a disposable database with the schema applied:

```powershell
python apps/server/tests/test_integration.py
```

Alternatively, enable the opt-in CTest target:

```powershell
cmake -S apps/server -B build/server -DPASSWALL_ENABLE_INTEGRATION_TESTS=ON
ctest --test-dir build/server -C Release --output-on-failure
```

The `passwall_server_integration` test checks routes, validation, duplicate
registration, JWT tampering, owner isolation and logout. Signed invalid-claim
checks run only when the test process has the same `PASSWALL_JWT_SECRET` as the
server. Tests create accounts and leave data behind; use a disposable database.
CTest does not start PostgreSQL or the server.

## Limitations

- `GET /vault/key-info` is not part of OpenAPI and is not implemented. The desktop
  currently keeps encrypted key metadata locally and supports backup/import.
- The request-body limit is 1 MiB. Backend failures return sanitized JSON 500
  responses, including on routes where OpenAPI does not enumerate that fallback.
- Email uniqueness is case-sensitive. Requests authorized before concurrent logout
  may finish after the session is deleted.
- Expired sessions are rejected immediately, but database cleanup is manual:

```sql
DELETE FROM auth_sessions WHERE expires_at <= CURRENT_TIMESTAMP;
```

## Related documentation

- [Server architecture](ARCHITECTURE.md)
- [Desktop setup](../desktop/README.md)
- [Desktop architecture](../desktop/ARCHITECTURE.md)
- [OpenAPI contract](../../api/openapi.yaml)
