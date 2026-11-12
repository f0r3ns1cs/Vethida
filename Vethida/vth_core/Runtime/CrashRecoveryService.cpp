#include "CrashRecoveryService.h"

#include "../Logger.h"
#include "Paths.h"
#include "Threading.h"

#include "../Services/RegistryEditor.h"
#include "../Tweaks/TweakEngine.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QSaveFile>

namespace vth
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catRecover, "vth.recover")

        constexpr const char* kMarkerFile = "crash-marker.json";
        constexpr int         kMarkerVersion = 1;
    }

    CrashRecoveryService::CrashRecoveryService() = default;
    CrashRecoveryService::~CrashRecoveryService() = default;

    QString CrashRecoveryService::markerPath() const {
        return paths::protectedData()
             + QStringLiteral("/")
             + QString::fromLatin1(kMarkerFile);
    }

    void CrashRecoveryService::initialize() {
        VTH_ASSERT_UI_THREAD();

        QFile f(markerPath());
        if (!f.exists()) {
            qCInfo(catRecover) << "no crash marker - clean prior shutdown";
            return;
        }
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(catRecover) << "marker exists but unreadable:"
                                  << f.errorString();
            return;
        }

        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        f.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qCWarning(catRecover) << "marker parse error:" << err.errorString();
            uncleanDetected_ = true;
            return;
        }

        const QJsonObject root = doc.object();
        priorBatchId_      = root.value(QStringLiteral("batchId")).toString();
        priorBatchStarted_ = QDateTime::fromString(
            root.value(QStringLiteral("startedAt")).toString(), Qt::ISODate);

        priorBatchOps_.clear();
        const QJsonArray ops = root.value(QStringLiteral("ops")).toArray();
        priorBatchOps_.reserve(ops.size());
        for (const auto& v : ops) {
            if (v.isString()) priorBatchOps_.append(v.toString());
        }

        uncleanDetected_ = true;

        qCWarning(catRecover).noquote()
            << "unclean prior shutdown detected - batch"
            << priorBatchId_ << "with" << priorBatchOps_.size()
            << "ops, started" << priorBatchStarted_.toString(Qt::ISODate);
    }

    void CrashRecoveryService::beginBatch(const QString& batchId,
                                          const QStringList& opIds) {
        VTH_ASSERT_UI_THREAD();
        writeMarker(batchId, opIds, QDateTime::currentDateTimeUtc());
    }

    void CrashRecoveryService::endBatch() {
        VTH_ASSERT_UI_THREAD();
        clearMarker();
    }

    void CrashRecoveryService::acknowledge() {
        VTH_ASSERT_UI_THREAD();
        clearMarker();
        uncleanDetected_ = false;
        priorBatchId_.clear();
        priorBatchOps_.clear();
        priorBatchStarted_ = {};
    }

    void CrashRecoveryService::writeMarker(const QString& batchId,
                                           const QStringList& opIds,
                                           const QDateTime& startedAt) {
        paths::ensureDir(paths::protectedData());

        QJsonArray ops;
        for (const auto& s : opIds) ops.append(s);

        QJsonObject root;
        root.insert(QStringLiteral("version"),   kMarkerVersion);
        root.insert(QStringLiteral("batchId"),   batchId);
        root.insert(QStringLiteral("startedAt"), startedAt.toString(Qt::ISODate));
        root.insert(QStringLiteral("ops"),       ops);

        QSaveFile f(markerPath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(catRecover) << "writeMarker: open failed";
            return;
        }
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        if (!f.commit()) {
            qCWarning(catRecover) << "writeMarker: commit failed";
        }
    }

    void CrashRecoveryService::clearMarker() {
        QFile f(markerPath());
        if (f.exists() && !f.remove()) {
            qCWarning(catRecover) << "clearMarker: failed to remove" << f.fileName()
                                  << ":" << f.errorString();
        }
    }

    CrashRecoveryService::RevertOutcome CrashRecoveryService::revertPriorBatch() const
    {
        RevertOutcome out;

        vth::services::RegistryEditor reg;
        for (const QString& id : priorBatchOps_) {
            const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
            if (!spec || !spec->reversible)
                continue;

            if (vth::tweaks::revertVerbose(*spec, reg).ok)
                ++out.reverted;
            else
                ++out.failed;
        }

        out.message = out.failed == 0
            ? QObject::tr("Everything was reverted.")
            : out.reverted == 0
                ? QObject::tr("Nothing could be reverted.")
                : QObject::tr("Some tweaks were reverted.");

        if (out.failed > 0) {
            out.message += QLatin1Char(' ');
            out.message += QObject::tr("The rest are still listed on the Restore page, "
                                       "where each one can be tried again.");
        }
        return out;
    }
}
