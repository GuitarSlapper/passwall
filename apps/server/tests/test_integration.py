"""Black-box API tests. Run against a disposable PostgreSQL database."""

import concurrent.futures
import base64
import datetime
import hashlib
import hmac
import json
import os
import time
import unittest
import urllib.error
import urllib.request
import uuid


BASE = os.environ.get("PASSWALL_TEST_URL", "http://127.0.0.1:8080")


def call(method, path, body=None, token=None, raw=None):
    headers = {"Content-Type": "application/json"}
    if token is not None:
        headers["Authorization"] = "Bearer " + token
    data = raw if raw is not None else (
        json.dumps(body).encode() if body is not None else None
    )
    request = urllib.request.Request(BASE + path, data, headers, method=method)
    try:
        response = urllib.request.urlopen(request, timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        payload = response.read()
        return response.status, json.loads(payload) if payload else None


def registration():
    return {
        "email": f"test-{uuid.uuid4()}@example.com",
        "auth_password": "correct-test-password",
        "kdf": {"algorithm": "argon2id", "salt": "c2FsdA==",
                "memory": 65536, "iterations": 3, "parallelism": 1},
        "encrypted_vault_key": "Y2lwaGVydGV4dA==",
        "vault_key_nonce": "bm9uY2U=",
    }


class ApiTests(unittest.TestCase):
    def account(self):
        request = registration()
        code, user = call("POST", "/register", request)
        self.assertEqual(code, 201, user)
        uuid.UUID(user["id"])
        self.assertEqual(user["email"], request["email"])
        code, login = call("POST", "/login", {
            "email": request["email"], "password": request["auth_password"]})
        self.assertEqual(code, 200, login)
        self.assertEqual(login["token_type"], "Bearer")
        self.assertEqual(len(login["access_token"].split(".")), 3)
        return request, login["access_token"]

    def test_health(self):
        self.assertEqual(call("GET", "/health"),
                         (200, {"status": "ok", "version": "0.0.1"}))
        self.assertEqual(call("GET", "/ready"), (200, {"status": "ready"}))

    def test_validation(self):
        for body in (None, [], {}, {"email": 7}):
            with self.subTest(body=body):
                self.assertEqual(call("POST", "/register", body)[0], 400)
        self.assertEqual(call("POST", "/login", raw=b"{")[0], 400)
        for field in registration():
            body = registration()
            del body[field]
            self.assertEqual(call("POST", "/register", body)[0], 400)
        for field in ("memory", "iterations", "parallelism"):
            for value in (0, -1, True, "1", 1.5):
                body = registration()
                body["kdf"][field] = value
                self.assertEqual(call("POST", "/register", body)[0], 400)
        for password in ("short", "пароль", "😀" * 7):
            body = registration()
            body["auth_password"] = password
            self.assertEqual(call("POST", "/register", body)[0], 400)
        body = registration()
        body["auth_password"] = "😀" * 8
        self.assertEqual(call("POST", "/register", body)[0], 201)
        self.assertEqual(call("POST", "/login", {
            "email": body["email"], "password": body["auth_password"]})[0], 200)

    def test_authentication(self):
        request, token = self.account()
        self.assertEqual(call("POST", "/register", request)[0], 409)
        for email in (request["email"], f"missing-{uuid.uuid4()}@example.com"):
            self.assertEqual(call("POST", "/login", {
                "email": email, "password": "wrong-password"}),
                (401, {"error": "invalid_credentials"}))
        for bad in (None, "invalid", token + "x", "x" + token):
            for method, path in (("POST", "/logout"), ("GET", "/vault/entries"),
                                 ("POST", "/vault/entries"),
                                 ("DELETE", "/vault/entries/" + str(uuid.uuid4()))):
                self.assertEqual(call(method, path,
                    {"encrypted_data": "x", "nonce": "n"}, bad)[0], 401)
        self.assertEqual(call("POST", "/logout", token=token), (204, None))
        self.assertEqual(call("POST", "/logout", token=token)[0], 401)
        self.assertEqual(call("GET", "/vault/entries", token=token)[0], 401)

    def test_vault_owner_isolation(self):
        _, alice = self.account()
        _, bob = self.account()
        self.assertEqual(call("GET", "/vault/entries", token=alice), (200, []))
        self.assertEqual(call("POST", "/vault/entries", {}, alice)[0], 400)
        self.assertEqual(call("POST", "/vault/entries", {"encrypted_data": 1, "nonce": "n"}, alice)[0], 400)
        code, entry = call("POST", "/vault/entries",
                           {"encrypted_data": "opaque-ciphertext", "nonce": "nonce"}, alice)
        self.assertEqual(code, 201, entry)
        uuid.UUID(entry["id"])
        for field in ("created_at", "updated_at"):
            self.assertIsNotNone(datetime.datetime.fromisoformat(entry[field].replace("Z", "+00:00")).tzinfo)
        self.assertEqual(call("GET", "/vault/entries", token=alice), (200, [entry]))
        self.assertEqual(call("GET", "/vault/entries", token=bob), (200, []))
        path = "/vault/entries/" + entry["id"]
        self.assertEqual(call("DELETE", path, token=bob)[0], 404)
        self.assertEqual(call("DELETE", "/vault/entries/not-a-uuid", token=alice)[0], 404)
        self.assertEqual(call("DELETE", path, token=alice), (204, None))
        self.assertEqual(call("DELETE", path, token=alice)[0], 404)
        self.assertEqual(call("GET", "/vault/entries", token=alice), (200, []))

    def test_concurrent_registration(self):
        body = registration()
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
            codes = list(executor.map(lambda _: call("POST", "/register", body)[0], range(2)))
        self.assertEqual(sorted(codes), [201, 409])

    @unittest.skipUnless(os.environ.get("PASSWALL_JWT_SECRET"),
                         "Set the test server's JWT secret to test signed invalid claims")
    def test_jwt_claims(self):
        _, token = self.account()
        header, payload, _ = token.split(".")
        claims = json.loads(base64.urlsafe_b64decode(payload + "=" * (-len(payload) % 4)))

        def encode(value):
            return base64.urlsafe_b64encode(json.dumps(value).encode()).rstrip(b"=").decode()

        def signed(new_claims, new_header=header):
            message = new_header + "." + encode(new_claims)
            digest = hmac.new(os.environ["PASSWALL_JWT_SECRET"].encode(),
                              message.encode(), hashlib.sha256).digest()
            return message + "." + base64.urlsafe_b64encode(digest).rstrip(b"=").decode()

        for field in ("exp", "iat", "iss", "aud", "sub", "jti"):
            altered = dict(claims)
            del altered[field]
            self.assertEqual(call("GET", "/vault/entries", token=signed(altered))[0], 401, field)
        for field, value in (("exp", int(time.time()) - 1),
                             ("iat", int(time.time()) + 3600),
                             ("iss", "wrong-issuer"), ("aud", "wrong-audience"),
                             ("sub", str(uuid.uuid4())), ("jti", str(uuid.uuid4()))):
            altered = dict(claims, **{field: value})
            self.assertEqual(call("GET", "/vault/entries", token=signed(altered))[0], 401, field)
        self.assertEqual(call("GET", "/vault/entries", token=signed(
            claims, encode({"alg": "none", "typ": "JWT"})))[0], 401)
        self.assertEqual(call("POST", "/logout", token=token), (204, None))


if __name__ == "__main__":
    unittest.main(verbosity=2)
