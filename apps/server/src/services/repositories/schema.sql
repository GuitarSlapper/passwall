-- Apply once to an empty PostgreSQL database, using psql -v ON_ERROR_STOP=1.
BEGIN;

CREATE TABLE users (
    id uuid PRIMARY KEY,
    email text NOT NULL UNIQUE,
    password_hash text NOT NULL,
    kdf jsonb NOT NULL,
    encrypted_vault_key text NOT NULL,
    vault_key_nonce text NOT NULL
);

CREATE TABLE auth_sessions (
    id uuid PRIMARY KEY,
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at timestamptz NOT NULL
);
CREATE INDEX auth_sessions_expiry ON auth_sessions(expires_at);
CREATE INDEX auth_sessions_user ON auth_sessions(user_id);

CREATE TABLE vault_entries (
    id uuid PRIMARY KEY,
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    encrypted_data text NOT NULL,
    nonce text NOT NULL,
    created_at timestamptz NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at timestamptz NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX vault_entries_owner ON vault_entries(user_id, created_at, id);

COMMIT;
