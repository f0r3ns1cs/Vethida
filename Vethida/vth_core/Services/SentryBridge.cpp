#include "SentryBridge.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>

#include <cstdint>
#include "../Logger.h"
#include <QtCore/QString>
#include <cstring>

namespace vth::services
{
    namespace
    {
        // A union of uint64_t and double in sentry.h, so always 8 bytes.
        using sentry_value_t = uint64_t;

        // sentry_value_type_t
        constexpr int SENTRY_VALUE_TYPE_STRING = 4;
        constexpr int SENTRY_VALUE_TYPE_LIST   = 5;
        constexpr int SENTRY_VALUE_TYPE_OBJECT = 6;

        using BeforeSendFn = sentry_value_t (*)(sentry_value_t event, void* hint, void* closure);

        struct Api {
            void* (*optionsNew)() = nullptr;
            void  (*optionsSetDsn)(void*, const char*) = nullptr;
            void  (*optionsSetRelease)(void*, const char*) = nullptr;
            void  (*optionsSetEnvironment)(void*, const char*) = nullptr;
            void  (*optionsSetDatabasePath)(void*, const char*) = nullptr;
            void  (*optionsSetDebug)(void*, int) = nullptr;
            void  (*optionsSetAutoSession)(void*, int) = nullptr;
            void  (*optionsSetBeforeSend)(void*, BeforeSendFn, void*) = nullptr;
            int   (*init)(void*) = nullptr;
            int   (*close)() = nullptr;
            void  (*setTag)(const char*, const char*) = nullptr;

            sentry_value_t (*valueNewString)(const char*) = nullptr;
            int            (*valueSetByKey)(sentry_value_t, const char*, sentry_value_t) = nullptr;
            sentry_value_t (*valueGetByKey)(sentry_value_t, const char*) = nullptr;
            int            (*valueGetType)(sentry_value_t) = nullptr;
            const char*    (*valueAsString)(sentry_value_t) = nullptr;
            size_t         (*valueGetLength)(sentry_value_t) = nullptr;
            sentry_value_t (*valueGetByIndex)(sentry_value_t, size_t) = nullptr;
        };

        struct SentryVTable {
            using OptionsNewFn        = void* (*)();
            using OptionsSetDsnFn     = void  (*)(void* opts, const char* dsn);
            using OptionsSetReleaseFn = void  (*)(void* opts, const char* release);
            using OptionsSetEnvFn     = void  (*)(void* opts, const char* env);
            using OptionsSetDbPathFn  = void  (*)(void* opts, const char* path);
            using OptionsSetDebugFn   = void  (*)(void* opts, int debug);
            using OptionsSetAutoSessFn = void (*)(void* opts, int enable);
            using InitFn              = int   (*)(void* opts);
            using CloseFn             = int   (*)();

            using ValueNewObjectFn  = sentry_value_t (*)();
            using ValueNewStringFn  = sentry_value_t (*)(const char* s);
            using ValueNewBreadcrumbFn = sentry_value_t (*)(const char* type, const char* msg);
            using ValueNewMessageEventFn = sentry_value_t (*)(int level,
                                                              const char* logger,
                                                              const char* text);
            using ValueSetByKeyFn   = int (*)(sentry_value_t obj, const char* k, sentry_value_t v);

            using SetTagFn          = void (*)(const char* k, const char* v);
            using AddBreadcrumbFn   = void (*)(sentry_value_t crumb);
            using CaptureEventFn    = sentry_value_t (*)(sentry_value_t event);

            using BeforeSendCallbackFn = sentry_value_t (*)(sentry_value_t event,
                                                            void* hint,
                                                            void* closure);
            using OptionsSetBeforeSendFn = void (*)(void* opts,
                                                    BeforeSendCallbackFn cb,
                                                    void* closure);
            using ValueGetByKeyFn      = sentry_value_t (*)(sentry_value_t obj, const char* k);
            using ValueGetTypeFn       = int (*)(sentry_value_t v);
            using ValueAsStringFn      = const char* (*)(sentry_value_t v);
            using ValueGetLengthFn     = size_t (*)(sentry_value_t v);
            using ValueGetByIndexFn    = sentry_value_t (*)(sentry_value_t v, size_t i);

