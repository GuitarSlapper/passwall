#include "api/ApiClient.h"
#include "auth/AuthManager.h"
#include "auth/KeyStore.h"
#include "crypto/CryptoService.h"
#include "vault/VaultManager.h"
#include <QFile>
#include <QJsonArray>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

using namespace passwall::desktop;

class FakeServer : public QTcpServer
{
public:
    QJsonObject registration;
    QJsonArray entries;
    QByteArray lastBody;
    bool authorized{true};
    int overrideStatus{0};
    FakeServer()
    {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this,
                [this]
                {
                    auto* socket = nextPendingConnection();
                    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                    auto buffer = std::make_shared<QByteArray>();
                    connect(
                        socket, &QTcpSocket::readyRead, this,
                        [this, socket, buffer]
                        {
                            buffer->append(socket->readAll());
                            const auto boundary = buffer->indexOf("\r\n\r\n");
                            if (boundary < 0)
                                return;
                            const auto headers = buffer->left(boundary);
                            int length = 0;
                            for (const auto& line : headers.split('\n'))
                                if (line.toLower().startsWith("content-length:"))
                                    length = line.mid(15).trimmed().toInt();
                            if (buffer->size() < boundary + 4 + length)
                                return;
                            lastBody = buffer->mid(boundary + 4, length);
                            const auto line = headers.split('\n').first().split(' ');
                            const auto method = line[0], path = line[1];
                            const auto body = QJsonDocument::fromJson(lastBody).object();
                            int status = 200;
                            QJsonDocument response;
                            if (path == "/register")
                            {
                                registration = body;
                                status = 201;
                                response = QJsonDocument(
                                    QJsonObject{{"id", "12345678-1234-4234-8234-123456789abc"},
                                                {"email", body["email"]}});
                            }
                            else if (path == "/login")
                                response = QJsonDocument(QJsonObject{{"access_token", "test-token"},
                                                                     {"token_type", "Bearer"}});
                            else if (!authorized || !headers.contains("Bearer test-token"))
                                status = 401;
                            else if (path == "/logout")
                                status = 204;
                            else if (method == "GET")
                                response = QJsonDocument(entries);
                            else if (method == "DELETE")
                            {
                                entries = {};
                                status = 204;
                            }
                            else
                            {
                                auto entry = body;
                                entry["id"] = "87654321-1234-4234-8234-123456789abc";
                                entry["created_at"] = "2026-10-02T12:00:00Z";
                                entry["updated_at"] = "2026-10-02T12:00:00Z";
                                entries.append(entry);
                                response = QJsonDocument(entry);
                                status = 201;
                            }
                            if (overrideStatus)
                                status = overrideStatus;
                            const auto data =
                                status == 204
                                    ? QByteArray{}
                                    : (status >= 400 ? QByteArray("{\"error\":\"test_error\"}")
                                                     : response.toJson(QJsonDocument::Compact));
                            socket->write("HTTP/1.1 " + QByteArray::number(status) +
                                          " Test\r\nContent-Type: application/json\r\nConnection: "
                                          "close\r\nContent-Length: " +
                                          QByteArray::number(data.size()) + "\r\n\r\n" + data);
                            socket->disconnectFromHost();
                            buffer->clear();
                        });
                });
    }
    QUrl url() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(serverPort()));
    }
};

