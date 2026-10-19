#include "api/ApiClient.h"
#include "auth/AuthManager.h"
#include "crypto/CryptoService.h"
#include "vault/VaultManager.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThreadPool>
#include <QTimer>
#include <iostream>
int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Passwall");
    QCoreApplication::setApplicationName("Passwall");
    try
    {
        passwall::desktop::CryptoService crypto;
        passwall::desktop::ApiClient api;
        const auto server = qEnvironmentVariable("PASSWALL_API_URL");
        if (!server.isEmpty() && !api.setBaseUrl(QUrl(server)))
        {
            std::cerr << "Invalid PASSWALL_API_URL\n";
            return 1;
        }
        passwall::desktop::AuthManager auth(api);
        passwall::desktop::VaultManager vault(api, auth);
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("authManager", &auth);
        engine.rootContext()->setContextProperty("vaultManager", &vault);
        QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
        engine.loadFromModule("Passwall", "Main");
        if (engine.rootObjects().isEmpty())
            return 1;
        if (app.arguments().contains("--smoke-test"))
            QTimer::singleShot(250, &app, &QCoreApplication::quit);
        const int result = app.exec();
        vault.lock();
        api.clearToken();
        QThreadPool::globalInstance()->waitForDone();
        return result;
    }
    catch (const std::exception&)
    {
        std::cerr << "Client initialization failed\n";
        return 1;
    }
}
