#include "Updater.h"

#include "Dispatcher.h"
#include "../Http.h"
#include "../Logger.h"
#include "../Services/SecurityConfig.h"
#include "Paths.h"
#include "ServiceRegistry.h"
#include "Settings.h"
#include "Threading.h"
#include "UpdateManifestVerifier.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QSaveFile>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>
#include <QVersionNumber>

#include <QScopeGuard>

#include <string>

#include <windows.h>
#include <softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

namespace vth::core
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catUpdater, "vth.updater")

        constexpr const char* kSettingsChannel = "updater/channel";
        constexpr const char* kDirectiveCache  = "updater/availableCached";

        QString updatesDir()
        {
            return QDir(vth::paths::userData()).filePath(QStringLiteral("updates"));
        }

        QString channelToString(UpdateChannel c)
        {
            switch (c) {
                case UpdateChannel::Stable: return QStringLiteral("stable");
                case UpdateChannel::Beta:   return QStringLiteral("beta");
                case UpdateChannel::Off:    return QStringLiteral("off");
            }
            return QStringLiteral("stable");
        }

        UpdateChannel channelFromString(const QString& s)
        {
            if (s == QStringLiteral("beta")) return UpdateChannel::Beta;
            if (s == QStringLiteral("off"))  return UpdateChannel::Off;
            return UpdateChannel::Stable;
        }

        // no signature check here, only call this after UpdateManifestVerifier passed
        QJsonObject decodeJwsPayload(const QString& jws)
        {
            const QStringList parts = jws.split(QChar::fromLatin1('.'));
            if (parts.size() != 3) return {};
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromBase64(
                parts.at(1).toLatin1(),
                QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
            return doc.isObject() ? doc.object() : QJsonObject{};
        }
    }

    Updater::Updater(QObject* parent) : QObject(parent) {}
    Updater::~Updater() = default;

    void Updater::setStage(UpdateStage s)
    {
        if (state_.stage == s) return;
        state_.stage = s;
        Q_EMIT stageChanged(s);
    }

    void Updater::evaluateFromSettings()
    {
        auto* settings = ServiceRegistry::instance().get<Settings>();
        if (!settings) return;

        channel_ = channelFromString(
            settings->value(QString::fromLatin1(kSettingsChannel),
                            QStringLiteral("stable")).toString());
        if (channel_ == UpdateChannel::Off) {
            qCInfo(catUpdater).noquote() << "updater: channel=off, skipping";
            return;
        }

        // last advertised update, cached so it survives a restart
        const QVariant cached = settings->value(QString::fromLatin1(kDirectiveCache));
        if (!cached.isValid() || cached.toMap().isEmpty()) {
            setStage(UpdateStage::Idle);
            return;
        }
        const auto map = cached.toMap();
        UpdateAvailable u;
        u.version         = map.value(QStringLiteral("version")).toString();
        u.channel         = map.value(QStringLiteral("channel")).toString();
        u.downloadUrl     = map.value(QStringLiteral("download_url")).toString();
        u.sha256          = map.value(QStringLiteral("sha256")).toString();
        u.sizeBytes       = map.value(QStringLiteral("size_bytes")).toLongLong();
        u.manifestJws     = map.value(QStringLiteral("manifest_jws")).toString();
        // releaseNotesUrl / securityFix only come from the signed manifest below

        if (u.version.isEmpty() || u.downloadUrl.isEmpty() || u.sha256.isEmpty()) {
            qCWarning(catUpdater).noquote()
                << "updater: cached update missing required fields, skipping";
            return;
        }
        if (u.channel != channelToString(channel_))
            return; // other channel

        // only newer versions, so an old signed manifest can't be replayed
        const QVersionNumber running = QVersionNumber::fromString(
            QCoreApplication::applicationVersion());
        const QVersionNumber offered = QVersionNumber::fromString(u.version);
        if (offered.isNull() || running.isNull() || offered <= running) {
            qCInfo(catUpdater).noquote()
                << "updater: ignoring non-forward version" << u.version
                << "(running" << QCoreApplication::applicationVersion() << ")";
            return;
        }

        // The signed manifest has to match the advertised version/hash/url.
        // Release builds without the keys refuse everything, dev builds skip the check.
        const bool manifestKeysReady =
            vth::services::SecurityConfig::updateManifestKeysArePopulated();
#ifndef VETHIDA_DEV_BUILD
        if (!manifestKeysReady) {
            qCWarning(catUpdater).noquote()
                << "updater: manifest keys are unpopulated in a release build; "
                   "refusing update for version" << u.version;
            return;
        }
#endif
        if (manifestKeysReady) {
            if (u.manifestJws.isEmpty()) {
                qCWarning(catUpdater).noquote()
                    << "updater: refusing unsigned update for version" << u.version;
                return;
            }
            const auto verdict =
                vth::security::UpdateManifestVerifier::verify(u.manifestJws.toUtf8());
            if (verdict != vth::security::TokenStatus::Valid) {
                qCWarning(catUpdater).noquote()
                    << "updater: refusing update; manifest verify failed, status"
                    << static_cast<int>(verdict);
                return;
            }
            const QJsonObject claims = decodeJwsPayload(u.manifestJws);
            if (claims.value(QStringLiteral("version")).toString() != u.version
                || claims.value(QStringLiteral("sha256")).toString()
                       .compare(u.sha256, Qt::CaseInsensitive) != 0
                || claims.value(QStringLiteral("download_url")).toString() != u.downloadUrl) {
                qCWarning(catUpdater).noquote()
                    << "updater: refusing update; signed manifest claims do not "
                       "match advertised version/hash/url";
                return;
            }

            const QString notes = claims.value(QStringLiteral("release_notes_url")).toString();
            if (!notes.isEmpty()) {
                const QUrl notesUrl(notes);
                if (notesUrl.isValid() && notesUrl.scheme() == QStringLiteral("https"))
                    u.releaseNotesUrl = notes;
                else
                    qCWarning(catUpdater).noquote()
                        << "updater: ignoring non-https release notes URL";
            }
            u.securityFix = claims.value(QStringLiteral("security_fix")).toBool();
        }

        state_.advertised = u;
        setStage(UpdateStage::Idle);
        Q_EMIT availableChanged(u);
    }

    void Updater::onAvailableUpdate(const QString& version,
                                    const QString& channel,
                                    const QString& downloadUrl,
                                    const QString& sha256,
                                    qint64         sizeBytes,
                                    const QString& manifestJws)
    {
        VTH_ASSERT_UI_THREAD();

        auto* settings = ServiceRegistry::instance().get<Settings>();
        if (!settings) return;

        // cache it for the next launch
        QVariantMap m;
        m.insert(QStringLiteral("version"),       version);
        m.insert(QStringLiteral("channel"),       channel);
        m.insert(QStringLiteral("download_url"),  downloadUrl);
        m.insert(QStringLiteral("sha256"),        sha256);
        m.insert(QStringLiteral("size_bytes"),    sizeBytes);
        m.insert(QStringLiteral("manifest_jws"),  manifestJws);
        settings->setValue(QString::fromLatin1(kDirectiveCache), m);

        evaluateFromSettings();

        // auto download is off by default
        if (settings->value(QStringLiteral("updater/autoDownload"),
                            QStringLiteral("false")).toString() == QStringLiteral("true")) {
            beginDownload();
        }
    }

    void Updater::beginDownload()
    {
        VTH_ASSERT_UI_THREAD();

        if (state_.stage == UpdateStage::Downloading
            || state_.stage == UpdateStage::Verifying
            || state_.stage == UpdateStage::Ready
            || state_.stage == UpdateStage::Launching) {
            return; // already running/done
        }

        if (state_.advertised.version.isEmpty()
            || state_.advertised.downloadUrl.isEmpty()
            || state_.advertised.sha256.isEmpty()) {
            Q_EMIT failed(QStringLiteral("no advertised update to download"));
            setStage(UpdateStage::Failed);
            return;
        }

        setStage(UpdateStage::Downloading);
        state_.bytesReceived = 0;
        state_.bytesTotal    = state_.advertised.sizeBytes;
        qCInfo(catUpdater).noquote()
            << "updater: download requested for version"
            << state_.advertised.version
            << "size=" << state_.advertised.sizeBytes;

        const QString updates = updatesDir();
        QDir().mkpath(updates);
        const QString shaPrefix = state_.advertised.sha256.left(12).toLower();
        const QString destPath  = QDir(updates).filePath(
            QStringLiteral("vethida-update-%1-%2.exe")
                .arg(state_.advertised.version, shaPrefix));

        pruneStaleDownloads(destPath);

        QPointer<Updater> self(this);
        const QString url       = state_.advertised.downloadUrl;
        const QString version   = state_.advertised.version;

        Dispatcher::instance().dispatch([self, url, destPath, version]() {
            // whole file in memory, fine for the installer size
            const auto resp = vth::http::get(
                url, /*headers=*/{},
                std::chrono::milliseconds{10 * 60 * 1000});  // 10 min cap

            if (!resp.ok || resp.body.isEmpty()) {
                const QString err = resp.errorString.isEmpty()
                    ? QStringLiteral("download returned empty body")
                    : resp.errorString;
                Dispatcher::instance().dispatchUi([self, err]() {
                    if (!self) return;
                    Q_EMIT self->failed(QStringLiteral("download failed: %1").arg(err));
                    self->setStage(UpdateStage::Failed);
                });
                return;
            }

            QSaveFile file(destPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                const QString err = file.errorString();
                Dispatcher::instance().dispatchUi([self, err]() {
                    if (!self) return;
                    Q_EMIT self->failed(QStringLiteral("open-for-write failed: %1").arg(err));
                    self->setStage(UpdateStage::Failed);
                });
                return;
            }
            if (file.write(resp.body) != resp.body.size() || !file.commit()) {
                const QString err = file.errorString();
                Dispatcher::instance().dispatchUi([self, err]() {
                    if (!self) return;
                    Q_EMIT self->failed(QStringLiteral("write to disk failed: %1").arg(err));
                    self->setStage(UpdateStage::Failed);
                });
                return;
            }

            Dispatcher::instance().dispatchUi([self, destPath, version]() {
                if (!self) return;
                self->state_.localPath     = destPath;
                self->state_.bytesReceived = self->state_.bytesTotal;
                Q_EMIT self->downloadProgress(self->state_.bytesReceived,
                                            self->state_.bytesTotal);
                qCInfo(catUpdater).noquote()
                    << "updater: download complete for" << version
                    << "->" << destPath;
                self->verifyDownload();
            });
        });
    }

    void Updater::verifyDownload()
    {
        VTH_ASSERT_UI_THREAD();

        if (state_.localPath.isEmpty()) {
            Q_EMIT failed(QStringLiteral("verify called with no localPath"));
            setStage(UpdateStage::Failed);
            return;
        }
        setStage(UpdateStage::Verifying);

        // hashing + WinVerifyTrust (does revocation lookups) can take a while, keep them off the UI thread
        QPointer<Updater> self(this);
        const QString path     = state_.localPath;
        const QString expected = state_.advertised.sha256;

        Dispatcher::instance().dispatch([self, path, expected]() {
            if (!self) return;
            VerifyResult r;
            r.actualSha256 = sha256OfFile(path);
            r.hashMatches  = r.actualSha256.compare(expected, Qt::CaseInsensitive) == 0;
            if (r.hashMatches) {
                r.authenticodeOk = self->authenticodeOk(path);
                if (r.authenticodeOk)
                    r.signerCommonName = self->authenticodeSignerCommonName(path);
            }
            Dispatcher::instance().dispatchUi([self, r]() {
                if (!self) return;
                self->onVerifyFinished(r);
            });
        });
    }

    void Updater::onVerifyFinished(const VerifyResult& r)
    {
        VTH_ASSERT_UI_THREAD();

        if (!r.hashMatches) {
            Q_EMIT failed(QStringLiteral("sha256 mismatch (expected %1, got %2)")
                            .arg(state_.advertised.sha256, r.actualSha256));
            setStage(UpdateStage::Failed);
            return;
        }

        if (!r.authenticodeOk) {
            Q_EMIT failed(QStringLiteral("Authenticode verification failed"));
            setStage(UpdateStage::Failed);
            return;
        }

        // check it was signed by us, not just by anyone with a valid cert.
        // Release builds refuse if the publisher isn't pinned.
        if (vth::services::SecurityConfig::updatePublisherIsPinned()) {
            const QString signer = r.signerCommonName;
            const QString expected = QString::fromLatin1(
                vth::services::SecurityConfig::UpdatePublisherCommonName);
            if (signer != expected) {
                qCWarning(catUpdater).noquote()
                    << "updater: refusing installer signed by" << signer
                    << "- expected" << expected;
                Q_EMIT failed(QStringLiteral("installer was not signed by %1").arg(expected));
                setStage(UpdateStage::Failed);
                return;
            }
        } else {
#ifdef VETHIDA_DEV_BUILD
            qCWarning(catUpdater).noquote()
                << "updater: publisher is unpinned; accepting any trusted signer "
                   "(development build only)";
#else
            Q_EMIT failed(QStringLiteral("update publisher is not pinned in this build"));
            setStage(UpdateStage::Failed);
            return;
#endif
        }

        setStage(UpdateStage::Ready);
    }

    void Updater::installAndRestart()
    {
        if (state_.stage != UpdateStage::Ready) {
            qCWarning(catUpdater).noquote()
                << "installAndRestart called in stage" << int(state_.stage);
            return;
        }
        setStage(UpdateStage::Launching);

        // /update = silent + relaunch afterwards (see setup/main.cpp)
        const QStringList args{
            QStringLiteral("/update"),
            QStringLiteral("/norestart"),
        };
        if (!QProcess::startDetached(state_.localPath, args)) {
            Q_EMIT failed(QStringLiteral("failed to spawn installer"));
            setStage(UpdateStage::Failed);
            return;
        }

        qCInfo(catUpdater).noquote()
            << "updater: spawned installer, exiting Vethida";
        QCoreApplication::quit();
    }

    bool Updater::authenticodeOk(const QString& path) const
    {
        if (path.isEmpty()) return false;

        WINTRUST_FILE_INFO fileInfo = {};
        fileInfo.cbStruct       = sizeof(fileInfo);
        fileInfo.pcwszFilePath  = reinterpret_cast<const wchar_t*>(path.utf16());

        GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;

        WINTRUST_DATA data = {};
        data.cbStruct          = sizeof(data);
        data.dwUIChoice        = WTD_UI_NONE;
        data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
        data.dwUnionChoice     = WTD_CHOICE_FILE;
        data.pFile             = &fileInfo;
        data.dwStateAction     = WTD_STATEACTION_VERIFY;

        LONG status = WinVerifyTrust(nullptr, &policy, &data);

        data.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(nullptr, &policy, &data);

        return status == ERROR_SUCCESS;
    }

    // Signer CN or empty on failure. Doesn't validate anything itself, call authenticodeOk first.
    QString Updater::authenticodeSignerCommonName(const QString& path) const
    {
        if (path.isEmpty())
            return {};

        HCERTSTORE store = nullptr;
        HCRYPTMSG  msg   = nullptr;
        DWORD      encoding = 0, contentType = 0, formatType = 0;

        if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                              reinterpret_cast<const wchar_t*>(path.utf16()),
                              CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                              CERT_QUERY_FORMAT_FLAG_BINARY,
                              0, &encoding, &contentType, &formatType,
                              &store, &msg, nullptr)) {
            return {};
        }

        const auto cleanup = qScopeGuard([&]() {
            if (msg)   CryptMsgClose(msg);
            if (store) CertCloseStore(store, 0);
        });

        DWORD infoSize = 0;
        if (!CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &infoSize)
            || infoSize == 0) {
            return {};
        }
        QByteArray infoBuf(static_cast<int>(infoSize), '\0');
        if (!CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, infoBuf.data(), &infoSize))
            return {};

        auto* signer = reinterpret_cast<CMSG_SIGNER_INFO*>(infoBuf.data());
        CERT_INFO lookup = {};
        lookup.Issuer       = signer->Issuer;
        lookup.SerialNumber = signer->SerialNumber;

        PCCERT_CONTEXT cert = CertFindCertificateInStore(
            store, encoding, 0, CERT_FIND_SUBJECT_CERT, &lookup, nullptr);
        if (!cert)
            return {};

        // length includes the null
        const DWORD chars = CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                               0, nullptr, nullptr, 0);
        QString name;
        if (chars > 1) {
            std::wstring buf(chars, L'\0');
            const DWORD written = CertGetNameStringW(
                cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, buf.data(), chars);
            if (written > 1)
                name = QString::fromWCharArray(buf.data(), static_cast<int>(written - 1));
        }
        CertFreeCertificateContext(cert);
        return name;
    }

    void Updater::pruneStaleDownloads(const QString& keepPath)
    {
        QDir dir(updatesDir());
        if (!dir.exists())
            return;

        const QFileInfo keep(keepPath);
        const QFileInfoList stale = dir.entryInfoList(
            { QStringLiteral("vethida-update-*.exe") }, QDir::Files);
        for (const QFileInfo& fi : stale) {
            if (fi.fileName() == keep.fileName())
                continue;
            if (QFile::remove(fi.absoluteFilePath())) {
                qCInfo(catUpdater).noquote()
                    << "updater: removed stale download" << fi.fileName();
            }
        }
    }

    QString Updater::sha256OfFile(const QString& path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return {};
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&f)) return {};
        return QString::fromLatin1(hash.result().toHex());
    }
}
