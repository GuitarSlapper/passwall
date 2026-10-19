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
#include <QUrl>

namespace passwall::desktop
{
class KeyStore final
{
public:
    static VaultKeyInfo load(const QUrl& server, const QString& email);
    static void save(const QUrl& server, const QString& email, const VaultKeyInfo&);
    static QString stage(const QUrl&, const QString&, const VaultKeyInfo&);
    static void exportFile(const QUrl& server, const QString& email, const QUrl& file);
    static void importFile(const QUrl& server, const QString& email, const QUrl& file);
    static void writeBackup(const QUrl&, const QString&, const VaultKeyInfo&, const QUrl& file);

private:
    static QString path(const QUrl&, const QString&);
};
} // namespace passwall::desktop
