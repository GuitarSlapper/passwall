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

#pragma once

#include "api/dto/ApiTypes.h"
#include <QJsonObject>
#include <memory>

namespace passwall::desktop
{
class SecretKey final
{
public:
    explicit SecretKey(QByteArray bytes);
    ~SecretKey();
    SecretKey(const SecretKey&) = delete;
    SecretKey& operator=(const SecretKey&) = delete;
    const QByteArray& bytes() const
    {
        return bytes_;
    }

private:
    QByteArray bytes_;
};
using KeyPtr = std::shared_ptr<SecretKey>;

class CryptoService final
{
public:
    CryptoService();
    static VaultKeyInfo createVault(QString masterPassword);
    static KeyPtr unlock(QString masterPassword, const VaultKeyInfo&);
    static EncryptedEntry encryptEntry(const QJsonObject&, const KeyPtr&);
    static QJsonObject decryptEntry(const EncryptedEntry&, const KeyPtr&);
    static void validateKeyInfo(const VaultKeyInfo&);
    static void wipe(QByteArray&);
    static void wipe(QString&);

private:
    static KeyPtr derive(QString& masterPassword, const KdfParameters&);
    static EncryptedEntry encrypt(const QByteArray&, const KeyPtr&, const QByteArray& context);
    static QByteArray decrypt(const EncryptedEntry&, const KeyPtr&, const QByteArray& context);
};
} // namespace passwall::desktop
