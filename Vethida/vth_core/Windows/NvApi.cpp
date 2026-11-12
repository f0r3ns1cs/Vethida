#include "NvApi.h"

#include "../Logger.h"
#include "../Runtime/Paths.h"
#include "../Runtime/Tier.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStringList>

#include <cstring>

#include <windows.h>

// NVAPI loaded at runtime through nvapi_QueryInterface, does nothing without the driver
namespace vth::win::nvapi
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catNvapi, "vth.nvapi")

        // Stable across driver versions.
        constexpr unsigned kIdInitialize          = 0x0150E828u;
        constexpr unsigned kIdDrsCreateSession    = 0x0694D52Eu;
        constexpr unsigned kIdDrsLoadSettings     = 0x375DBD6Bu;
        constexpr unsigned kIdDrsGetGlobalProfile = 0x617BFF9Fu;
        constexpr unsigned kIdDrsSetSetting       = 0x577DD202u;
        constexpr unsigned kIdDrsSaveSettings     = 0xFCBC7E14u;
        constexpr unsigned kIdDrsDestroySession   = 0xDAD9CFF8u;
        // These three are optional, and are checked where they're used.
        constexpr unsigned kIdDrsGetBaseProfile   = 0xDA8466A0u;
        constexpr unsigned kIdDrsGetSetting       = 0x73BF8338u;
        constexpr unsigned kIdDrsRestoreDefault   = 0x53F0381Eu;

        using NvSession = void*;
        using NvProfile = void*;

        // NVDRS_SETTING. The size is part of the version tag, so it must match exactly.
#pragma pack(push, 8)
        struct NvdrsUnion {
            unsigned char rawData[4100];
            void setDword(unsigned value) noexcept { std::memcpy(rawData, &value, sizeof(value)); }
        };
        struct NvdrsSetting {
            unsigned    version;
            wchar_t     settingName[2048];
            unsigned    settingId;
            unsigned    settingType;       // 0 = NVDRS_DWORD_TYPE
            unsigned    settingLocation;   // 0 = NVDRS_CURRENT_PROFILE_LOCATION
            unsigned    isCurrentPredefined;
            unsigned    isPredefinedValid;
            NvdrsUnion  predefinedValue;
            NvdrsUnion  currentValue;
        };
