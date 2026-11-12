#include "TweakStateTracker.h"

#include "../Logger.h"
#include "Paths.h"
#include "ServiceRegistry.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QLatin1StringView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStringLiteral>
#include <QUuid>

#include <windows.h>
#include <winsvc.h>
#include <comdef.h>
#include <taskschd.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#include <string>
#include <utility>

namespace vth
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catTweakState, "vth.tweakstate")

        HKEY hiveFor(const QString& hive) {
            if (hive.compare(QStringLiteral("HKLM"), Qt::CaseInsensitive) == 0)
                return HKEY_LOCAL_MACHINE;
            if (hive.compare(QStringLiteral("HKCU"), Qt::CaseInsensitive) == 0)
                return HKEY_CURRENT_USER;
            return nullptr;
        }

        // Marks a previous REG_BINARY stored as base64 in prevString.
        constexpr QLatin1StringView kBinaryMarker("[base64] ");

        LONG writeSnapshotToOpenKey(HKEY key, const TweakSnapshot& s) {
            if (!s.existedBefore) {
                const LONG r = RegDeleteValueW(key, s.name.toStdWString().c_str());
                return (r == ERROR_FILE_NOT_FOUND) ? ERROR_SUCCESS : r;
            }
            if (s.isDword) {
                const DWORD v = s.prevDword;
                return RegSetValueExW(key, s.name.toStdWString().c_str(), 0, REG_DWORD,
                                      reinterpret_cast<const BYTE*>(&v), sizeof(v));
            }
            if (s.prevString.startsWith(kBinaryMarker)) {
                const QByteArray bytes = QByteArray::fromBase64(
                    s.prevString.mid(kBinaryMarker.size()).toLatin1());
                return RegSetValueExW(key, s.name.toStdWString().c_str(), 0, REG_BINARY,
                                      reinterpret_cast<const BYTE*>(bytes.constData()),
                                      static_cast<DWORD>(bytes.size()));
            }
            const std::wstring w = s.prevString.toStdWString();
            return RegSetValueExW(key, s.name.toStdWString().c_str(), 0, REG_SZ,
                                  reinterpret_cast<const BYTE*>(w.c_str()),
                                  static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)));
        }

        // Shares open keys across a batched revert.
        class RegistryWriteBatch {
        public:
            RegistryWriteBatch() = default;
            ~RegistryWriteBatch() {
                for (auto it = open_.begin(); it != open_.end(); ++it) {
                    RegCloseKey(it.value());
                }
            }
            RegistryWriteBatch(const RegistryWriteBatch&) = delete;
            RegistryWriteBatch& operator=(const RegistryWriteBatch&) = delete;

            // Null with *missing set when the key is absent and create is false.
            HKEY get(const QString& hive, const QString& subkey, bool create, bool* missing) {
                if (missing) *missing = false;
                const QString cacheKey = hive.toUpper() + QChar::fromLatin1('|') + subkey;
                const auto it = open_.constFind(cacheKey);
                if (it != open_.constEnd()) return it.value();

                HKEY h = hiveFor(hive);
                if (!h) return nullptr;
                HKEY key = nullptr;
                const LONG r = create
                    ? RegCreateKeyExW(h, subkey.toStdWString().c_str(), 0, nullptr, 0,
                                      KEY_WRITE, nullptr, &key, nullptr)
                    : RegOpenKeyExW(h, subkey.toStdWString().c_str(), 0, KEY_WRITE, &key);
                if (r != ERROR_SUCCESS) {
                    if (missing) *missing = r == ERROR_FILE_NOT_FOUND;
                    return nullptr;
                }
                open_.insert(cacheKey, key);
                return key;
            }

        private:
            QHash<QString, HKEY> open_;
        };

        // A deleted key is not a failure: it is recreated if the value existed
        // before, and otherwise there is nothing to do.
        bool restoreRegistryBatched(const TweakSnapshot& s, RegistryWriteBatch& batch) {
            bool missing = false;
            HKEY key = batch.get(s.hive, s.subkey, s.existedBefore, &missing);
            if (!key) return !s.existedBefore && missing;
            return writeSnapshotToOpenKey(key, s) == ERROR_SUCCESS;
        }

        bool restoreService(const TweakSnapshot& s) {
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (!scm) return false;
            SC_HANDLE svc = OpenServiceW(scm, s.serviceName.toStdWString().c_str(),
                                         SERVICE_CHANGE_CONFIG | SERVICE_START);
            if (!svc) { CloseServiceHandle(scm); return false; }
            const bool restored = ChangeServiceConfigW(
                svc, SERVICE_NO_CHANGE, s.prevStartType, SERVICE_NO_CHANGE,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != FALSE;
            if (restored && s.serviceWasRunning) {
                // best effort; the restored start type covers the next boot
                StartServiceW(svc, 0, nullptr);
            }
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return restored;
        }

        // A task that no longer exists counts as done.
        bool restoreTask(const TweakSnapshot& s) {
            if (!s.taskWasEnabled)
                return true;
            const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool ownsCom = SUCCEEDED(init);
            bool ok = false;
            ITaskService* svc = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_ITaskService, reinterpret_cast<void**>(&svc)))) {
                if (SUCCEEDED(svc->Connect(VARIANT{}, VARIANT{}, VARIANT{}, VARIANT{}))) {
                    ITaskFolder* root = nullptr;
                    BSTR rootPath = SysAllocString(L"\\");
                    if (SUCCEEDED(svc->GetFolder(rootPath, &root))) {
                        IRegisteredTask* task = nullptr;
                        BSTR path = SysAllocString(s.taskPath.toStdWString().c_str());
                        const HRESULT hr = root->GetTask(path, &task);
                        if (SUCCEEDED(hr)) {
                            ok = SUCCEEDED(task->put_Enabled(VARIANT_TRUE));
                            task->Release();
                        } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
                                   || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
                            ok = true;
                        }
                        SysFreeString(path);
                        root->Release();
                    }
                    SysFreeString(rootPath);
                }
                svc->Release();
            }
            if (ownsCom)
                CoUninitialize();
            return ok;
        }

        // Per-thread, so two applies in flight don't share a batch.
        thread_local QString tCurrentBatch;
    }

    QString newTweakBatchId(const QString& label) {
        const QString unique = QUuid::createUuid().toString(QUuid::Id128);
        return label.isEmpty() ? unique : label + QLatin1Char(':') + unique;
    }

    TweakBatch::TweakBatch(QString batchId)
        : id_(std::move(batchId))
        , outer_(tCurrentBatch)
    {
        tCurrentBatch = id_;
    }

    TweakBatch::~TweakBatch() {
        tCurrentBatch = outer_;
    }

    QString currentTweakBatch() {
        return tCurrentBatch;
    }

    QString snapshotKindToString(SnapshotKind k) {
        switch (k) {
            case SnapshotKind::Registry: return QStringLiteral("registry");
            case SnapshotKind::Service:  return QStringLiteral("service");
            case SnapshotKind::Task:     return QStringLiteral("task");
        }
        return QStringLiteral("registry");
    }

    SnapshotKind snapshotKindFromString(const QString& s) {
        if (s == QStringLiteral("service")) return SnapshotKind::Service;
        if (s == QStringLiteral("task"))    return SnapshotKind::Task;
        return SnapshotKind::Registry;
    }

    QJsonObject TweakSnapshot::toJson() const {
        QJsonObject o;
        o[QStringLiteral("domain")] = domain;
        o[QStringLiteral("kind")]   = snapshotKindToString(kind);
        o[QStringLiteral("label")]  = label;
        o[QStringLiteral("batch")]  = batchId;
        o[QStringLiteral("at")]     = appliedAt.toString(Qt::ISODateWithMs);
        switch (kind) {
            case SnapshotKind::Registry:
                o[QStringLiteral("hive")]       = hive;
                o[QStringLiteral("subkey")]     = subkey;
                o[QStringLiteral("name")]       = name;
                o[QStringLiteral("existed")]    = existedBefore;
                o[QStringLiteral("isDword")]    = isDword;
                o[QStringLiteral("prevDword")]  = static_cast<qint64>(prevDword);
                o[QStringLiteral("prevString")] = prevString;
                break;
            case SnapshotKind::Service:
                o[QStringLiteral("serviceName")] = serviceName;
                o[QStringLiteral("prevStart")]   = static_cast<int>(prevStartType);
                o[QStringLiteral("wasRunning")]  = serviceWasRunning;
                break;
            case SnapshotKind::Task:
                o[QStringLiteral("taskPath")]   = taskPath;
                o[QStringLiteral("wasEnabled")] = taskWasEnabled;
                break;
        }
        return o;
    }

    TweakSnapshot TweakSnapshot::fromJson(const QJsonObject& obj) {
        TweakSnapshot s;
        s.domain = obj.value(QStringLiteral("domain")).toString();
        s.kind   = snapshotKindFromString(obj.value(QStringLiteral("kind")).toString());
        s.label  = obj.value(QStringLiteral("label")).toString();
        // Absent in older entries, which load as one undated group.
        s.batchId   = obj.value(QStringLiteral("batch")).toString();
        s.appliedAt = QDateTime::fromString(obj.value(QStringLiteral("at")).toString(),
                                            Qt::ISODateWithMs);
        switch (s.kind) {
            case SnapshotKind::Registry:
                s.hive          = obj.value(QStringLiteral("hive")).toString();
                s.subkey        = obj.value(QStringLiteral("subkey")).toString();
                s.name          = obj.value(QStringLiteral("name")).toString();
                s.existedBefore = obj.value(QStringLiteral("existed")).toBool();
                s.isDword       = obj.value(QStringLiteral("isDword")).toBool();
                s.prevDword     = static_cast<quint32>(
                    obj.value(QStringLiteral("prevDword")).toVariant().toLongLong());
                s.prevString    = obj.value(QStringLiteral("prevString")).toString();
                break;
            case SnapshotKind::Service:
                s.serviceName       = obj.value(QStringLiteral("serviceName")).toString();
                s.prevStartType     = static_cast<quint32>(
                    obj.value(QStringLiteral("prevStart")).toInt());
                s.serviceWasRunning = obj.value(QStringLiteral("wasRunning")).toBool();
                break;
            case SnapshotKind::Task:
                s.taskPath       = obj.value(QStringLiteral("taskPath")).toString();
                s.taskWasEnabled = obj.value(QStringLiteral("wasEnabled")).toBool();
                break;
        }
        return s;
    }

    TweakStateTracker::TweakStateTracker() {
        load();
    }

    QString TweakStateTracker::filePath() const {
        return paths::protectedData() + QStringLiteral("/tweak-state.json");
    }

    void TweakStateTracker::load() {
        QFile f(filePath());
        if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
        QJsonParseError err;
        const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isArray()) {
            qCWarning(catTweakState) << "tweak-state.json unreadable:" << err.errorString();
            // The next save would erase the only record of what changed, so keep a copy.
            f.close();
            QFile::copy(filePath(), filePath() + QStringLiteral(".unreadable-")
                        + QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss")));
            return;
        }
        for (const auto& v : doc.array()) {
            entries_.append(TweakSnapshot::fromJson(v.toObject()));
        }
    }

    // Called with mu_ held.
    void TweakStateTracker::saveLocked() {
        paths::ensureDir(paths::protectedData());
        QSaveFile f(filePath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(catTweakState) << "Could not open tweak-state.json for write.";
            return;
        }
        QJsonArray arr;
        for (const auto& e : entries_) arr.append(e.toJson());
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        if (!f.commit())
            qCWarning(catTweakState) << "tweak-state.json not saved:" << f.errorString();
    }

    void TweakStateTracker::record(TweakSnapshot snap) {
        if (snap.appliedAt.isNull())
            snap.appliedAt = QDateTime::currentDateTimeUtc();
        if (snap.batchId.isEmpty())
            snap.batchId = currentTweakBatch();
        // Outside any TweakBatch, a change is a batch of its own.
        if (snap.batchId.isEmpty())
            snap.batchId = newTweakBatchId();

        QMutexLocker lock(&mu_);
        entries_.append(std::move(snap));
        saveLocked();
    }

    QVector<TweakSnapshot> TweakStateTracker::entries() const {
        QMutexLocker lock(&mu_);
        return entries_;
    }

    QVector<TweakSnapshot> TweakStateTracker::entries(const QString& domain) const {
        QMutexLocker lock(&mu_);
        QVector<TweakSnapshot> out;
        for (const auto& e : entries_) {
            if (e.domain == domain) out.append(e);
        }
        return out;
    }

    QStringList TweakStateTracker::domains() const {
        QMutexLocker lock(&mu_);
        QStringList out;
        for (const auto& e : entries_) {
            if (!out.contains(e.domain)) out.append(e.domain);
        }
        return out;
    }

    void TweakStateTracker::clearDomain(const QString& domain) {
        QMutexLocker lock(&mu_);
        const auto inDomain = [&domain](const TweakSnapshot& e) { return e.domain == domain; };
        if (entries_.removeIf(inDomain) > 0)
            saveLocked();
    }

    QVector<TweakBatchGroup> TweakStateTracker::batches() const {
        QMutexLocker lock(&mu_);
        // Walked backwards for newest first. Older entries have no timestamp.
        QVector<TweakBatchGroup> out;
        QHash<QString, int>      indexOf;

        for (int i = entries_.size() - 1; i >= 0; --i) {
            const TweakSnapshot& s = entries_.at(i);
            auto it = indexOf.constFind(s.batchId);
            if (it == indexOf.constEnd()) {
                TweakBatchGroup g;
                g.batchId = s.batchId;
                out.append(std::move(g));
                it = indexOf.insert(s.batchId, out.size() - 1);
            }
            TweakBatchGroup& g = out[it.value()];
            g.entries.append(TweakLedgerEntry{ i, s });
            if (s.appliedAt.isValid() && (!g.appliedAt.isValid() || s.appliedAt > g.appliedAt))
                g.appliedAt = s.appliedAt;
        }
        return out;
    }

    TweakStateTracker::RevertReport TweakStateTracker::revertMatching(
        const std::function<bool(const TweakSnapshot&)>& want) {
        RevertReport report = revertMatchingLocked(want);
        if (report.reverted > 0)
            Q_EMIT reverted();
        return report;
    }

    TweakStateTracker::RevertReport TweakStateTracker::revertMatchingLocked(
        const std::function<bool(const TweakSnapshot&)>& want) {
        QMutexLocker lock(&mu_);
        RevertReport report;
        RegistryWriteBatch batch;
        // Newest first, so layered writes to the same value unwind correctly.
        for (int i = entries_.size() - 1; i >= 0; --i) {
            const auto& s = entries_.at(i);
            if (!want(s)) continue;
            bool ok = false;
            switch (s.kind) {
                case SnapshotKind::Registry: ok = restoreRegistryBatched(s, batch); break;
                case SnapshotKind::Service:  ok = restoreService(s); break;
                case SnapshotKind::Task:     ok = restoreTask(s); break;
            }
            if (ok) {
                entries_.remove(i);
                ++report.reverted;
            } else {
                ++report.failed;
                if (!report.failedDomains.contains(s.domain))
                    report.failedDomains << s.domain;
            }
        }
        if (report.reverted > 0)
            saveLocked();
        return report;
    }

    TweakStateTracker::RevertReport TweakStateTracker::undoDomain(const QString& domain) {
        return revertMatching([&domain](const TweakSnapshot& s) { return s.domain == domain; });
    }

    TweakStateTracker::RevertReport TweakStateTracker::undoBatch(const QString& batchId) {
        return revertMatching([&batchId](const TweakSnapshot& s) { return s.batchId == batchId; });
    }

    TweakStateTracker::RevertReport TweakStateTracker::undoAll() {
        return revertMatching([](const TweakSnapshot&) { return true; });
    }

    void recordTweakSnapshot(TweakSnapshot snap) {
        auto* tracker = ServiceRegistry::instance().get<TweakStateTracker>();
        if (tracker) tracker->record(std::move(snap));
    }

    QVector<TweakSnapshot> tweakSnapshots(const QString& domain) {
        auto* tracker = ServiceRegistry::instance().get<TweakStateTracker>();
        if (!tracker) return {};
        return tracker->entries(domain);
    }

    void clearTweakDomain(const QString& domain) {
        auto* tracker = ServiceRegistry::instance().get<TweakStateTracker>();
        if (tracker) tracker->clearDomain(domain);
    }
}

