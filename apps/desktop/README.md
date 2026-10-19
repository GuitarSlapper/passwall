# Passwall Desktop

## Overview

Qt/QML password-wallet client with client-side encryption. It implements the
[OpenAPI contract](../../api/openapi.yaml) through a typed HTTP client.
See [Architecture](ARCHITECTURE.md) for components and data flows.

## Requirements

- CMake 3.20+ and a C++23 compiler compatible with your Qt installation.
- Qt 6.5+: Core, Gui, Qml, Quick, QuickControls2, Network and Concurrent.
- Qt Test when `BUILD_TESTING` is enabled.
- libsodium 1.0.18+.

## Build

Run commands from the repository root. Make dependencies available through your
package-manager toolchain or `CMAKE_PREFIX_PATH`.

```powershell
cmake -S apps/desktop -B build/desktop -G Ninja `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 `
  -DSODIUM_INCLUDE_DIR=C:/dependencies/libsodium/include `
  -DSODIUM_LIBRARY=C:/dependencies/libsodium/lib/libsodium.dll.a
cmake --build build/desktop --config Release
```

Replace example paths with your installation paths. Use the compiler matching
Qt. The executable is `passwall_desktop` (`.exe` on Windows). Multi-configuration
generators place it under `build/desktop/Release`.

For Windows deployment:

```powershell
windeployqt --qmldir apps/desktop/src/qml build/desktop/passwall_desktop.exe
```

Copy the matching libsodium DLL beside the executable. During development, Qt,
compiler runtime and libsodium DLL directories can instead be added to `PATH`.

## Configuration

| Variable | Default | Purpose |
| --- | --- | --- |
| `PASSWALL_API_URL` | `http://127.0.0.1:8080` | Initial server URL; also editable before login. |
| `PASSWALL_DATA_DIR` | Qt AppLocalDataLocation | Directory containing encrypted key metadata in `keys/`. |

Remote connections require HTTPS. HTTP is allowed only for loopback hosts.
Certificate verification remains enabled and redirects are not followed.

## Run

```powershell
./build/desktop/passwall_desktop.exe
```

1. Register using an authentication password and a separate master password.
2. Export an encrypted key backup and retain it with your backups.
3. Sign in with the authentication password, then unlock with the master password.
4. List, create, reveal or delete vault entries. Lock to clear the open vault.
5. On another device, sign in and import the encrypted key backup before unlocking.

Registration stages an encrypted `.pending.json` candidate before contacting the
server. Confirmed registration and successful local storage remove the candidate.
If the response is lost, import the retained candidate after confirming that the
account exists. A rejected registration candidate is not an existing account's key.

## Test

```powershell
ctest --test-dir build/desktop -C Release --output-on-failure
```

`passwall_desktop_tests` checks cryptography, KDF limits, transport policy,
registration/login/unlock/CRUD/logout, backup identity and session expiry using
a local HTTP test server. `passwall_desktop_qml_smoke` loads the application with
the offscreen platform. These tests do not require PostgreSQL or a running server.

## Limitations

- OpenAPI does not define `GET /vault/key-info`. Key metadata is stored locally;
  importing a compatible backup is required for an account created on another device.
- Losing both local metadata and its backup prevents recovery through the current API.
- Tokens and decrypted entries are memory-only. No offline cache, automatic lock,
  password editing or clipboard integration is implemented.
- Logout locks immediately. If server revocation fails, the local token is discarded,
  but the remote session remains valid until expiration.
- Qt/QML may retain temporary string copies; wiping owned buffers cannot guarantee
  erasure of every copy.

## Related documentation

- [Desktop architecture](ARCHITECTURE.md)
- [Server setup](../server/README.md)
- [Server architecture](../server/ARCHITECTURE.md)
- [OpenAPI contract](../../api/openapi.yaml)