class ClientTests : public QObject
{
    Q_OBJECT
    QTemporaryDir dataDirectory_;
private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(dataDirectory_.isValid());
        qputenv("PASSWALL_DATA_DIR", dataDirectory_.path().toUtf8());
        QCoreApplication::setOrganizationName("PasswallTests");
        QCoreApplication::setApplicationName("PasswallTests");
        CryptoService crypto;
    }
    void encryption()
    {
        const auto info = CryptoService::createVault("master-password");
        const auto key = CryptoService::unlock("master-password", info);
        const QJsonObject plain{{"password", "secret"}, {"site", "example"}};
        auto encrypted = CryptoService::encryptEntry(plain, key);
        QCOMPARE(CryptoService::decryptEntry(encrypted, key), plain);
        QVERIFY(!encrypted.ciphertext.contains("secret"));
        QVERIFY(encrypted.nonce != CryptoService::encryptEntry(plain, key).nonce);
        QVERIFY_EXCEPTION_THROWN(CryptoService::unlock("incorrect", info), std::runtime_error);
        encrypted.ciphertext[0] ^= 1;
        QVERIFY_EXCEPTION_THROWN(CryptoService::decryptEntry(encrypted, key), std::runtime_error);
        auto unsafe = info;
        unsafe.kdf.memory = 999999999;
        QVERIFY_EXCEPTION_THROWN(CryptoService::unlock("master-password", unsafe),
                                 std::runtime_error);
        QCOMPARE(ApiClient::keyInfoJson(ApiClient::parseKeyInfo(ApiClient::keyInfoJson(info))),
                 ApiClient::keyInfoJson(info));
    }
    void transportPolicy()
    {
        ApiClient api;
        QVERIFY(!api.setBaseUrl(QUrl("http://example.com")));
        QVERIFY(!api.setBaseUrl(QUrl("https://user:pass@example.com")));
        QVERIFY(api.setBaseUrl(QUrl("https://example.com")));
        FakeServer server;
        QVERIFY(api.setBaseUrl(server.url()));
        server.overrideStatus = 409;
        bool finished = false;
        api.login({"user@example.com", "password"},
                  [&](ApiResult<LoginResponse> result)
                  {
                      QVERIFY(!result);
                      QVERIFY(result.error.contains("already exists"));
                      finished = true;
                  });
        QTRY_VERIFY(finished);
    }
    void lifecycle()
    {
        FakeServer server;
        ApiClient api;
        QVERIFY(api.setBaseUrl(server.url()));
        AuthManager auth(api);
        VaultManager vault(api, auth);
        auth.registerUser("client@example.com", "authentication-password", "master-password");
        QTRY_VERIFY_WITH_TIMEOUT(!auth.busy(), 15000);
        QVERIFY(auth.message().contains("Account created"));
        QCOMPARE(KeyStore::load(api.baseUrl(), "client@example.com").encryptedKey,
                 ApiClient::parseKeyInfo(server.registration).encryptedKey);
        QVERIFY(!QJsonDocument(server.registration).toJson().contains("master-password"));
        QTemporaryDir backup;
        const auto file = QUrl::fromLocalFile(backup.filePath("key.json"));
        auth.exportKey(file);
        QVERIFY(QFile::exists(file.toLocalFile()));
        QVERIFY_EXCEPTION_THROWN(KeyStore::importFile(api.baseUrl(), "other@example.com", file),
                                 std::runtime_error);
        auth.login("client@example.com", "authentication-password");
        QTRY_VERIFY(auth.authenticated());
        QVERIFY(!vault.unlocked());
        auth.importKey(file);
        vault.unlock("master-password");
        QTRY_VERIFY_WITH_TIMEOUT(vault.unlocked() && !vault.busy(), 15000);
        vault.createEntry("example.com", "alice", "vault-secret", "note");
        QTRY_VERIFY(!vault.busy());
        QVERIFY2(vault.message().isEmpty(), qPrintable(vault.message()));
        QCOMPARE(vault.entries().size(), 1);
        QVERIFY(!QJsonDocument(server.entries).toJson().contains("vault-secret"));
        const auto id = vault.entries().first().toMap()["id"].toString();
        QCOMPARE(vault.password(id), QString("vault-secret"));
        vault.deleteEntry(id);
        QTRY_VERIFY(!vault.busy());
        QVERIFY(vault.entries().isEmpty());
        auth.logout();
        QTRY_VERIFY(!auth.authenticated());
        QVERIFY(!vault.unlocked());
        QVERIFY(vault.password(id).isEmpty());
    }
    void expiredSession()
    {
        FakeServer server;
        ApiClient api;
        QVERIFY(api.setBaseUrl(server.url()));
        AuthManager auth(api);
        VaultManager vault(api, auth);
        auth.login("user@example.com", "password123");
        QTRY_VERIFY(auth.authenticated());
        server.authorized = false;
        bool finished = false;
        api.getVaultEntries(
            [&](ApiResult<QVector<VaultEntryDto>> r)
            {
                QVERIFY(!r);
                finished = true;
            });
        QTRY_VERIFY(finished);
        QVERIFY(!auth.authenticated());
        QVERIFY(!vault.unlocked());
    }
};
QTEST_GUILESS_MAIN(ClientTests)
#include "ClientTests.moc"