            OptionsNewFn        optionsNew = nullptr;
            OptionsSetDsnFn     optionsSetDsn = nullptr;
            OptionsSetReleaseFn optionsSetRelease = nullptr;
            OptionsSetEnvFn     optionsSetEnv = nullptr;
            OptionsSetDbPathFn  optionsSetDbPath = nullptr;
            OptionsSetDebugFn   optionsSetDebug = nullptr;
            OptionsSetAutoSessFn optionsSetAutoSession = nullptr;
            OptionsSetBeforeSendFn optionsSetBeforeSend = nullptr;
            InitFn              init = nullptr;
            CloseFn             close = nullptr;

            ValueNewObjectFn        valueNewObject = nullptr;
            ValueNewStringFn        valueNewString = nullptr;
            ValueNewBreadcrumbFn    valueNewBreadcrumb = nullptr;
            ValueNewMessageEventFn  valueNewMessageEvent = nullptr;
            ValueSetByKeyFn         valueSetByKey = nullptr;
            ValueGetByKeyFn         valueGetByKey = nullptr;
            ValueGetTypeFn          valueGetType = nullptr;
            ValueAsStringFn         valueAsString = nullptr;
            ValueGetLengthFn        valueGetLength = nullptr;
            ValueGetByIndexFn       valueGetByIndex = nullptr;

            SetTagFn        setTag = nullptr;
            AddBreadcrumbFn addBreadcrumb = nullptr;
            CaptureEventFn  captureEvent = nullptr;
        };
        struct LoaderState {
            QMutex   mtx;
            bool     probed = false;
            bool     loaded = false;
            bool     initialized = false;
            QLibrary lib;
            Api      api;
            SentryVTable vt;
        };

        LoaderState& state()
        {
            static LoaderState s;
            return s;
        }

        // crashpad_handler.exe has to be beside sentry.dll.
        QString resolveSentryPath()
        {
            const QDir exeDir(QCoreApplication::applicationDirPath());
            const QString candidates[] = {
                exeDir.filePath(QStringLiteral("sentry.dll")),
                exeDir.filePath(QStringLiteral("bin/sentry.dll")),
            };
            for (const QString& c : candidates) {
                if (QFileInfo(c).isFile()) return c;
            }
            return {};
        }

        template <typename T>
        bool resolve(QLibrary& lib, T& fn, const char* name)
        {
            fn = reinterpret_cast<T>(lib.resolve(name));
            return fn != nullptr;
        }

        // The scrubbing hook is required, so nothing is ever sent unscrubbed.
        bool load(LoaderState& s)
        {
            if (s.probed) return s.loaded;
            s.probed = true;

            const QString path = resolveSentryPath();
            if (path.isEmpty()) return false;
            s.lib.setFileName(path);
            if (!s.lib.load()) return false;

            Api& a = s.api;
            s.loaded = resolve(s.lib, a.optionsNew, "sentry_options_new")
                    && resolve(s.lib, a.optionsSetDsn, "sentry_options_set_dsn")
                    && resolve(s.lib, a.optionsSetRelease, "sentry_options_set_release")
                    && resolve(s.lib, a.optionsSetEnvironment, "sentry_options_set_environment")
                    && resolve(s.lib, a.optionsSetDatabasePath, "sentry_options_set_database_path")
                    && resolve(s.lib, a.optionsSetDebug, "sentry_options_set_debug")
                    && resolve(s.lib, a.optionsSetAutoSession, "sentry_options_set_auto_session_tracking")
                    && resolve(s.lib, a.optionsSetBeforeSend, "sentry_options_set_before_send")
                    && resolve(s.lib, a.init, "sentry_init")
                    && resolve(s.lib, a.close, "sentry_close")
                    && resolve(s.lib, a.setTag, "sentry_set_tag")
                    && resolve(s.lib, a.valueNewString, "sentry_value_new_string")
                    && resolve(s.lib, a.valueSetByKey, "sentry_value_set_by_key")
                    && resolve(s.lib, a.valueGetByKey, "sentry_value_get_by_key")
                    && resolve(s.lib, a.valueGetType, "sentry_value_get_type")
                    && resolve(s.lib, a.valueAsString, "sentry_value_as_string")
                    && resolve(s.lib, a.valueGetLength, "sentry_value_get_length")
                    && resolve(s.lib, a.valueGetByIndex, "sentry_value_get_by_index");
            return s.loaded;
        }