#pragma pack(pop)

        // MAKE_NVAPI_VERSION(NvdrsSetting, 1)
        constexpr unsigned kNvdrsSettingVer = sizeof(NvdrsSetting) | (1u << 16);
        static_assert(kNvdrsSettingVer == 77856u, "NVDRS_SETTING layout drifted from the NVAPI ABI");

        using QueryInterfaceFn = void* (*)(unsigned);
        using InitializeFn     = int (*)();
        using CreateSessionFn  = int (*)(NvSession*);
        using LoadSettingsFn   = int (*)(NvSession);
        using GetGlobalFn      = int (*)(NvSession, NvProfile*);
        using GetBaseFn        = int (*)(NvSession, NvProfile*);
        using SetSettingFn     = int (*)(NvSession, NvProfile, NvdrsSetting*);
        using GetSettingFn     = int (*)(NvSession, NvProfile, unsigned, NvdrsSetting*);
        using RestoreDefaultFn = int (*)(NvSession, NvProfile, unsigned);
        using SaveSettingsFn   = int (*)(NvSession);
        using DestroySessionFn = int (*)(NvSession);

        struct Loader {
            HMODULE          dll             = nullptr;
            InitializeFn     initialize      = nullptr;
            CreateSessionFn  createSession   = nullptr;
            LoadSettingsFn   loadSettings    = nullptr;
            GetGlobalFn      getGlobal       = nullptr;
            GetBaseFn        getBase         = nullptr;
            SetSettingFn     setSetting      = nullptr;
            GetSettingFn     getSetting      = nullptr;
            RestoreDefaultFn restoreDefault  = nullptr;
            SaveSettingsFn   saveSettings    = nullptr;
            DestroySessionFn destroySession  = nullptr;
            bool             ok              = false;
        };

        // Resolved once, and never unloaded.
        const Loader& loader()
        {
            static const Loader loaded = [] {
                Loader l;
                l.dll = ::LoadLibraryW(L"nvapi64.dll");
                if (!l.dll)
                    return l;
                auto query = reinterpret_cast<QueryInterfaceFn>(
                    ::GetProcAddress(l.dll, "nvapi_QueryInterface"));
                if (!query) {
                    qCWarning(catNvapi) << "nvapi_QueryInterface not exported";
                    return l;
                }
                l.initialize     = reinterpret_cast<InitializeFn>(query(kIdInitialize));
                l.createSession  = reinterpret_cast<CreateSessionFn>(query(kIdDrsCreateSession));
                l.loadSettings   = reinterpret_cast<LoadSettingsFn>(query(kIdDrsLoadSettings));
                l.getGlobal      = reinterpret_cast<GetGlobalFn>(query(kIdDrsGetGlobalProfile));
                l.getBase        = reinterpret_cast<GetBaseFn>(query(kIdDrsGetBaseProfile));
                l.setSetting     = reinterpret_cast<SetSettingFn>(query(kIdDrsSetSetting));
                l.getSetting     = reinterpret_cast<GetSettingFn>(query(kIdDrsGetSetting));
                l.restoreDefault = reinterpret_cast<RestoreDefaultFn>(query(kIdDrsRestoreDefault));
                l.saveSettings   = reinterpret_cast<SaveSettingsFn>(query(kIdDrsSaveSettings));
                l.destroySession = reinterpret_cast<DestroySessionFn>(query(kIdDrsDestroySession));
                if (!l.initialize || !l.createSession || !l.loadSettings || !l.getGlobal
                    || !l.setSetting || !l.saveSettings || !l.destroySession) {
                    qCWarning(catNvapi) << "NVAPI DRS entry points unavailable";
                    return l;
                }
                if (l.initialize() != 0) {   // NVAPI_OK == 0
                    qCWarning(catNvapi) << "NvAPI_Initialize failed";
                    return l;
                }
                l.ok = true;
                return l;
            }();
            return loaded;
        }
    }

    bool available()
    {
        return loader().ok;
    }

    const QVector<ProfileSetting>& optimalProfile()
    {
        static const QVector<ProfileSetting> kProfile = {
            { 0x0005F543u, 1u,          "Ultra Low Latency - CPL State" },
            { 0x000F00BAu, 0u,          "Resizable BAR - Feature" },
            { 0x00198FFFu, 1u,          "Shader Cache - Enabled" },
            { 0x0019BB68u, 0u,          "Texture filtering - Negative LOD bias" },
            { 0x002ECAF2u, 0u,          "Texture filtering - Trilinear optimization" },
            { 0x002ED8CDu, 0x00000032u, "Sharpening Value" },
            { 0x002ED8CEu, 0x00000011u, "Sharpening - Denoising Factor" },
            { 0x00598928u, 0u,          "Sharpening Filter" },
            { 0x005A375Cu, 0x96861077u, "Vertical Sync Tear Control" },
            { 0x0064B541u, 1u,          "Preferred Refresh Rate" },
            { 0x00664339u, 0u,          "Predefined Ambient Occlusion Usage" },
            { 0x00667329u, 0u,          "Ambient Occlusion - Setting" },
            { 0x007BA09Eu, 1u,          "Maximum pre-rendered frames" },
            { 0x0084CD70u, 1u,          "Texture filtering - Anisotropic filter optimization" },
            { 0x0098C1ACu, 0u,          "Enable sample interleaving (MFAA)" },
            { 0x00A879CFu, 0x08416747u, "Vertical Sync" },
            { 0x00AC8497u, 0xFFFFFFFFu, "Shader Cache - Cache size" },
            { 0x00CE2691u, 0x00000014u, "Texture filtering - Quality" },
            { 0x00E73211u, 1u,          "Texture filtering - Anisotropic sample optimization" },
            { 0x101AE763u, 0u,          "Vertical Sync - Smooth AFR Behavior" },
            { 0x101E61A9u, 1u,          "Anisotropic filtering setting" },
            { 0x1034CB89u, 1u,          "Antialiasing - FXAA Enabled" },
            { 0x1035DB89u, 0u,          "Predefined Ansel Usage" },
            { 0x103BCCB5u, 0u,          "Prevent Anisotropic Filtering" },
            { 0x10444444u, 0u,          "Quality upscaling" },
            { 0x104554B6u, 0u,          "Application Profile Notification Popup Timeout" },
            { 0x1057EB71u, 1u,          "Power Management - Mode" },
            { 0x1074C972u, 0u,          "Enable FXAA" },
            { 0x1075D972u, 0u,          "Enable Ansel" },
            { 0x107D639Du, 0u,          "Antialiasing - Gamma correction" },
            { 0x107EFC5Bu, 1u,          "Antialiasing (MSAA) - Mode" },
            { 0x10835000u, 0u,          "Ultra Low Latency - Enabled" },
            { 0x1094F157u, 0u,          "G-SYNC - Global Feature" },
            { 0x1094F1F7u, 0u,          "G-SYNC - Global Mode" },
            { 0x10A879ACu, 1u,          "G-SYNC - Application Requested State" },
            { 0x10A879CFu, 1u,          "G-SYNC - Application State" },
            { 0x10D2BB16u, 1u,          "Anisotropic Filtering - Mode" },
            { 0x10D773D2u, 0u,          "Antialiasing (MSAA) - Setting" },
            { 0x1194F158u, 0u,          "G-SYNC - Application Mode" },
            { 0x2089BF6Cu, 0u,          "Antialiasing - Line Gamma" },
            { 0x20C1221Eu, 0x00000002u, "Threaded optimization" },
            { 0x50166C5Eu, 0u,          "CUDA - Force P2 State" },
        };
        return kProfile;
    }

    namespace
    {
        // Written once, because a second apply would otherwise save its own values.
        QString backupPath()
        {
            return vth::paths::protectedData() + QStringLiteral("/nvidia-profile-backup.json");
        }

        // "own" is false for a setting inherited from the driver's defaults, which
        // undo restores to the default.
        bool snapshotGlobal(const Loader& nv, NvSession session, NvProfile global)
        {
            if (!nv.getSetting || QFile::exists(backupPath()))
                return QFile::exists(backupPath());
            QJsonArray rows;
            for (const ProfileSetting& p : optimalProfile()) {
                auto* setting = new NvdrsSetting{};
                setting->version = kNvdrsSettingVer;
                const int st = nv.getSetting(session, global, p.id, setting);
                QJsonObject row;
                row[QStringLiteral("id")] = static_cast<qint64>(p.id);
                const bool own = st == 0 && setting->settingLocation == 0;
                row[QStringLiteral("own")] = own;
                if (own) {
                    unsigned v = 0;
                    std::memcpy(&v, setting->currentValue.rawData, sizeof(v));
                    row[QStringLiteral("value")] = static_cast<qint64>(v);
                }
                delete setting;
                rows.append(row);
            }
            QSaveFile f(backupPath());
            if (!f.open(QIODevice::WriteOnly))
                return false;
            f.write(QJsonDocument(rows).toJson(QJsonDocument::Compact));
            return f.commit();
        }
    }

    bool hasProfileBackup()
    {
        return QFile::exists(backupPath());
    }

    Result revertOptimalProfile()
    {
        Result r;
        const Loader& nv = loader();
        if (!nv.ok) {
            r.error = QStringLiteral("The NVIDIA driver is not available, so its settings cannot be "
                                     "changed right now.");
            return r;
        }
        QFile f(backupPath());
        if (!f.open(QIODevice::ReadOnly)) {
            r.error = QStringLiteral("There is no saved copy of your earlier NVIDIA settings.");
            return r;
        }
        const QJsonArray rows = QJsonDocument::fromJson(f.readAll()).array();
        f.close();

        NvSession session = nullptr;
        if (nv.createSession(&session) != 0 || !session) {
            r.error = QStringLiteral("The NVIDIA driver would not open its settings.");
            return r;
        }
        bool saved = false;
        int failed = 0;
        NvProfile global = nullptr;
        if (nv.loadSettings(session) == 0 && nv.getGlobal(session, &global) == 0 && global) {
            for (const QJsonValue& v : rows) {
                const QJsonObject row = v.toObject();
                const unsigned id = static_cast<unsigned>(row.value(QStringLiteral("id")).toInteger());
                int st = -1;
                if (row.value(QStringLiteral("own")).toBool()) {
                    auto* setting = new NvdrsSetting{};
                    setting->version = kNvdrsSettingVer;
                    setting->settingId = id;
                    setting->settingType = 0;
                    setting->settingLocation = 0;
                    setting->currentValue.setDword(
                        static_cast<unsigned>(row.value(QStringLiteral("value")).toInteger()));
                    st = nv.setSetting(session, global, setting);
                    delete setting;
                } else if (nv.restoreDefault) {
                    st = nv.restoreDefault(session, global, id);
                }
                if (st == 0)
                    ++r.valuesWritten;
                else
                    ++failed;
            }
            saved = nv.saveSettings(session) == 0;
        }
        nv.destroySession(session);

        if (!saved) {
            r.error = QStringLiteral("The NVIDIA driver would not save the change. Close the NVIDIA "
                                     "Control Panel if it is open, then try again.");
            return r;
        }
        r.ok = true;
        if (failed > 0)
            r.error = QStringLiteral("%1 of %2 settings could not be put back.")
                          .arg(failed).arg(rows.size());
        else
            QFile::remove(backupPath());
        qCInfo(catNvapi) << "global profile restored:" << r.valuesWritten << "of" << rows.size();
        return r;
    }

    int applyOptimalProfile(const std::function<void(int done, int total)>& onProgress)
    {
        if (!vth::meetsTier(vth::models::ProductTier::Pro))
            return 0;

        const Loader& nv = loader();
        if (!nv.ok)
            return 0;

        NvSession session = nullptr;
        if (nv.createSession(&session) != 0 || !session) {
            qCWarning(catNvapi) << "DRS_CreateSession failed";
            return 0;
        }

        const QVector<ProfileSetting>& profile = optimalProfile();
        const int total = static_cast<int>(profile.size());
        int applied = 0;

        if (nv.loadSettings(session) == 0) {
            NvProfile global = nullptr;
            if (nv.getGlobal(session, &global) == 0 && global) {
                if (!snapshotGlobal(nv, session, global))
                    qCWarning(catNvapi) << "global profile snapshot not taken; Undo will be unavailable";
                for (int i = 0; i < total; ++i) {
                    NvdrsSetting setting{};
                    setting.version = kNvdrsSettingVer;
                    setting.settingId = profile.at(i).id;
                    setting.settingType = 0;      // NVDRS_DWORD_TYPE
                    setting.settingLocation = 0;  // NVDRS_CURRENT_PROFILE_LOCATION
                    setting.currentValue.setDword(profile.at(i).value);
                    if (nv.setSetting(session, global, &setting) == 0)
                        ++applied;
                    if (onProgress)
                        onProgress(i + 1, total);
                }
                if (nv.saveSettings(session) != 0) {
                    qCWarning(catNvapi) << "DRS_SaveSettings failed";
                    applied = 0;
                }
            } else {
                qCWarning(catNvapi) << "DRS_GetCurrentGlobalProfile failed";
            }
        } else {
            qCWarning(catNvapi) << "DRS_LoadSettings failed";
        }

        nv.destroySession(session);
        return applied;
    }

    namespace
    {
        constexpr unsigned kPowerMizerId          = 0x1057EB71u;
        constexpr unsigned kPowerMizerPreferMax   = 1u;
        constexpr unsigned kPowerMizerAdaptive    = 0u;

        constexpr unsigned kThreadControlId       = 0x20C1221Eu;
        constexpr unsigned kThreadControlOn       = 1u;
        constexpr unsigned kThreadControlAuto     = 0u;

        constexpr unsigned kPreRenderLimitId      = 0x007BA09Eu;
        constexpr unsigned kPreRenderLimitUltra   = 1u;
        constexpr unsigned kPreRenderLimitDefault = 0u;

        constexpr unsigned kVsyncModeId           = 0x00A879CFu;
        constexpr unsigned kVsyncForceOff         = 0x08416747u;
        constexpr unsigned kVsyncAppControlled    = 0x60925292u;

        constexpr unsigned kTexFilterQualityId    = 0x00CE2691u;
        constexpr unsigned kTexFilterHighPerf     = 0x00000014u;
        constexpr unsigned kTexFilterQuality      = 0x00000000u;

        constexpr unsigned kShaderCacheId         = 0x00198FFFu;
        constexpr unsigned kShaderCacheOn         = 1u;

        // A partial write isn't rolled back, because the revert writes all of them.
        Result writeBaseProfile(const QVector<ProfileSetting>& settings,
                                const QString& summary)
        {
            Result r;
            const Loader& nv = loader();
            if (!nv.ok) {
                r.error = QStringLiteral("NVAPI is not available on this system.");
                return r;
            }
            if (!nv.getBase) {
                r.error = QStringLiteral("NvAPI_DRS_GetBaseProfile is not exported by this driver.");
                return r;
            }

            NvSession session = nullptr;
            if (nv.createSession(&session) != 0 || !session) {
                r.error = QStringLiteral("NvAPI_DRS_CreateSession failed.");
                return r;
            }

            if (nv.loadSettings(session) != 0) {
                r.error = QStringLiteral("NvAPI_DRS_LoadSettings failed.");
                nv.destroySession(session);
                return r;
            }

            NvProfile base = nullptr;
            if (nv.getBase(session, &base) != 0 || !base) {
                r.error = QStringLiteral("NvAPI_DRS_GetBaseProfile failed.");
                nv.destroySession(session);
                return r;
            }

            QStringList failures;
            for (const ProfileSetting& p : settings) {
                NvdrsSetting setting{};
                setting.version         = kNvdrsSettingVer;
                setting.settingId       = p.id;
                setting.settingType     = 0;   // NVDRS_DWORD_TYPE
                setting.settingLocation = 0;   // NVDRS_CURRENT_PROFILE_LOCATION
                setting.currentValue.setDword(p.value);
                if (nv.setSetting(session, base, &setting) == 0)
                    ++r.valuesWritten;
                else
                    failures << QString::fromLatin1(p.label);
            }

            if (r.valuesWritten == 0) {
                r.error = QStringLiteral("Every NvAPI_DRS_SetSetting call failed: %1")
                              .arg(failures.join(QStringLiteral(", ")));
                nv.destroySession(session);
                return r;
            }

            if (nv.saveSettings(session) != 0) {
                r.error = QStringLiteral("NvAPI_DRS_SaveSettings failed.");
                nv.destroySession(session);
                return r;
            }

            nv.destroySession(session);
            r.ok = true;
            if (!failures.isEmpty()) {
                r.error = QStringLiteral("Applied %1 of %2; failed for %3")
                              .arg(r.valuesWritten)
                              .arg(settings.size())
                              .arg(failures.join(QStringLiteral(", ")));
            }
            qCInfo(catNvapi).noquote()
                << "DRS base profile:" << summary
                << "-" << r.valuesWritten << "of" << settings.size() << "written";
            return r;
        }
    }

    Result applyBasePerfProfile()
    {
        return writeBaseProfile({
            { kPowerMizerId,       kPowerMizerPreferMax,   "Power management mode" },
            { kThreadControlId,    kThreadControlOn,       "Threaded optimization" },
            { kPreRenderLimitId,   kPreRenderLimitUltra,   "Maximum pre-rendered frames" },
            { kVsyncModeId,        kVsyncForceOff,         "Vertical sync" },
            { kTexFilterQualityId, kTexFilterHighPerf,     "Texture filtering quality" },
            { kShaderCacheId,      kShaderCacheOn,         "Shader cache" },
        }, QStringLiteral("session performance profile"));
    }

    Result revertBasePerfProfile()
    {
        return writeBaseProfile({
            { kPowerMizerId,       kPowerMizerAdaptive,    "Power management mode" },
            { kThreadControlId,    kThreadControlAuto,     "Threaded optimization" },
            { kPreRenderLimitId,   kPreRenderLimitDefault, "Maximum pre-rendered frames" },
            { kVsyncModeId,        kVsyncAppControlled,    "Vertical sync" },
            { kTexFilterQualityId, kTexFilterQuality,      "Texture filtering quality" },
            { kShaderCacheId,      kShaderCacheOn,         "Shader cache" },
        }, QStringLiteral("driver defaults"));
    }
}
