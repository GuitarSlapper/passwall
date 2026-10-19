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