        QString redactString(const QString& input)
        {
            static const QString home =
                QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
            static const QRegularExpression hexDigest(
                QStringLiteral(R"(\b[0-9a-fA-F]{32,}\b)"));
            static const QRegularExpression jwt(
                QStringLiteral(R"(eyJ[A-Za-z0-9_\-]+\.[A-Za-z0-9_\-]+\.[A-Za-z0-9_\-]+)"));

            QString result = input;
            if (!home.isEmpty()) {
                result.replace(home, QStringLiteral("<USERPROFILE>"), Qt::CaseInsensitive);
                result.replace(QDir::toNativeSeparators(home), QStringLiteral("<USERPROFILE>"),
                               Qt::CaseInsensitive);
            }
            result.replace(hexDigest, QStringLiteral("<REDACTED>"));
            result.replace(jwt, QStringLiteral("<JWT>"));
            return result;
        }

        void redactStringField(const Api& a, sentry_value_t obj, const char* key)
        {
            const sentry_value_t v = a.valueGetByKey(obj, key);
            if (a.valueGetType(v) != SENTRY_VALUE_TYPE_STRING) return;

            const char* raw = a.valueAsString(v);
            if (!raw) return;

            const QString original = QString::fromUtf8(raw);
            const QString redacted = redactString(original);
            if (redacted != original)
                a.valueSetByKey(obj, key, a.valueNewString(redacted.toUtf8().constData()));
        }

        // Runs on the SDK's thread.
        extern "C" sentry_value_t scrubBeforeSend(sentry_value_t event, void*, void*)
        {
            const Api& a = state().api;

            // The machine name is often the owner's name.
            a.valueSetByKey(event, "server_name", a.valueNewString("pc"));

            const sentry_value_t message = a.valueGetByKey(event, "message");
            if (a.valueGetType(message) == SENTRY_VALUE_TYPE_OBJECT) {
                redactStringField(a, message, "message");
                redactStringField(a, message, "formatted");
            }

            const sentry_value_t exception = a.valueGetByKey(event, "exception");
            if (a.valueGetType(exception) == SENTRY_VALUE_TYPE_OBJECT) {
                const sentry_value_t values = a.valueGetByKey(exception, "values");
                if (a.valueGetType(values) == SENTRY_VALUE_TYPE_LIST) {
                    const size_t n = a.valueGetLength(values);
                    for (size_t i = 0; i < n; ++i) {
                        const sentry_value_t exc = a.valueGetByIndex(values, i);
                        if (a.valueGetType(exc) == SENTRY_VALUE_TYPE_OBJECT)
                            redactStringField(a, exc, "value");
                    }
                }
            }
            return event;
        }
    }

    bool SentryBridge::initialize(const QString& dsn,
                                  const QString& release,
                                  const QString& environment,
                                  const QString& databasePath,
                                  bool debug)
    {
        auto& s = state();
        QMutexLocker lock(&s.mtx);
        if (!load(s)) return false;
        if (s.initialized) return true;

        const Api& a = s.api;
        void* opts = a.optionsNew();
        if (!opts) return false;

        a.optionsSetDsn(opts, dsn.toUtf8().constData());
        a.optionsSetRelease(opts, release.toUtf8().constData());
        a.optionsSetEnvironment(opts, environment.toUtf8().constData());
        a.optionsSetDatabasePath(opts, databasePath.toUtf8().constData());
        a.optionsSetDebug(opts, debug ? 1 : 0);
        // Session tracking would send a ping on every launch, which isn't what the
        // person agreed to.
        a.optionsSetAutoSession(opts, 0);
        a.optionsSetBeforeSend(opts, &scrubBeforeSend, nullptr);

        // sentry_init takes ownership of the options either way.
        s.initialized = a.init(opts) == 0;
        return s.initialized;
    }

