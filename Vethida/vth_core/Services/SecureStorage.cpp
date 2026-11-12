#include "SecureStorage.h"

#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>

#include <windows.h>
#include <dpapi.h>
#pragma comment(lib, "Crypt32.lib")

namespace vth::services
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catSecureStore, "vth.securestore")

        QString storeDir()
        {
            return vth::paths::userData() + QStringLiteral("/secure-data");
        }

        // Not a secret. Changing it makes everything already stored unreadable.
        DATA_BLOB appEntropy()
        {
            static BYTE bytes[] = {
                0x56, 0x65, 0x74, 0x68, 0x69, 0x64, 0x61, 0x2E,
                0x73, 0x65, 0x63, 0x75, 0x72, 0x65, 0x2E, 0x76,
                0x31, 0x9A, 0x3C, 0x71, 0xE4, 0x0B, 0xD2, 0x8F,
                0x46, 0xA1, 0x5D, 0xC8, 0x22, 0xF7, 0x60, 0xBE
            };
            DATA_BLOB b{};
            b.pbData = bytes;
            b.cbData = sizeof(bytes);
            return b;
        }

        QByteArray dpapiProtect(const QByteArray& plaintext)
        {
            DATA_BLOB in{};
            in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plaintext.constData()));
            in.cbData = static_cast<DWORD>(plaintext.size());
            DATA_BLOB entropy = appEntropy();
            DATA_BLOB out{};
            if (!CryptProtectData(&in, nullptr, &entropy, nullptr, nullptr,
                                  CRYPTPROTECT_UI_FORBIDDEN, &out)) {
                return {};
            }
            QByteArray ciphertext(reinterpret_cast<const char*>(out.pbData),
                                  static_cast<int>(out.cbData));
            LocalFree(out.pbData);
            return ciphertext;
        }

        QByteArray dpapiUnprotect(const QByteArray& ciphertext)
        {
            DATA_BLOB in{};
            in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(ciphertext.constData()));
            in.cbData = static_cast<DWORD>(ciphertext.size());
            DATA_BLOB entropy = appEntropy();
            DATA_BLOB out{};
            if (!CryptUnprotectData(&in, nullptr, &entropy, nullptr, nullptr,
                                    CRYPTPROTECT_UI_FORBIDDEN, &out)) {
                return {};
            }
            QByteArray plaintext(reinterpret_cast<const char*>(out.pbData),
                                 static_cast<int>(out.cbData));
            LocalFree(out.pbData);
            return plaintext;
        }
    }

    void SecureStorage::setJson(const QString& key, const QByteArray& json)
    {
        if (!vth::paths::ensureDir(storeDir())) {
            qCWarning(catSecureStore) << "cannot create secure store directory";
            return;
        }
        const QByteArray ciphertext = dpapiProtect(json);
        if (ciphertext.isEmpty()) {
            qCWarning(catSecureStore) << "DPAPI protect failed for key" << key;
            return;
        }
        QSaveFile f(filePath(key));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || f.write(ciphertext) != ciphertext.size()
            || !f.commit()) {
            qCWarning(catSecureStore) << "cannot write secure store file for" << key;
        }
    }

    std::optional<QByteArray> SecureStorage::getJson(const QString& key)
    {
        QFile f(filePath(key));
        if (!f.exists() || !f.open(QIODevice::ReadOnly))
            return std::nullopt;
        const QByteArray plain = dpapiUnprotect(f.readAll());
        if (plain.isEmpty())
            return std::nullopt;
        return plain;
    }

    void SecureStorage::deleteKey(const QString& key)
    {
        QFile::remove(filePath(key));
    }

    QString SecureStorage::filePath(const QString& key)
    {
        return QDir(storeDir()).filePath(sanitizeKey(key) + QStringLiteral(".enc"));
    }

    QString SecureStorage::sanitizeKey(const QString& key)
    {
        QString s;
        s.reserve(key.size());
        for (QChar c : key) {
            if (c.isLetterOrNumber() || c == u'-' || c == u'_')
                s.append(c);
            else
                s.append(u'_');
        }
        return s;
    }
}

namespace vth::services
{
    void SecureStorage::set(const QString& key, const QString& value)
    {
        if (!vth::paths::ensureDir(storeDir())) {
            qCWarning(catSecureStore) << "cannot create secure store directory";
            return;
        }
        const QByteArray ciphertext = dpapiProtect(value.toUtf8());
        if (ciphertext.isEmpty()) {
            qCWarning(catSecureStore) << "DPAPI protect failed for key" << key;
            return;
        }
        QSaveFile f(filePath(key));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(catSecureStore) << "cannot open secure store file for" << key;
            return;
        }
        f.write(ciphertext);
        f.commit();
    }
    std::optional<QString> SecureStorage::get(const QString& key)
    {
        QFile f(filePath(key));
        if (!f.exists() || !f.open(QIODevice::ReadOnly))
            return std::nullopt;
        const QByteArray plain = dpapiUnprotect(f.readAll());
        if (plain.isEmpty())
            return std::nullopt;
        return QString::fromUtf8(plain);
    }
}