namespace vth
{
namespace
{
        bool restoreRegistry(const TweakSnapshot& s) {
            HKEY h = hiveFor(s.hive);
            if (!h) return false;
            HKEY key = nullptr;
            const LONG open = s.existedBefore
                ? RegCreateKeyExW(h, s.subkey.toStdWString().c_str(), 0, nullptr, 0,
                                  KEY_WRITE, nullptr, &key, nullptr)
                : RegOpenKeyExW(h, s.subkey.toStdWString().c_str(), 0, KEY_WRITE, &key);
            if (open != ERROR_SUCCESS)
                return !s.existedBefore && open == ERROR_FILE_NOT_FOUND;
            const LONG r = writeSnapshotToOpenKey(key, s);
            RegCloseKey(key);
            return r == ERROR_SUCCESS;
        }
}
}

namespace vth
{
    QString snapshotKindLabel(SnapshotKind k) {
        switch (k) {
            case SnapshotKind::Registry: return QStringLiteral("Registry");
            case SnapshotKind::Service:  return QStringLiteral("Service");
            case SnapshotKind::Task:     return QStringLiteral("Task");
        }
        return {};
    }
    void TweakStateTracker::removeAt(int index) {
        QMutexLocker lock(&mu_);
        if (index < 0 || index >= entries_.size()) return;
        entries_.remove(index);
        saveLocked();
    }
    void TweakStateTracker::clearAll() {
        QMutexLocker lock(&mu_);
        if (entries_.isEmpty()) return;
        entries_.clear();
        saveLocked();
    }
    int TweakStateTracker::revertDomain(const QString& domain) {
        return undoDomain(domain).reverted;
    }
    int TweakStateTracker::revertBatch(const QString& batchId) {
        return undoBatch(batchId).reverted;
    }
    int TweakStateTracker::revertAll() {
        return undoAll().reverted;
    }
    bool TweakStateTracker::revertOne(int index) {
        QMutexLocker lock(&mu_);
        if (index < 0 || index >= entries_.size()) return false;
        const auto& s = entries_.at(index);
        bool ok = false;
        switch (s.kind) {
            case SnapshotKind::Registry: ok = restoreRegistry(s); break;
            case SnapshotKind::Service:  ok = restoreService(s); break;
            case SnapshotKind::Task:     ok = restoreTask(s); break;
        }
        if (ok) {
            entries_.remove(index);
            saveLocked();
        }
        return ok;
    }
    int revertTweakDomain(const QString& domain) {
        auto* tracker = ServiceRegistry::instance().get<TweakStateTracker>();
        if (!tracker) return 0;
        return tracker->revertDomain(domain);
    }
}