    void SentryBridge::shutdown()
    {
        auto& s = state();
        QMutexLocker lock(&s.mtx);
        if (!s.initialized) return;
        s.api.close();
        s.initialized = false;
    }

    void SentryBridge::setTag(const QString& key, const QString& value)
    {
        auto& s = state();
        QMutexLocker lock(&s.mtx);
        if (!s.initialized) return;
        s.api.setTag(key.toUtf8().constData(), value.toUtf8().constData());
    }
}

namespace vth::services
{
namespace
{
        VTH_DEFINE_LOG_CATEGORY(catSentryBridge, "vth.sentrybridge")
        constexpr int SENTRY_LEVEL_DEBUG   = -1;
        constexpr int SENTRY_LEVEL_INFO    = 0;
        constexpr int SENTRY_LEVEL_WARNING = 1;
        constexpr int SENTRY_LEVEL_ERROR   = 2;
        constexpr int SENTRY_LEVEL_FATAL   = 3;
        constexpr int SENTRY_VALUE_TYPE_NULL   = 0;
        constexpr int SENTRY_VALUE_TYPE_BOOL   = 1;
        constexpr int SENTRY_VALUE_TYPE_INT32  = 2;
        constexpr int SENTRY_VALUE_TYPE_DOUBLE = 3;
        int toSentryLevel(SentryBridge::Level lv)
        {
            switch (lv) {
                case SentryBridge::Level::Debug:   return SENTRY_LEVEL_DEBUG;
                case SentryBridge::Level::Info:    return SENTRY_LEVEL_INFO;
                case SentryBridge::Level::Warning: return SENTRY_LEVEL_WARNING;
                case SentryBridge::Level::Error:   return SENTRY_LEVEL_ERROR;
                case SentryBridge::Level::Fatal:   return SENTRY_LEVEL_FATAL;
            }
            return SENTRY_LEVEL_INFO;
        }
        const char* levelString(SentryBridge::Level lv)
        {
            switch (lv) {
                case SentryBridge::Level::Debug:   return "debug";
                case SentryBridge::Level::Info:    return "info";
                case SentryBridge::Level::Warning: return "warning";
                case SentryBridge::Level::Error:   return "error";
                case SentryBridge::Level::Fatal:   return "fatal";
            }
            return "info";
        }
        QByteArray utf8(const QString& s)
        {
            return s.toUtf8();
        }
        struct RedactionState {
            QString userProfile;
            QRegularExpression hexDigest;
            QRegularExpression jwt;
            bool initialized = false;
            QMutex mtx;
        };
        RedactionState& redactionState()
        {
            static RedactionState s;
            return s;
        }
        void initialiseRedactionState()
        {
            auto& r = redactionState();
            QMutexLocker lock(&r.mtx);
            if (r.initialized) return;
            r.userProfile = QStandardPaths::writableLocation(
                QStandardPaths::HomeLocation);
            r.hexDigest = QRegularExpression(
                QStringLiteral(R"(\b[0-9a-fA-F]{32,}\b)"));
            r.jwt = QRegularExpression(
                QStringLiteral(R"(eyJ[A-Za-z0-9_\-]+\.[A-Za-z0-9_\-]+\.[A-Za-z0-9_\-]+)"));
            r.initialized = true;
        }
}
}

namespace vth::services
{
    bool SentryBridge::tryLoad()
    {
        auto& s = state();
        QMutexLocker lock(&s.mtx);
        if (s.probed) return s.loaded;
        s.probed = true;
        try {
            const QString path = resolveSentryPath();
            if (path.isEmpty()) return false;
            s.lib.setFileName(path);
            if (!s.lib.load()) return false;
            s.vt.optionsNew = reinterpret_cast<SentryVTable::OptionsNewFn>(
                s.lib.resolve("sentry_options_new"));
            s.vt.optionsSetDsn = reinterpret_cast<SentryVTable::OptionsSetDsnFn>(
                s.lib.resolve("sentry_options_set_dsn"));
            s.vt.optionsSetRelease = reinterpret_cast<SentryVTable::OptionsSetReleaseFn>(
                s.lib.resolve("sentry_options_set_release"));
            s.vt.optionsSetEnv = reinterpret_cast<SentryVTable::OptionsSetEnvFn>(
                s.lib.resolve("sentry_options_set_environment"));
            s.vt.optionsSetDbPath = reinterpret_cast<SentryVTable::OptionsSetDbPathFn>(
                s.lib.resolve("sentry_options_set_database_path"));
            s.vt.optionsSetDebug = reinterpret_cast<SentryVTable::OptionsSetDebugFn>(
                s.lib.resolve("sentry_options_set_debug"));
            s.vt.optionsSetAutoSession = reinterpret_cast<SentryVTable::OptionsSetAutoSessFn>(
                s.lib.resolve("sentry_options_set_auto_session_tracking"));
            s.vt.optionsSetBeforeSend = reinterpret_cast<SentryVTable::OptionsSetBeforeSendFn>(
                s.lib.resolve("sentry_options_set_before_send"));
            s.vt.init = reinterpret_cast<SentryVTable::InitFn>(
                s.lib.resolve("sentry_init"));
            s.vt.close = reinterpret_cast<SentryVTable::CloseFn>(
                s.lib.resolve("sentry_close"));
            s.vt.valueNewObject = reinterpret_cast<SentryVTable::ValueNewObjectFn>(
                s.lib.resolve("sentry_value_new_object"));
            s.vt.valueNewString = reinterpret_cast<SentryVTable::ValueNewStringFn>(
                s.lib.resolve("sentry_value_new_string"));
            s.vt.valueNewBreadcrumb = reinterpret_cast<SentryVTable::ValueNewBreadcrumbFn>(
                s.lib.resolve("sentry_value_new_breadcrumb"));
            s.vt.valueNewMessageEvent = reinterpret_cast<SentryVTable::ValueNewMessageEventFn>(
                s.lib.resolve("sentry_value_new_message_event"));
            s.vt.valueSetByKey = reinterpret_cast<SentryVTable::ValueSetByKeyFn>(
                s.lib.resolve("sentry_value_set_by_key"));
            s.vt.valueGetByKey = reinterpret_cast<SentryVTable::ValueGetByKeyFn>(
                s.lib.resolve("sentry_value_get_by_key"));
            s.vt.valueGetType = reinterpret_cast<SentryVTable::ValueGetTypeFn>(
                s.lib.resolve("sentry_value_get_type"));
            s.vt.valueAsString = reinterpret_cast<SentryVTable::ValueAsStringFn>(
                s.lib.resolve("sentry_value_as_string"));
            s.vt.valueGetLength = reinterpret_cast<SentryVTable::ValueGetLengthFn>(
                s.lib.resolve("sentry_value_get_length"));
            s.vt.valueGetByIndex = reinterpret_cast<SentryVTable::ValueGetByIndexFn>(
                s.lib.resolve("sentry_value_get_by_index"));
            s.vt.setTag = reinterpret_cast<SentryVTable::SetTagFn>(
                s.lib.resolve("sentry_set_tag"));
            s.vt.addBreadcrumb = reinterpret_cast<SentryVTable::AddBreadcrumbFn>(
                s.lib.resolve("sentry_add_breadcrumb"));
            s.vt.captureEvent = reinterpret_cast<SentryVTable::CaptureEventFn>(
                s.lib.resolve("sentry_capture_event"));
            s.loaded =
                (s.vt.optionsNew         != nullptr) &&
                (s.vt.optionsSetDsn      != nullptr) &&
                (s.vt.init               != nullptr) &&
                (s.vt.close              != nullptr) &&
                (s.vt.valueNewMessageEvent != nullptr) &&
                (s.vt.captureEvent       != nullptr);
        } catch (const std::exception& ex) {
            qCWarning(catSentryBridge) << "tryLoad failed:" << ex.what();
            s.loaded = false;
        }
        return s.loaded;
    }
    bool SentryBridge::isInitialized()
    {
        auto& s = state();
        QMutexLocker lock(&s.mtx);
        return s.initialized;
    }
    void SentryBridge::captureMessage(const QString& message,
                                      Level level,
                                      const QHash<QString, QString>& tags,
                                      const QHash<QString, QString>& extras)
    {
        auto& s = state();
        {
            QMutexLocker lock(&s.mtx);
            if (!s.initialized) return;
            if (s.vt.valueNewMessageEvent == nullptr) return;
            if (s.vt.captureEvent == nullptr) return;
        }
        try {
            const QByteArray msg = utf8(message);
            sentry_value_t event = s.vt.valueNewMessageEvent(
                toSentryLevel(level), /*logger*/ nullptr, msg.constData());
            if (s.vt.valueNewObject != nullptr &&
                s.vt.valueNewString != nullptr &&
                s.vt.valueSetByKey  != nullptr) {
                if (!tags.isEmpty()) {
                    sentry_value_t tagObj = s.vt.valueNewObject();
                    for (auto it = tags.constBegin(); it != tags.constEnd(); ++it) {
                        const QByteArray k = utf8(it.key());
                        const QByteArray v = utf8(it.value());
                        sentry_value_t vs = s.vt.valueNewString(v.constData());
                        s.vt.valueSetByKey(tagObj, k.constData(), vs);
                    }
                    s.vt.valueSetByKey(event, "tags", tagObj);
                }
                if (!extras.isEmpty()) {
                    sentry_value_t extraObj = s.vt.valueNewObject();
                    for (auto it = extras.constBegin(); it != extras.constEnd(); ++it) {
                        const QByteArray k = utf8(it.key());
                        const QByteArray v = utf8(it.value());
                        sentry_value_t vs = s.vt.valueNewString(v.constData());
                        s.vt.valueSetByKey(extraObj, k.constData(), vs);
                    }
                    s.vt.valueSetByKey(event, "extra", extraObj);
                }
            }
            s.vt.captureEvent(event);
        } catch (const std::exception& ex) {
            qCWarning(catSentryBridge) << "captureMessage failed:" << ex.what();
        }
    }
    void SentryBridge::addBreadcrumb(const QString& message,
                                     const QString& category,
                                     const QString& type,
                                     Level level,
                                     const QHash<QString, QString>& data)
    {
        auto& s = state();
        {
            QMutexLocker lock(&s.mtx);
            if (!s.initialized) return;
            if (s.vt.valueNewBreadcrumb == nullptr) return;
            if (s.vt.addBreadcrumb == nullptr) return;
        }
        try {
            const QByteArray typeUtf8 = utf8(type);
            const QByteArray msgUtf8  = utf8(message);
            sentry_value_t crumb = s.vt.valueNewBreadcrumb(
                type.isEmpty()    ? nullptr : typeUtf8.constData(),
                message.isEmpty() ? nullptr : msgUtf8.constData());
            if (s.vt.valueNewString != nullptr && s.vt.valueSetByKey != nullptr) {
                if (!category.isEmpty()) {
                    const QByteArray cat = utf8(category);
                    sentry_value_t catVal = s.vt.valueNewString(cat.constData());
                    s.vt.valueSetByKey(crumb, "category", catVal);
                }
                const char* lvl = levelString(level);
                sentry_value_t lvlVal = s.vt.valueNewString(lvl);
                s.vt.valueSetByKey(crumb, "level", lvlVal);
                if (!data.isEmpty() && s.vt.valueNewObject != nullptr) {
                    sentry_value_t dataObj = s.vt.valueNewObject();
                    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
                        const QByteArray k = utf8(it.key());
                        const QByteArray v = utf8(it.value());
                        sentry_value_t vs = s.vt.valueNewString(v.constData());
                        s.vt.valueSetByKey(dataObj, k.constData(), vs);
                    }
                    s.vt.valueSetByKey(crumb, "data", dataObj);
                }
            }
            s.vt.addBreadcrumb(crumb);
        } catch (const std::exception& ex) {
            qCWarning(catSentryBridge) << "addBreadcrumb failed:" << ex.what();
        }
    }
}
