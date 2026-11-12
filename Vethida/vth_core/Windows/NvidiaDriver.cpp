#include "NvidiaDriver.h"

#include "NvidiaCleanTables.h"
#include "../Http.h"
#include "../Runtime/Archive.h"
#include "../SystemRestore.h"
#include "../Logger.h"
#include "../Runtime/Paths.h"
#include "../Runtime/ServiceRegistry.h"
#include "../Runtime/TweakStateTracker.h"
#include "Privileges.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QSettings>
#include <QStringLiteral>
#include <QSysInfo>
#include <QUrl>
#include <QXmlStreamReader>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace vth::win::nvidia
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catNvidia, "vth.nvidia")

        // Absolute path, because parts of this run elevated.
        QString system32(const wchar_t* relative) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH) return QString::fromWCharArray(relative);
            return QString::fromWCharArray(dir) + QLatin1Char('\\')
                 + QString::fromWCharArray(relative);
        }

        QString runPowerShell(const QString& script) {
            QProcess p;
            p.setProgram(system32(L"WindowsPowerShell\\v1.0\\powershell.exe"));
            p.setArguments({
                QStringLiteral("-NoProfile"),
                QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"), script,
            });
            p.start();
            if (!p.waitForStarted(5000)) return {};
            p.waitForFinished(120000);
            return QString::fromLocal8Bit(p.readAllStandardOutput());
        }
    }

    // By PCI vendor id, because with no driver bound Windows calls the card
    // "Microsoft Basic Display Adapter". The real name is remembered for that case.
    GpuInfo detectGpu() {
        GpuInfo g;
        HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
        if (set == INVALID_HANDLE_VALUE)
            return g;
        SP_DEVINFO_DATA dev{};
        dev.cbSize = sizeof(dev);
        for (DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
            wchar_t hwid[2048] = {};
            if (!SetupDiGetDeviceRegistryPropertyW(set, &dev, SPDRP_HARDWAREID, nullptr,
                                                   reinterpret_cast<PBYTE>(hwid), sizeof(hwid), nullptr)
                || !QString::fromWCharArray(hwid).contains(QLatin1String("VEN_10DE"), Qt::CaseInsensitive))
                continue;

            wchar_t instance[512] = {};
            if (SetupDiGetDeviceInstanceIdW(set, &dev, instance, static_cast<DWORD>(std::size(instance)), nullptr))
                g.deviceInstanceId = QString::fromWCharArray(instance);

            wchar_t desc[512] = {};
            if (SetupDiGetDeviceRegistryPropertyW(set, &dev, SPDRP_DEVICEDESC, nullptr,
                                                  reinterpret_cast<PBYTE>(desc), sizeof(desc), nullptr))
                g.name = QString::fromWCharArray(desc).trimmed();

            HKEY drv = SetupDiOpenDevRegKey(set, &dev, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
            if (drv != INVALID_HANDLE_VALUE) {
                wchar_t version[64] = {};
                DWORD size = sizeof(version);
                if (RegQueryValueExW(drv, L"DriverVersion", nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(version), &size) == ERROR_SUCCESS)
                    g.driverVersion = QString::fromWCharArray(version);
                RegCloseKey(drv);
            }
            g.detected = true;
            break;
        }
        SetupDiDestroyDeviceInfoList(set);

        QSettings remembered(QSettings::NativeFormat, QSettings::UserScope,
                             QStringLiteral("Rens Engineering"), QStringLiteral("Vethida-hardware"));
        const bool realName = g.detected
            && g.name.contains(QLatin1String("NVIDIA"), Qt::CaseInsensitive);
        if (realName)
            remembered.setValue(QStringLiteral("nvidia/lastGpuName"), g.name);
        else if (g.detected)
            g.name = remembered.value(QStringLiteral("nvidia/lastGpuName")).toString();
        return g;
    }

    DownloadResult downloadFromUrl(const QString& url,
                                   const QString& outputPath,
                                   const DownloadProgressFn& progress,
                                   std::atomic<bool>& cancel) {
        const QUrl u(url);
        if (!u.isValid() || u.host().isEmpty()) return DownloadResult::InvalidUrl;

        QFileInfo fi(outputPath);
        QDir().mkpath(fi.absolutePath());

        QFile out(outputPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(catNvidia) << "downloadFromUrl: open failed:" << out.errorString();
            return DownloadResult::WriteError;
        }

        const vth::http::Download d = vth::http::download(
            url,
            [&out](const char* data, qint64 size) {
                return out.write(data, size) == size;
            },
            [&cancel]() { return cancel.load(); },
            [&progress](qint64 received, qint64 total) {
                if (progress) progress(DownloadProgress{ received, total });
            });

        out.close();

        if (d.canceled)    return DownloadResult::Canceled;
        if (d.sinkRefused) {
            qCWarning(catNvidia) << "downloadFromUrl: write failed:" << out.errorString();
            return DownloadResult::WriteError;
        }
        if (!d.ok) {
            qCWarning(catNvidia) << "downloadFromUrl:" << url << d.errorString;
            return DownloadResult::NetworkError;
        }
        return DownloadResult::Ok;
    }

    namespace
    {
        QByteArray httpGetString(const QString& url) {
            const vth::http::Response r = vth::http::get(url);
            if (!r.ok) {
                qCWarning(catNvidia) << "httpGetString:" << url << r.errorString;
                return {};
            }
            return r.body;
        }

        QString normalizeModel(QString s) {
            s = s.toLower();
            s.remove(QStringLiteral("nvidia"));
            s.remove(QStringLiteral("(r)"));
            s.remove(QStringLiteral("(tm)"));
            s = s.simplified();
            return s;
        }

        // DriverManualLookup returns the package for a psid, pfid and OS, as JSON.
        const char* kAjax =
            "https://gfwsl.geforce.com/services_toolkit/services/com/nvidia/services/AjaxDriverService.php";

        // TypeID 2 is product series and 3 is product. Answers in XML.
        const char* kLookup = "https://www.nvidia.com/Download/API/lookupValueSearch.aspx";

        QVector<QPair<QString, QString>> lookupValues(int typeId, const QString& parentId) {
            QString url = QStringLiteral("%1?TypeID=%2")
                              .arg(QString::fromLatin1(kLookup)).arg(typeId);
            if (!parentId.isEmpty())
                url += QStringLiteral("&ParentID=%1").arg(parentId);

            QVector<QPair<QString, QString>> out;
            QXmlStreamReader xml(httpGetString(url));
            QString name, value;
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == QLatin1String("LookupValue")) {
                        name.clear();
                        value.clear();
                    } else if (xml.name() == QLatin1String("Name")) {
                        name = xml.readElementText().trimmed();
                    } else if (xml.name() == QLatin1String("Value")) {
                        value = xml.readElementText().trimmed();
                    }
                } else if (xml.isEndElement() && xml.name() == QLatin1String("LookupValue")) {
                    if (!name.isEmpty() && !value.isEmpty())
                        out.append({ name, value });
                }
            }
            if (xml.hasError())
                qCWarning(catNvidia) << "lookupValues: bad XML from" << url << xml.errorString();
            return out;
        }
    }

    ResolveResult resolveLatestDriverUrl(const GpuInfo& gpu) {
        ResolveResult r;
        if (!gpu.detected || gpu.name.isEmpty()) {
            r.error = QStringLiteral("No NVIDIA GPU detected.");
            return r;
        }

        const QString geforceTypeId = QStringLiteral("1");   // GeForce family root id
        const QString target = normalizeModel(gpu.name);

        const QVector<QPair<QString, QString>> seriesList =
            lookupValues(2, geforceTypeId);
        if (seriesList.isEmpty()) {
            r.error = QStringLiteral("NVIDIA driver lookup is unavailable right now.");
            return r;
        }

        // Most plausible series first. "RTX 4070" is usually in "GeForce RTX 40 Series".
        const bool mobile = target.contains(QStringLiteral("laptop"))
                         || target.contains(QStringLiteral("notebook"))
                         || target.contains(QStringLiteral("max-q"));
        auto seriesScore = [&](const QString& seriesName) {
            const QString s = normalizeModel(seriesName);
            int score = 0;
            if (target.contains(QStringLiteral("rtx")) && s.contains(QStringLiteral("rtx"))) score += 2;
            if (target.contains(QStringLiteral("gtx")) && s.contains(QStringLiteral("gtx"))) score += 2;
            for (const QString& tok : s.split(QChar::fromLatin1(' '), Qt::SkipEmptyParts))
                if (tok.size() == 2 && tok[0].isDigit() && target.contains(tok)) score += 3;
            if (s.contains(QStringLiteral("notebook")) == mobile) score += 1;
            return score;
        };

        QVector<QPair<QString, QString>> ordered = seriesList;
        std::stable_sort(ordered.begin(), ordered.end(),
            [&](const auto& a, const auto& b) { return seriesScore(a.first) > seriesScore(b.first); });

        // Exact name first, because "4070 Ti SUPER" is listed before "4070" and also
        // contains it.
        struct Candidate { QString psid, pfid, name; };
        QVector<Candidate> seen;
        QString psid, pfid;
        for (const auto& series : ordered) {
            for (const auto& product : lookupValues(3, series.second)) {
                const QString pn = normalizeModel(product.first);
                if (pn.isEmpty()) continue;
                if (pn == target) {
                    psid = series.second;
                    pfid = product.second;
                    break;
                }
                seen.append({ series.second, product.second, pn });
            }
            if (!pfid.isEmpty()) break;
        }

        // Otherwise the longest product name inside the adapter's ("... with Max-Q
        // Design"), then the shortest one containing it, which is the base model.
        if (pfid.isEmpty() && target.size() >= 6) {
            const Candidate* best = nullptr;
            for (const Candidate& c : seen)
                if (c.name.size() >= 6 && target.contains(c.name)
                    && (!best || c.name.size() > best->name.size()))
                    best = &c;
            if (!best) {
                for (const Candidate& c : seen)
                    if (c.name.contains(target)
                        && (!best || c.name.size() < best->name.size()))
                        best = &c;
            }
            if (best) {
                psid = best->psid;
                pfid = best->pfid;
            }
        }

        if (pfid.isEmpty()) {
            r.error = QStringLiteral("Could not match \"%1\" to an NVIDIA product.").arg(gpu.name);
            return r;
        }

        const QString osId = QSysInfo::productVersion().startsWith(QStringLiteral("11"))
            ? QStringLiteral("135")    // Windows 11
            : QStringLiteral("57");    // Windows 10 64-bit

        const QString lookup = QStringLiteral(
            "%1?func=DriverManualLookup&psid=%2&pfid=%3&osID=%4"
            "&languageCode=1033&beta=0&isWHQL=1&dltype=-1&dch=1"
            "&upCRD=0&qnf=0&sort1=0&numberOfResults=1")
            .arg(QString::fromLatin1(kAjax), psid, pfid, osId);

        const QJsonObject root = QJsonDocument::fromJson(httpGetString(lookup)).object();
        const QJsonArray ids = root.value(QStringLiteral("IDS")).toArray();
        if (ids.isEmpty()) {
            r.error = QStringLiteral("No driver was published for this GPU and OS.");
            return r;
        }
        const QJsonObject info = ids.first().toObject()
                                     .value(QStringLiteral("downloadInfo")).toObject();
        r.url     = info.value(QStringLiteral("DownloadURL")).toString();
        r.version = info.value(QStringLiteral("Version")).toString();
        if (r.url.isEmpty()) {
            r.error = QStringLiteral("NVIDIA returned no download URL.");
            return r;
        }

        // Its setup.exe runs elevated, so it must be NVIDIA's host over HTTPS.
        const QUrl resolved(r.url);
        const QString host = resolved.host();
        if (resolved.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
            qCWarning(catNvidia) << "Refusing non-HTTPS driver URL scheme:" << resolved.scheme();
            r.url.clear();
            r.error = QStringLiteral("NVIDIA returned a download URL that is not HTTPS.");
            return r;
        }
        if (host.compare(QStringLiteral("nvidia.com"), Qt::CaseInsensitive) != 0
            && !host.endsWith(QStringLiteral(".nvidia.com"), Qt::CaseInsensitive)) {
            qCWarning(catNvidia) << "Refusing non-NVIDIA driver host:" << host;
            r.url.clear();
            r.error = QStringLiteral("Resolved driver URL host \"%1\" is not an NVIDIA host.")
                          .arg(host);
            return r;
        }

        r.ok = true;
        return r;
    }

    ExtractResult extractPackage(const QString& packagePath,
                                 const QString& outputDir,
                                 QString* errorOut) {
        if (!QFileInfo::exists(packagePath)) {
            if (errorOut) *errorOut = QStringLiteral("Package file not found.");
            return ExtractResult::Failed;
        }
        QDir().mkpath(outputDir);

        const auto res = vth::archive::extract7z(packagePath, outputDir);
        if (!res.ok) {
            if (errorOut) *errorOut = res.error;
            return ExtractResult::Failed;
        }
        return ExtractResult::Ok;
    }

    const QVector<Component>& componentCatalog() {
        static const QVector<Component> kComponents = {
            {QStringLiteral("Display.Driver"),
             QStringLiteral("Display Driver"),
             QStringLiteral("Core kernel + user-mode display driver. Required."),
             /*required=*/true,  /*defaultEnabled=*/true,  {}},

            {QStringLiteral("Display.NvContainer"),
             QStringLiteral("NVIDIA Container"),
             QStringLiteral("Service host required by every other NVIDIA component."),
             /*required=*/true,  /*defaultEnabled=*/true,  {}},

            {QStringLiteral("NVI2"),
             QStringLiteral("NVIDIA Virtual Audio"),
             QStringLiteral("HDMI/DisplayPort audio driver. Disable if you use a "
                            "discrete sound card and never route audio over the GPU."),
             /*required=*/false, /*defaultEnabled=*/true,  {QStringLiteral("Display.Driver")}},

            {QStringLiteral("Display.Optimus"),
             QStringLiteral("Optimus"),
             QStringLiteral("Hybrid graphics switching for laptops. Desktop users can disable."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("HDAudio"),
             QStringLiteral("HD Audio Driver"),
             QStringLiteral("HD Audio (HDMI/DP) - alias for NVI2 on some packages."),
             false, true, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("PhysX"),
             QStringLiteral("PhysX System Software"),
             QStringLiteral("Physics SDK - used by a small set of older games. "
                            "Most modern titles do not require it."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("PPC"),
             QStringLiteral("PPC (PhysX legacy)"),
             QStringLiteral("Legacy PhysX runtime. Off by default."),
             false, false, {QStringLiteral("PhysX")}},

            {QStringLiteral("Display.NView"),
             QStringLiteral("NVIDIA NView"),
             QStringLiteral("Multi-display profiles. Most users use Windows display "
                            "settings instead."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("Display.Update"),
             QStringLiteral("Display Driver Update"),
             QStringLiteral("Update component used by GeForce Experience to push driver "
                            "updates. Disable if you manage drivers manually."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("FrameViewSDK"),
             QStringLiteral("FrameView SDK"),
             QStringLiteral("Performance-measurement SDK. Off by default."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("MSVCRT"),
             QStringLiteral("Visual C++ Runtime"),
             QStringLiteral("Microsoft Visual C++ runtime bundled with NVIDIA installer. "
                            "Safe to disable when the runtime is already installed."),
             false, true, {}},

            {QStringLiteral("NvApp"),
             QStringLiteral("NVIDIA App"),
             QStringLiteral("New unified NVIDIA app (replaces GeForce Experience)."),
             false, false, {QStringLiteral("Display.NvContainer")}},

            {QStringLiteral("NvBackend"),
             QStringLiteral("NVIDIA Backend"),
             QStringLiteral("GeForce Experience backend service."),
             false, false, {QStringLiteral("Display.NvContainer")}},

            {QStringLiteral("NvCpl"),
             QStringLiteral("NVIDIA Control Panel"),
             QStringLiteral("Classic NVIDIA Control Panel - the per-game 3D Settings UI."),
             false, true, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("NvDLISR"),
             QStringLiteral("Deep Learning Super Resolution Driver"),
             QStringLiteral("DLSR support libraries for video upscaling / RTX VSR."),
             false, true, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("NVPCF"),
             QStringLiteral("NVIDIA Performance Counter Framework"),
             QStringLiteral("Performance counter exposure used by FrameView / Reflex."),
             false, true, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("NvvHCI"),
             QStringLiteral("USB-C / Virtual HCI"),
             QStringLiteral("USB-C / VirtualLink controller support for VR HMDs."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("ShieldWirelessController"),
             QStringLiteral("Shield Wireless Controller"),
             QStringLiteral("Shield game controller driver. Off by default unless "
                            "you have a Shield."),
             false, false, {QStringLiteral("Display.Driver")}},

            {QStringLiteral("GFExperience"),
             QStringLiteral("GeForce Experience (legacy)"),
             QStringLiteral("Legacy GeForce Experience client. Replaced by NVIDIA App."),
             false, false, {QStringLiteral("NvBackend")}},

            {QStringLiteral("ShadowPlay"),
             QStringLiteral("ShadowPlay"),
             QStringLiteral("In-game capture / replay overlay. Requires GeForce "
                            "Experience or NVIDIA App."),
             false, false, {QStringLiteral("Display.NvContainer")}},

            {QStringLiteral("NvModuleTracker"),
             QStringLiteral("NVIDIA Module Tracker"),
             QStringLiteral("Telemetry module that tracks loaded driver modules. "
                            "Recommended off."),
             false, false, {QStringLiteral("Display.NvContainer")}},

            {QStringLiteral("nodejs"),
             QStringLiteral("Node.js Runtime"),
             QStringLiteral("Bundled Node.js for the NVIDIA app web shell. Off by default."),
             false, false, {QStringLiteral("NvApp")}},

            {QStringLiteral("NvTelemetry"),
             QStringLiteral("NVIDIA Telemetry"),
             QStringLiteral("Driver telemetry / usage reporting. Recommended off."),
             false, false, {QStringLiteral("Display.NvContainer")}},

            {QStringLiteral("NvVAD"),
             QStringLiteral("NVIDIA Virtual Audio Device"),
             QStringLiteral("RTX Voice / Broadcast virtual audio driver."),
             false, false, {QStringLiteral("Display.Driver")}},
        };
        return kComponents;
    }

    QStringList recommendedComponentIds() {
        QStringList ids;
        for (const auto& c : componentCatalog()) {
            if (c.required || c.defaultEnabled) ids << c.id;
        }
        return ids;
    }

    QSet<QString> resolveDependencies(const QSet<QString>& chosen) {
        QSet<QString> out = chosen;
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& c : componentCatalog()) {
                if (!out.contains(c.id)) continue;
                for (const auto& dep : c.dependsOn) {
                    if (!out.contains(dep)) { out.insert(dep); changed = true; }
                }
            }
        }
        for (const auto& c : componentCatalog()) {
            if (c.required) out.insert(c.id);
        }
        return out;
    }

    const QVector<InstallSetting>& installSettingsCatalog() {
        static const QVector<InstallSetting> kSettings = {
            {QStringLiteral("msihighprio"),
             QStringLiteral("MSI mode + High device priority"),
             QStringLiteral("Forces the GPU to use MSI interrupts and assigns the "
                            "highest device priority. Reduces interrupt latency."),
             /*defaultEnabled=*/true},

            {QStringLiteral("mpo"),
             QStringLiteral("Disable Multi-Plane Overlay"),
             QStringLiteral("Disables MPO. Resolves frame-pacing artifacts on multi-"
                            "monitor systems and HDR oddities; tiny power-draw cost."),
             /*defaultEnabled=*/true},

            {QStringLiteral("hdcp"),
             QStringLiteral("Disable HDCP"),
             QStringLiteral("Disables HDCP signaling. Useful if streaming software / "
                            "capture cards complain about HDCP-protected content. Do "
                            "not disable if you watch DRMed media on this PC."),
             /*defaultEnabled=*/false},

            {QStringLiteral("nvidiaansel"),
             QStringLiteral("Disable Ansel"),
             QStringLiteral("Disables NVIDIA Ansel screenshot tool integration. "
                            "Frees up resources during gaming."),
             /*defaultEnabled=*/true},

            {QStringLiteral("inftelemetry"),
             QStringLiteral("Strip telemetry from installer config (INF/cfg) - modifies vendor installer"),
             QStringLiteral("Removes Telemetry / UsageReporting / UpdatusUser / "
                            "CrashDump / Privacy XML tags from setup.cfg and "
                            "presentations.cfg before setup.exe runs. This edits "
                            "NVIDIA's own installer package (redistribution/EULA "
                            "consideration); prefer the post-install telemetry "
                            "disable instead. Off by default."),
             /*defaultEnabled=*/false},

            {QStringLiteral("drvtlm"),
             QStringLiteral("Disable runtime driver telemetry (post-install)"),
             QStringLiteral("After silent install completes, stops and disables the "
                            "NvTelemetryContainer service and the three NvTm* "
                            "scheduled tasks (NvTmMon / NvTmRep / NvTmRepOnLogon). "
                            "Driver continues to function; live telemetry collection "
                            "and upload are blocked. Reverts via the audit log."),
             /*defaultEnabled=*/true},
        };
        return kSettings;
    }

    namespace
    {
        bool stripTelemetryFromCfg(const QString& path) {
            QFile f(path);
            if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
            const QString content = QString::fromUtf8(f.readAll());
            f.close();

            static const QStringList kKill = {
                QStringLiteral("Telemetry"),
                QStringLiteral("UsageReporting"),
                QStringLiteral("UpdatusUser"),
                QStringLiteral("UpdatusUserManifest"),
                QStringLiteral("CrashDump"),
                QStringLiteral("Privacy"),
            };

            QStringList lines = content.split(QLatin1Char('\n'));
            int dropped = 0;
            for (auto it = lines.begin(); it != lines.end(); ) {
                const QString trimmed = it->trimmed();
                bool matched = false;
                for (const auto& key : kKill) {
                    if (trimmed.contains(QStringLiteral("<%1").arg(key), Qt::CaseInsensitive)
                     || trimmed.contains(QStringLiteral("</%1>").arg(key), Qt::CaseInsensitive)) {
                        matched = true;
                        break;
                    }
                }
                if (matched) { it = lines.erase(it); ++dropped; }
                else         { ++it; }
            }

            if (dropped == 0) return false;

            QSaveFile out(path);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                return false;
            }
            out.write(lines.join(QLatin1Char('\n')).toUtf8());
            return out.commit();
        }

        bool removeComponentSection(const QString& cfgPath, const QString& componentId) {
            QFile f(cfgPath);
            if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
            const QString content = QString::fromUtf8(f.readAll());
            f.close();

            // Lines look like <file name="..." setName="Display.NView" .../>
            QStringList lines = content.split(QLatin1Char('\n'));
            const QString needle = QStringLiteral("setName=\"%1\"").arg(componentId);
            int dropped = 0;
            for (auto it = lines.begin(); it != lines.end(); ) {
                if (it->contains(needle, Qt::CaseInsensitive)) {
                    it = lines.erase(it); ++dropped;
                } else {
                    ++it;
                }
            }

            if (dropped == 0) return false;

            QSaveFile out(cfgPath);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                return false;
            }
            out.write(lines.join(QLatin1Char('\n')).toUtf8());
            return out.commit();
        }
    }

    PatchResult patchPackage(const QString&       extractedDir,
                             const QSet<QString>& componentIdsKept,
                             const QSet<QString>& settingIdsEnabled) {
        PatchResult r;
        if (!QFileInfo(extractedDir).isDir()) {
            r.error = QStringLiteral("Extracted directory does not exist: %1").arg(extractedDir);
            return r;
        }

        const QSet<QString> resolved = resolveDependencies(componentIdsKept);

        const QStringList cfgs = {
            extractedDir + QStringLiteral("/setup.cfg"),
            extractedDir + QStringLiteral("/presentations.cfg"),
        };

        for (const auto& c : componentCatalog()) {
            if (resolved.contains(c.id))
                continue;
            for (const auto& cfg : cfgs)
                removeComponentSection(cfg, c.id);
        }

        if (settingIdsEnabled.contains(QStringLiteral("inftelemetry"))) {
            for (const auto& cfg : cfgs)
                stripTelemetryFromCfg(cfg);
        }

        r.ok = true;
        return r;
    }

    InstallResult runSilentInstall(const QString& extractedDir) {
        InstallResult r;
        // No tier gate, because a lapsed license must not leave a card without a
        // driver when this finishes an interrupted run.
        const QString setup = extractedDir + QStringLiteral("/setup.exe");
        if (!QFileInfo(setup).exists()) {
            r.error = QStringLiteral("setup.exe not found at %1").arg(setup);
            return r;
        }
        QProcess p;
        p.setProgram(setup);
        p.setArguments({
            QStringLiteral("-passive"),
            QStringLiteral("-clean"),
            QStringLiteral("-nosplash"),
            QStringLiteral("-noeula"),
            QStringLiteral("-noreboot"),
            QStringLiteral("-nofinish"),
        });
        p.setWorkingDirectory(extractedDir);
        p.start();
        if (!p.waitForStarted(15'000)) {
            r.error = QStringLiteral("Could not start setup.exe.");
            return r;
        }
        // Stopped if hung, so the fallback doesn't run alongside it.
        if (!p.waitForFinished(45 * 60 * 1000)) {
            p.kill();
            p.waitForFinished(10000);
            r.error = QStringLiteral("The NVIDIA installer stopped responding.");
            return r;
        }
        r.exitCode = p.exitCode();
        if (p.exitStatus() != QProcess::NormalExit) {
            r.error = QStringLiteral("The NVIDIA installer closed unexpectedly.");
            return r;
        }
        if (r.exitCode != 0) {
            r.error = QStringLiteral("The NVIDIA installer reported error %1.").arg(r.exitCode);
            return r;
        }
        r.ok = true;
        return r;
    }

    namespace
    {
        QStringList enumerateNvidiaClassSubkeys();

        const wchar_t* const kProvider = L"NVIDIA";

        // Packages in other setup classes stay, even if NVIDIA provided them.
        const QStringList& driverStoreClasses() {
            static const QStringList k = {
                QStringLiteral("display"),
                QStringLiteral("media"),
                QStringLiteral("extension"),
                QStringLiteral("softwarecomponent"),
                QStringLiteral("system"),
                QStringLiteral("softwaredevice"),
                QStringLiteral("usb"),
            };
            return k;
        }

        // Returns 1 if stopped, 0 if not installed, -1 if still running at the timeout.
        int stopServiceAndWait(const wchar_t* serviceName, int timeoutMs) {
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (!scm) return -1;
            SC_HANDLE svc = OpenServiceW(scm, serviceName,
                                         SERVICE_STOP | SERVICE_QUERY_STATUS);
            if (!svc) {
                const DWORD e = GetLastError();
                CloseServiceHandle(scm);
                return (e == ERROR_SERVICE_DOES_NOT_EXIST) ? 0 : -1;
            }

            SERVICE_STATUS_PROCESS ss{};
            DWORD needed = 0;
            auto query = [&]() -> bool {
                return QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                                            reinterpret_cast<LPBYTE>(&ss),
                                            sizeof(ss), &needed) != 0;
            };

            if (query() && ss.dwCurrentState == SERVICE_STOPPED) {
                CloseServiceHandle(svc);
                CloseServiceHandle(scm);
                return 1;
            }

            SERVICE_STATUS ctl{};
            ControlService(svc, SERVICE_CONTROL_STOP, &ctl);   // may already be stopping

            // NVDisplay.ContainerLocalSystem can take seconds to drop its handles, and
            // starting setup.exe before that leaves the driver half installed.
            const int kStep = 250;
            for (int waited = 0; waited < timeoutMs; waited += kStep) {
                if (query() && ss.dwCurrentState == SERVICE_STOPPED) {
                    CloseServiceHandle(svc);
                    CloseServiceHandle(scm);
                    return 1;
                }
                Sleep(kStep);
            }

            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return -1;
        }

        // Returns 1 if deleted, 0 if not installed, 2 if it goes at the next restart,
        // -1 if it failed.
        int deleteService(const wchar_t* serviceName) {
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (!scm) return -1;
            SC_HANDLE svc = OpenServiceW(scm, serviceName, DELETE);
            if (!svc) {
                const DWORD e = GetLastError();
                CloseServiceHandle(scm);
                return (e == ERROR_SERVICE_DOES_NOT_EXIST) ? 0 : -1;
            }
            const BOOL ok = DeleteService(svc);
            const DWORD err = ok ? ERROR_SUCCESS : GetLastError();
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            if (ok) return 1;
            if (err == ERROR_SERVICE_MARKED_FOR_DELETE) return 2;
            return -1;
        }

        // Through SetupAPI, which resolves tokens like "%NVIDIA%" from [Strings].
        QString infVersionKey(const QString& infPath, const wchar_t* key) {
            const HINF h = SetupOpenInfFileW(infPath.toStdWString().c_str(),
                                             nullptr, INF_STYLE_WIN4, nullptr);
            if (h == INVALID_HANDLE_VALUE) return {};
            QString out;
            INFCONTEXT ctx{};
            if (SetupFindFirstLineW(h, L"Version", key, &ctx)) {
                wchar_t buf[512] = {};
                DWORD needed = 0;
                if (SetupGetStringFieldW(&ctx, 1, buf, DWORD(std::size(buf)), &needed))
                    out = QString::fromWCharArray(buf).trimmed();
            }
            SetupCloseInfFile(h);
            return out;
        }

        struct OemInf {
            QString publishedName;   // oem12.inf
            QString fullPath;        // C:\Windows\INF\oem12.inf
            QString provider;
            QString setupClass;
            QString catalog;
        };

        QVector<OemInf> enumerateOemInfs() {
            QVector<OemInf> out;
            wchar_t winDir[MAX_PATH] = {};
            if (GetWindowsDirectoryW(winDir, MAX_PATH) == 0) return out;

            const QString infDir = QString::fromWCharArray(winDir)
                                 + QStringLiteral("\\INF");
            QDir dir(infDir);
            const QStringList names =
                dir.entryList({ QStringLiteral("oem*.inf") }, QDir::Files);
            out.reserve(names.size());
            for (const QString& name : names) {
                OemInf inf;
                inf.publishedName = name;
                inf.fullPath      = dir.absoluteFilePath(name);
                inf.provider      = infVersionKey(inf.fullPath, L"Provider");
                inf.setupClass    = infVersionKey(inf.fullPath, L"Class");
                inf.catalog       = infVersionKey(inf.fullPath, L"CatalogFile");
                if (inf.provider.isEmpty() && inf.setupClass.isEmpty())
                    continue;
                out.append(inf);
            }
            return out;
        }

        // DiUninstallDriverW also removes the bound devices, which stops Windows
        // reinstalling the driver from the store on a rescan.
        bool uninstallOemInf(const OemInf& inf, bool& rebootOut) {
            using DiUninstallDriverW_t = BOOL (WINAPI*)(HWND, LPCWSTR, DWORD, PBOOL);

            // The uninstall fails on a read-only INF.
            const std::wstring wide = inf.fullPath.toStdWString();
            const DWORD attrs = GetFileAttributesW(wide.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY))
                SetFileAttributesW(wide.c_str(), attrs & ~DWORD(FILE_ATTRIBUTE_READONLY));

            static HMODULE newdev = LoadLibraryW(L"newdev.dll");
            if (newdev) {
                const auto fn = reinterpret_cast<DiUninstallDriverW_t>(
                    GetProcAddress(newdev, "DiUninstallDriverW"));
                if (fn) {
                    BOOL needReboot = FALSE;
                    if (fn(nullptr, wide.c_str(), 0 /* DIURFLAG_DEFAULT */, &needReboot)) {
                        if (needReboot) rebootOut = true;
                        return true;
                    }
                }
            }

            // Without SUOI_FORCEDELETE this refuses on the live display adapter.
            if (SetupUninstallOEMInfW(inf.publishedName.toStdWString().c_str(),
                                      SUOI_FORCEDELETE, nullptr)) {
                rebootOut = true;
                return true;
            }
            return false;
        }

        QString expandEnv(const char* raw) {
            const std::wstring in = QString::fromLatin1(raw).toStdWString();
            wchar_t buf[MAX_PATH * 2] = {};
            const DWORD n = ExpandEnvironmentStringsW(in.c_str(), buf,
                                                     DWORD(std::size(buf)));
            if (n == 0 || n > std::size(buf)) return {};
            QString out = QString::fromWCharArray(buf);
            // An unset variable comes back as the literal "%Name%".
            if (out.contains(QLatin1Char('%'))) return {};
            return out;
        }

        // The exit code, or -1 if it didn't finish.
        int runSchtasks(const QStringList& args) {
            QProcess p;
            p.setProgram(system32(L"schtasks.exe"));
            p.setArguments(args);
            p.start();
            if (!p.waitForStarted(5000) || !p.waitForFinished(15000)
                || p.exitStatus() != QProcess::NormalExit)
                return -1;
            return p.exitCode();
        }

        bool deleteScheduledTask(const QString& taskPath) {
            return runSchtasks({ QStringLiteral("/Delete"), QStringLiteral("/TN"), taskPath,
                                 QStringLiteral("/F") }) == 0;
        }

        // An elevated token holds some privileges disabled until they're switched on.
        bool enablePrivilege(const wchar_t* name) {
            HANDLE token = nullptr;
            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
                return false;
            TOKEN_PRIVILEGES tp{};
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            bool ok = false;
            if (LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid)) {
                AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
                ok = (GetLastError() == ERROR_SUCCESS);
            }
            CloseHandle(token);
            return ok;
        }

        // For keys whose ACL denies even an elevated token.
        bool takeKeyOwnership(HKEY root, const QString& subkey) {
            static const bool privileged = [] {
                const bool a = enablePrivilege(SE_TAKE_OWNERSHIP_NAME);
                const bool b = enablePrivilege(SE_RESTORE_NAME);
                return a || b;
            }();
            if (!privileged)
                return false;

            const std::wstring w = subkey.toStdWString();
            HKEY key = nullptr;
            if (RegOpenKeyExW(root, w.c_str(), 0, WRITE_OWNER, &key) != ERROR_SUCCESS)
                return false;

            BYTE sidBuf[SECURITY_MAX_SID_SIZE] = {};
            DWORD sidSize = sizeof(sidBuf);
            auto* admins = reinterpret_cast<PSID>(sidBuf);
            if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr,
                                    admins, &sidSize)) {
                RegCloseKey(key);
                return false;
            }

            SECURITY_DESCRIPTOR sd{};
            if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)
                || !SetSecurityDescriptorOwner(&sd, admins, FALSE)
                || RegSetKeySecurity(key, OWNER_SECURITY_INFORMATION, &sd)
                       != ERROR_SUCCESS) {
                RegCloseKey(key);
                return false;
            }
            RegCloseKey(key);

            if (RegOpenKeyExW(root, w.c_str(), 0, WRITE_DAC, &key) != ERROR_SUCCESS)
                return false;

            const DWORD aclSize = sizeof(ACL)
                                + sizeof(ACCESS_ALLOWED_ACE)
                                + GetLengthSid(admins);
            std::vector<BYTE> aclBuf(aclSize, 0);
            auto* dacl = reinterpret_cast<PACL>(aclBuf.data());
            bool ok = false;
            if (InitializeAcl(dacl, aclSize, ACL_REVISION)
                && AddAccessAllowedAce(dacl, ACL_REVISION, KEY_ALL_ACCESS, admins)) {
                SECURITY_DESCRIPTOR dsd{};
                if (InitializeSecurityDescriptor(&dsd, SECURITY_DESCRIPTOR_REVISION)
                    && SetSecurityDescriptorDacl(&dsd, TRUE, dacl, FALSE)) {
                    ok = RegSetKeySecurity(key, DACL_SECURITY_INFORMATION, &dsd)
                             == ERROR_SUCCESS;
                }
            }
            RegCloseKey(key);
            return ok;
        }

        QString regDefaultString(HKEY root, const QString& subkey) {
            HKEY key = nullptr;
            if (RegOpenKeyExW(root, subkey.toStdWString().c_str(), 0,
                              KEY_READ, &key) != ERROR_SUCCESS)
                return {};
            wchar_t buf[512] = {};
            DWORD size = sizeof(buf);
            DWORD type = 0;
            QString out;
            if (RegQueryValueExW(key, nullptr, nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS
                && (type == REG_SZ || type == REG_EXPAND_SZ))
                out = QString::fromWCharArray(buf).trimmed();
            RegCloseKey(key);
            return out;
        }

        QString regValueString(HKEY root, const QString& subkey, const wchar_t* name) {
            HKEY key = nullptr;
            if (RegOpenKeyExW(root, subkey.toStdWString().c_str(), 0,
                              KEY_READ, &key) != ERROR_SUCCESS)
                return {};
            wchar_t buf[512] = {};
            DWORD size = sizeof(buf);
            DWORD type = 0;
            QString out;
            if (RegQueryValueExW(key, name, nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS
                && (type == REG_SZ || type == REG_EXPAND_SZ))
                out = QString::fromWCharArray(buf).trimmed();
            RegCloseKey(key);
            return out;
        }

        QStringList regSubkeyNames(HKEY root, const QString& subkey) {
            QStringList out;
            HKEY key = nullptr;
            if (RegOpenKeyExW(root, subkey.toStdWString().c_str(), 0,
                              KEY_READ | KEY_ENUMERATE_SUB_KEYS, &key) != ERROR_SUCCESS)
                return out;
            for (DWORD i = 0;; ++i) {
                wchar_t name[512] = {};
                DWORD len = DWORD(std::size(name));
                const LSTATUS s = RegEnumKeyExW(key, i, name, &len,
                                                nullptr, nullptr, nullptr, nullptr);
                if (s == ERROR_NO_MORE_ITEMS) break;
                if (s != ERROR_SUCCESS) continue;
                out << QString::fromWCharArray(name, int(len));
            }
            RegCloseKey(key);
            return out;
        }

        bool deleteTree(HKEY root, const QString& subkey) {
            HKEY probe = nullptr;
            const std::wstring w = subkey.toStdWString();
            if (RegOpenKeyExW(root, w.c_str(), 0, KEY_READ, &probe) != ERROR_SUCCESS)
                return false;
            RegCloseKey(probe);
            if (RegDeleteTreeW(root, w.c_str()) == ERROR_SUCCESS)
                return true;
            if (!takeKeyOwnership(root, subkey))
                return false;
            return RegDeleteTreeW(root, w.c_str()) == ERROR_SUCCESS;
        }

        bool anyContains(const QString& haystack, const QStringList& needles) {
            for (const QString& n : needles)
                if (!n.isEmpty() && haystack.contains(n, Qt::CaseInsensitive))
                    return true;
            return false;
        }

        bool anyStartsWith(const QString& haystack, const QStringList& needles) {
            for (const QString& n : needles)
                if (!n.isEmpty() && haystack.startsWith(n, Qt::CaseInsensitive))
                    return true;
            return false;
        }

        // Removes the CLSID along with its AppID and TypeLib.
        void removeClsidSatellites(const QString& clsidGuid, QStringList& removed) {
            if (clsidGuid.isEmpty())
                return;
            const QString base = QStringLiteral("CLSID\\") + clsidGuid;

            const QString appId = regValueString(HKEY_CLASSES_ROOT, base, L"AppID");
            if (!appId.isEmpty()
                && deleteTree(HKEY_CLASSES_ROOT, QStringLiteral("AppID\\") + appId))
                removed << QStringLiteral("HKCR\\AppID\\") + appId;

            const QString typeLib =
                regDefaultString(HKEY_CLASSES_ROOT, base + QStringLiteral("\\TypeLib"));
            if (!typeLib.isEmpty()
                && deleteTree(HKEY_CLASSES_ROOT, QStringLiteral("TypeLib\\") + typeLib))
                removed << QStringLiteral("HKCR\\TypeLib\\") + typeLib;

            if (deleteTree(HKEY_CLASSES_ROOT, base))
                removed << QStringLiteral("HKCR\\") + base;
        }

        void cleanClassRoots(const QStringList& prefixes, QStringList& removed) {
            for (const QString& child : regSubkeyNames(HKEY_CLASSES_ROOT, QString())) {
                if (!anyStartsWith(child, prefixes))
                    continue;
                const QString clsid = regDefaultString(
                    HKEY_CLASSES_ROOT, child + QStringLiteral("\\CLSID"));
                removeClsidSatellites(clsid, removed);
                if (deleteTree(HKEY_CLASSES_ROOT, child))
                    removed << QStringLiteral("HKCR\\") + child;
            }
        }

        // By the registration's text, because the GUIDs change between driver branches.
        void cleanClsidLeftovers(const QStringList& needles, QStringList& removed) {
            for (const QString& guid : regSubkeyNames(HKEY_CLASSES_ROOT,
                                                      QStringLiteral("CLSID"))) {
                const QString base = QStringLiteral("CLSID\\") + guid;
                QString haystack = regDefaultString(HKEY_CLASSES_ROOT, base);
                haystack += QLatin1Char(' ')
                    + regDefaultString(HKEY_CLASSES_ROOT,
                                       base + QStringLiteral("\\InprocServer32"));
                haystack += QLatin1Char(' ')
                    + regDefaultString(HKEY_CLASSES_ROOT,
                                       base + QStringLiteral("\\LocalServer32"));
                haystack += QLatin1Char(' ')
                    + regValueString(HKEY_CLASSES_ROOT, base, L"AppID");
                if (haystack.trimmed().isEmpty())
                    continue;
                if (!anyContains(haystack, needles))
                    continue;
                removeClsidSatellites(guid, removed);
            }
        }

        void cleanInterfaces(const QStringList& needles, QStringList& removed) {
            for (const QString& guid : regSubkeyNames(HKEY_CLASSES_ROOT,
                                                      QStringLiteral("Interface"))) {
                const QString base = QStringLiteral("Interface\\") + guid;
                const QString name = regDefaultString(HKEY_CLASSES_ROOT, base);
                if (name.isEmpty() || !anyContains(name, needles))
                    continue;
                const QString typeLib =
                    regDefaultString(HKEY_CLASSES_ROOT, base + QStringLiteral("\\TypeLib"));
                if (!typeLib.isEmpty()
                    && deleteTree(HKEY_CLASSES_ROOT, QStringLiteral("TypeLib\\") + typeLib))
                    removed << QStringLiteral("HKCR\\TypeLib\\") + typeLib;
                if (deleteTree(HKEY_CLASSES_ROOT, base))
                    removed << QStringLiteral("HKCR\\") + base;
            }
        }

        void uninstallMsiProducts(const QStringList& needles, QStringList& removed) {
            static const QString kRoots[] = {
                QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                QStringLiteral("SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
            };
            QStringList done;
            for (const QString& root : kRoots) {
                for (const QString& code : regSubkeyNames(HKEY_LOCAL_MACHINE, root)) {
                    // An MSI product's Uninstall key is named by its ProductCode.
                    if (!code.startsWith(QLatin1Char('{')) || !code.endsWith(QLatin1Char('}')))
                        continue;
                    if (done.contains(code, Qt::CaseInsensitive))
                        continue;
                    const QString key = root + QLatin1Char('\\') + code;
                    const QString name = regValueString(HKEY_LOCAL_MACHINE, key, L"DisplayName");
                    if (name.isEmpty() || !anyContains(name, needles))
                        continue;
                    const QString uninstall =
                        regValueString(HKEY_LOCAL_MACHINE, key, L"UninstallString");
                    if (!uninstall.contains(QStringLiteral("msiexec"), Qt::CaseInsensitive))
                        continue;

                    QProcess p;
                    p.setProgram(system32(L"msiexec.exe"));
                    p.setArguments({ QStringLiteral("/x"), code, QStringLiteral("/qn"),
                                     QStringLiteral("/norestart") });
                    p.start();
                    if (!p.waitForStarted(10'000))
                        continue;
                    if (!p.waitForFinished(5 * 60 * 1000)) {
                        qCWarning(catNvidia) << "msiexec /x" << code << name << "timed out";
                        continue;
                    }
                    // 0 removed; 3010 removed, restart pending; 1605 already gone.
                    const int rc = p.exitCode();
                    if (p.exitStatus() == QProcess::NormalExit
                        && (rc == 0 || rc == 3010 || rc == 1605)) {
                        done << code;
                        removed << QStringLiteral("%1 %2").arg(name, code);
                    } else {
                        qCWarning(catNvidia) << "msiexec /x" << code << name << "exit" << rc;
                    }
                }
            }
        }

        // A leftover refcount makes Windows skip re-registering the component.
        void dropSharedDllRefs(const QString& absolutePath) {
            static const wchar_t* kShared =
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs";
            static const wchar_t* kFolders =
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders";
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kShared, 0, KEY_SET_VALUE, &key)
                == ERROR_SUCCESS) {
                RegDeleteValueW(key, absolutePath.toStdWString().c_str());
                RegCloseKey(key);
            }
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kFolders, 0, KEY_SET_VALUE, &key)
                == ERROR_SUCCESS) {
                const QString withSep = absolutePath.endsWith(QLatin1Char('\\'))
                    ? absolutePath : absolutePath + QLatin1Char('\\');
                RegDeleteValueW(key, withSep.toStdWString().c_str());
                RegCloseKey(key);
            }
        }

        int cleanDriverFiles(const QStringList& files, QStringList& removedOut) {
            wchar_t winDir[MAX_PATH] = {};
            wchar_t sys32[MAX_PATH]  = {};
            if (GetWindowsDirectoryW(winDir, MAX_PATH) == 0) return 0;
            if (GetSystemDirectoryW(sys32, MAX_PATH) == 0)   return 0;
            const QString win = QString::fromWCharArray(winDir);
            const QString s32 = QString::fromWCharArray(sys32);

            const QStringList roots = {
                s32,
                s32 + QStringLiteral("\\drivers"),
                win + QStringLiteral("\\SysWOW64"),
                win + QStringLiteral("\\SysWOW64\\drivers"),
                win,
                win + QStringLiteral("\\Prefetch"),
            };

            int count = 0;
            for (const QString& root : roots) {
                if (!QFileInfo(root).isDir())
                    continue;
                for (const QString& rel : files) {
                    QString name = rel;
                    if (name.startsWith(QLatin1Char('\\')))
                        name = name.mid(1);
                    if (name.isEmpty())
                        continue;
                    const QString abs = root + QLatin1Char('\\') + name;
                    const QFileInfo fi(abs);
                    if (!fi.exists())
                        continue;
                    bool gone = false;
                    if (fi.isDir()) {
                        gone = QDir(abs).removeRecursively();
                    } else {
                        const std::wstring w = QDir::toNativeSeparators(abs).toStdWString();
                        const DWORD attrs = GetFileAttributesW(w.c_str());
                        if (attrs != INVALID_FILE_ATTRIBUTES
                            && (attrs & FILE_ATTRIBUTE_READONLY))
                            SetFileAttributesW(w.c_str(),
                                               attrs & ~DWORD(FILE_ATTRIBUTE_READONLY));
                        gone = QFile::remove(abs);
                        if (!gone) {
                            // In use, so it goes at the next restart.
                            MoveFileExW(w.c_str(), nullptr,
                                        MOVEFILE_DELAY_UNTIL_REBOOT);
                        }
                    }
                    if (gone) {
                        dropSharedDllRefs(abs);
                        ++count;
                        if (removedOut.size() < 64)
                            removedOut << abs;
                    }
                }
            }
            return count;
        }

        // A record whose file is gone blocks the next install from writing there.
        int cleanPnpLockdown(const QStringList& driverFiles) {
            static const QString kRoot =
                QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\PnpLockdownFiles");
            int count = 0;
            for (const QString& child : regSubkeyNames(HKEY_LOCAL_MACHINE, kRoot)) {
                QString normalized = child;
                normalized.replace(QLatin1Char('/'), QLatin1Char('\\'));
                wchar_t expanded[MAX_PATH * 2] = {};
                const DWORD n = ExpandEnvironmentStringsW(
                    normalized.toStdWString().c_str(), expanded,
                    DWORD(std::size(expanded)));
                const QString path = (n > 0 && n <= std::size(expanded))
                    ? QString::fromWCharArray(expanded) : normalized;
                if (QFileInfo::exists(path))
                    continue;
                if (!anyContains(path, driverFiles))
                    continue;
                if (deleteTree(HKEY_LOCAL_MACHINE, kRoot + QLatin1Char('\\') + child))
                    ++count;
            }
            return count;
        }

        // Run before the INF is uninstalled, while Source can still be matched.
        void prePnpLockdownForInf(const QString& oemInfName) {
            static const QString kRoot =
                QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\PnpLockdownFiles");
            if (oemInfName.isEmpty())
                return;
            for (const QString& child : regSubkeyNames(HKEY_LOCAL_MACHINE, kRoot)) {
                const QString source = regValueString(
                    HKEY_LOCAL_MACHINE, kRoot + QLatin1Char('\\') + child, L"Source");
                if (source.isEmpty())
                    continue;
                if (!source.contains(oemInfName, Qt::CaseInsensitive))
                    continue;
                deleteTree(HKEY_LOCAL_MACHINE, kRoot + QLatin1Char('\\') + child);
            }
        }

        int cleanControlVideo(const QStringList& serviceNames, QStringList& removedOut) {
            static const QString kVideo =
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Video");
            QStringList match = serviceNames;
            match << QStringLiteral("BasicDisplay");

            int count = 0;
            for (const QString& child : regSubkeyNames(HKEY_LOCAL_MACHINE, kVideo)) {
                const QString svc = regValueString(
                    HKEY_LOCAL_MACHINE,
                    kVideo + QLatin1Char('\\') + child + QStringLiteral("\\Video"),
                    L"Service");
                if (svc.isEmpty())
                    continue;
                bool hit = false;
                for (const QString& s : match) {
                    if (svc.compare(s, Qt::CaseInsensitive) == 0) { hit = true; break; }
                }
                if (!hit)
                    continue;

                if (deleteTree(HKEY_LOCAL_MACHINE, kVideo + QLatin1Char('\\') + child)) {
                    ++count;
                    if (removedOut.size() < 64)
                        removedOut << QStringLiteral("HKLM\\") + kVideo
                                      + QLatin1Char('\\') + child;
                }
                deleteTree(HKEY_LOCAL_MACHINE,
                    QStringLiteral("SYSTEM\\CurrentControlSet\\Hardware Profiles\\UnitedVideo\\CONTROL\\VIDEO\\")
                    + child);
                deleteTree(HKEY_LOCAL_MACHINE,
                    QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\UnitedVideo\\CONTROL\\VIDEO\\")
                    + child);
                deleteTree(HKEY_LOCAL_MACHINE,
                    QStringLiteral("SOFTWARE\\Microsoft\\DirectX\\") + child);
            }
            return count;
        }

        // Stale records can cause a wrong resolution or refresh rate after a reinstall.
        int removeMonitorConfiguration() {
            static const QStringList kKeys = {
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Configuration"),
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Connectivity"),
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\ScaleFactors"),
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\MonitorDataStore"),
            };
            int count = 0;
            for (const QString& key : kKeys)
                for (const QString& child : regSubkeyNames(HKEY_LOCAL_MACHINE, key))
                    if (deleteTree(HKEY_LOCAL_MACHINE, key + QLatin1Char('\\') + child))
                        ++count;
            return count;
        }

        int clearOptimusUpperFilters() {
            static const QString kClassSystem =
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e97d-e325-11ce-bfc1-08002be10318}");
            int count = 0;
            for (const QString& child : regSubkeyNames(HKEY_LOCAL_MACHINE, kClassSystem)) {
                bool numeric = !child.isEmpty();
                for (QChar c : child) { if (!c.isDigit()) { numeric = false; break; } }
                if (!numeric)
                    continue;

                const QString path = kClassSystem + QLatin1Char('\\') + child;
                HKEY key = nullptr;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.toStdWString().c_str(), 0,
                                  KEY_READ | KEY_SET_VALUE, &key) != ERROR_SUCCESS)
                    continue;

                DWORD type = 0, size = 0;
                if (RegQueryValueExW(key, L"UpperFilters", nullptr, &type, nullptr, &size)
                        == ERROR_SUCCESS
                    && type == REG_MULTI_SZ && size > 0) {
                    std::vector<wchar_t> buf(size / sizeof(wchar_t) + 2, L'\0');
                    DWORD sz = size;
                    if (RegQueryValueExW(key, L"UpperFilters", nullptr, &type,
                                         reinterpret_cast<LPBYTE>(buf.data()), &sz)
                        == ERROR_SUCCESS) {
                        // REG_MULTI_SZ: rebuild without the NVIDIA filters.
                        std::vector<wchar_t> rebuilt;
                        bool changed = false;
                        const wchar_t* p = buf.data();
                        while (*p) {
                            const QString entry = QString::fromWCharArray(p);
                            const bool drop =
                                entry.compare(QStringLiteral("nvpciflt"), Qt::CaseInsensitive) == 0
                             || entry.compare(QStringLiteral("nvkflt"), Qt::CaseInsensitive) == 0;
                            if (drop) {
                                changed = true;
                            } else {
                                const std::wstring w = entry.toStdWString();
                                rebuilt.insert(rebuilt.end(), w.begin(), w.end());
                                rebuilt.push_back(L'\0');
                            }
                            while (*p) ++p;
                            ++p;
                        }
                        if (changed) {
                            if (rebuilt.empty()) {
                                RegDeleteValueW(key, L"UpperFilters");
                            } else {
                                rebuilt.push_back(L'\0');   // list terminator
                                RegSetValueExW(key, L"UpperFilters", 0, REG_MULTI_SZ,
                                    reinterpret_cast<const BYTE*>(rebuilt.data()),
                                    DWORD(rebuilt.size() * sizeof(wchar_t)));
                            }
                            ++count;
                        }
                    }
                }
                RegCloseKey(key);
            }
            return count;
        }

        // setup.exe doesn't remove the Store build.
        bool removeStoreControlPanel() {
            const QString out = runPowerShell(QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Get-AppxPackage -AllUsers -Name 'NVIDIACorp.NVIDIAControlPanel' | "
                "Remove-AppxPackage -AllUsers;"
                "Get-AppxProvisionedPackage -Online | "
                "Where-Object { $_.DisplayName -eq 'NVIDIACorp.NVIDIAControlPanel' } | "
                "Remove-AppxProvisionedPackage -Online -ErrorAction SilentlyContinue;"
                "'done'"));
            return out.contains(QStringLiteral("done"));
        }

        int cleanShaderCaches() {
            static const char* const kCaches[] = {
                "%LOCALAPPDATA%\\NVIDIA\\DXCache",
                "%LOCALAPPDATA%\\NVIDIA\\GLCache",
                "%LOCALAPPDATA%\\NVIDIA\\ComputeCache",
                "%LOCALAPPDATA%\\NVIDIA Corporation\\NV_Cache",
                "%LOCALAPPDATA%\\D3DSCache",
                "%ProgramData%\\NVIDIA Corporation\\NV_Cache",
            };
            int count = 0;
            for (const char* raw : kCaches) {
                const QString abs = expandEnv(raw);
                if (abs.isEmpty())
                    continue;
                QDir d(abs);
                if (d.exists() && d.removeRecursively())
                    ++count;
            }
            return count;
        }

        // The local account NVIDIA's old updater created, which shows up at sign-in.
        bool removeUpdatusUser() {
            const QString out = runPowerShell(QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "if (Get-LocalUser -Name 'UpdatusUser') "
                "{ Remove-LocalUser -Name 'UpdatusUser' }; 'done'"));
            return out.contains(QStringLiteral("done"));
        }
    }

    bool companionAppInstalled() {
        for (const char* raw : { "%ProgramFiles%\\NVIDIA Corporation\\NVIDIA app",
                                 "%ProgramFiles(x86)%\\NVIDIA Corporation\\NVIDIA GeForce Experience",
                                 "%ProgramFiles%\\NVIDIA Corporation\\NVIDIA GeForce Experience" }) {
            const QString abs = expandEnv(raw);
            if (!abs.isEmpty() && QFileInfo(abs).isDir())
                return true;
        }
        // The folder can be gone while the app is still registered.
        static const QString kUninstall =
            QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
        for (const QString& sub : regSubkeyNames(HKEY_LOCAL_MACHINE, kUninstall)) {
            const QString name = regValueString(
                HKEY_LOCAL_MACHINE, kUninstall + QLatin1Char('\\') + sub, L"DisplayName");
            if (name.compare(QStringLiteral("NVIDIA app"), Qt::CaseInsensitive) == 0
                || name.startsWith(QStringLiteral("NVIDIA GeForce Experience"), Qt::CaseInsensitive))
                return true;
        }
        return false;
    }

    CleanResult runCleanRemoval(const CleanOptions& options,
                                const CleanProgressFn& progress) {
        namespace T = vth::win::nvidia::tables;

        CleanResult r;
        if (!vth::win::isElevated()) {
            r.error = QStringLiteral(
                "Removing the display driver needs administrator privileges. "
                "Re-launch Vethida elevated and try again.");
            return r;
        }

        const auto step = [&progress](const char* stage, int pct) {
            if (progress)
                progress(QString::fromLatin1(stage), pct);
        };

        if (options.createRestorePoint) {
            step("Creating a restore point", 1);
            const auto outcome = vth::win::sysrestore::create(
                QStringLiteral("Before Vethida NVIDIA driver reinstall"), true);
            r.restorePointCreated =
                (outcome.result == vth::win::sysrestore::CreateResult::Ok);
            if (!r.restorePointCreated)
                qCWarning(catNvidia) << "no restore point before driver removal:"
                                     << vth::win::sysrestore::createResultLabel(outcome.result);
        }

        QStringList svcNames = T::services();
        if (options.removeGfe)       svcNames += T::servicesGfe();
        if (options.removeBroadcast) svcNames += T::servicesBroadcast();
        svcNames.removeDuplicates();

        // First, because a running NvContainer recreates registry state and holds
        // driver files open.
        step("Stopping NVIDIA services", 3);
        for (const QString& name : svcNames) {
            const std::wstring w = name.toStdWString();
            const int timeout = name.startsWith(QStringLiteral("NVDisplay")) ? 30'000
                              : name.startsWith(QStringLiteral("NvContainer")) ? 20'000
                              : 10'000;
            if (stopServiceAndWait(w.c_str(), timeout) == 1)
                r.servicesStopped << name;
        }

        // Before the driver store, because a ProgID needs its CLSID still registered.
        step("Removing COM registrations", 12);
        {
            QStringList roots = T::classRoots();
            if (options.removeGfe) roots += T::classRootsGfe();
            roots.removeDuplicates();
            cleanClassRoots(roots, r.registryKeysRemoved);

            QStringList clsids = options.removeGfe ? T::clsidLeftoversGfe()
                                                   : T::clsidLeftovers();
            if (options.removeBroadcast) clsids += T::clsidLeftoversBroadcast();
            clsids.removeDuplicates();
            cleanClsidLeftovers(clsids, r.registryKeysRemoved);

            QStringList ifaces = T::interfaces();
            if (options.removeGfe) ifaces += T::interfacesGfe();
            cleanInterfaces(ifaces, r.registryKeysRemoved);
        }

        step("Removing PhysX", 24);
        if (options.removePhysX)
            uninstallMsiProducts(T::installerPackages(), r.registryKeysRemoved);

        // Until these are gone, Windows puts the old driver back on a device rescan.
        step("Removing driver packages", 32);
        QStringList removedInfNames;
        for (const OemInf& inf : enumerateOemInfs()) {
            if (!inf.provider.contains(QString::fromWCharArray(kProvider),
                                       Qt::CaseInsensitive))
                continue;
            if (!driverStoreClasses().contains(inf.setupClass.toLower()))
                continue;
            const QString cat = inf.catalog.toLower();
            if (!options.removeBroadcast && cat.contains(QStringLiteral("nvrtxvad")))
                continue;
            if (!options.removeGfe && (cat.contains(QStringLiteral("nvvad"))
                                    || cat.contains(QStringLiteral("nvswcfilter"))))
                continue;

            prePnpLockdownForInf(inf.publishedName);

            const QString label = QStringLiteral("%1 (%2, %3)")
                .arg(inf.publishedName, inf.setupClass, inf.provider);
            if (uninstallOemInf(inf, r.rebootRequired)) {
                r.infsRemoved << label;
                removedInfNames << inf.publishedName;
            } else {
                r.infsFailed << label;
            }
        }
        if (!r.infsFailed.isEmpty())
            qCWarning(catNvidia) << "driver packages left behind:" << r.infsFailed;

        step("Removing NVIDIA services", 40);
        for (const QString& name : svcNames) {
            const std::wstring w = name.toStdWString();
            const int stop = stopServiceAndWait(w.c_str(), 10'000);
            if (stop == 0)
                continue;
            if (stop < 0) {
                qCWarning(catNvidia) << "service still running, left for the install:" << name;
                continue;
            }
            switch (deleteService(w.c_str())) {
                case 1: r.servicesDeleted << name; break;
                case 2: r.servicesDeleted << name; r.rebootRequired = true; break;
                default: break;
            }
        }

        step("Clearing hybrid-graphics filters", 44);
        if (clearOptimusUpperFilters() > 0)
            r.rebootRequired = true;

        step("Clearing registry entries", 56);
        {
            // The NVIDIA Corporation trees also hold the NVIDIA App's settings.
            QStringList trees = {
                QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\NVIDIA Display Driver"),
            };
            if (options.removeGfe) {
                trees << QStringLiteral("SOFTWARE\\NVIDIA Corporation")
                      << QStringLiteral("SOFTWARE\\WOW6432Node\\NVIDIA Corporation");
            }
            for (const QString& key : trees)
                if (deleteTree(HKEY_LOCAL_MACHINE, key))
                    r.registryKeysRemoved << QStringLiteral("HKLM\\") + key;

            for (const QString& sub : enumerateNvidiaClassSubkeys()) {
                const QString path =
                    QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Class\\") + sub;
                if (deleteTree(HKEY_LOCAL_MACHINE, path))
                    r.registryKeysRemoved << QStringLiteral("HKLM\\") + path;
            }

            cleanControlVideo(svcNames, r.registryKeysRemoved);
        }

        step("Clearing monitor configuration", 64);
        removeMonitorConfiguration();

        step("Deleting driver files", 70);
        {
            QStringList files = T::driverFiles();
            if (options.removeGfe)       files += T::driverFilesGfe();
            if (options.removeBroadcast) files += T::driverFilesBroadcast();
            files.removeDuplicates();
            const int n = cleanDriverFiles(files, r.pathsRemoved);
            qCInfo(catNvidia) << "driver files removed:" << n;

            cleanPnpLockdown(files);
        }

        step("Removing NVIDIA folders", 82);
        {
            QStringList keep;
            if (!options.removeGfe) {
                keep << QStringLiteral("NVIDIA app") << QStringLiteral("NvApp")
                     << QStringLiteral("NVIDIA GeForce Experience")
                     << QStringLiteral("GeForce Experience")
                     << QStringLiteral("GeForceExperienceSelfUpdate")
                     << QStringLiteral("NvBackend") << QStringLiteral("NvNode")
                     << QStringLiteral("ShadowPlay") << QStringLiteral("FrameViewSDK")
                     << QStringLiteral("NvContainer");
            }
            if (!options.removeBroadcast)
                keep << QStringLiteral("NVIDIA Broadcast");
            if (!options.removePhysX)
                keep << QStringLiteral("PhysX");

            auto kept = [&keep](const QString& child) {
                for (const QString& k : keep)
                    if (child.compare(k, Qt::CaseInsensitive) == 0)
                        return true;
                return false;
            };

            // Roots whose children may belong to a kept app.
            static const char* const kSharedRoots[] = {
                "%ProgramFiles%\\NVIDIA Corporation",
                "%ProgramFiles(x86)%\\NVIDIA Corporation",
                "%ProgramData%\\NVIDIA Corporation",
                "%LOCALAPPDATA%\\NVIDIA Corporation",
            };
            static const char* const kDriverRoots[] = {
                "%ProgramData%\\NVIDIA",
                "%LOCALAPPDATA%\\NVIDIA",
                "%APPDATA%\\NVIDIA",
            };

            auto removePath = [&r](const QString& abs) {
                const QFileInfo fi(abs);
                const bool ok = fi.isDir() ? QDir(abs).removeRecursively()
                                           : QFile::remove(abs);
                if (ok)
                    r.pathsRemoved << abs;
                else
                    r.rebootRequired = true;
            };

            for (const char* raw : kSharedRoots) {
                const QString abs = expandEnv(raw);
                if (abs.isEmpty() || !QFileInfo(abs).isDir())
                    continue;
                if (keep.isEmpty()) {
                    removePath(abs);
                    continue;
                }
                const QDir root(abs);
                for (const QString& child : root.entryList(QDir::AllEntries | QDir::Hidden
                                                           | QDir::System | QDir::NoDotAndDotDot)) {
                    if (!kept(child))
                        removePath(root.absoluteFilePath(child));
                }
            }
            for (const char* raw : kDriverRoots) {
                const QString abs = expandEnv(raw);
                if (!abs.isEmpty() && QFileInfo(abs).isDir())
                    removePath(abs);
            }
        }

        step("Clearing shader caches", 88);
        cleanShaderCaches();

        step("Removing the Store control panel", 91);
        if (options.removeGfe)
            removeStoreControlPanel();

        step("Removing leftover accounts", 93);
        if (options.removeLegacy)
            removeUpdatusUser();

        step("Removing scheduled tasks", 96);
        QStringList tasks = T::scheduledTasks();
        if (options.removeGfe)
            tasks += T::scheduledTasksGfe();
        for (const QString& task : tasks) {
            if (deleteScheduledTask(task))
                r.tasksRemoved << task;
        }

        step("Driver removed", 100);

        // Only the driver packages count, because the install depends on them.
        r.ok = r.infsFailed.isEmpty();
        if (!r.ok) {
            r.error = QStringLiteral(
                "%1 driver package(s) could not be removed. The reinstall can "
                "still continue, but a reboot and a second run may be needed to "
                "clear them.").arg(r.infsFailed.size());
        }
        return r;
    }

    const QVector<NvidiaRegTweak>& registryTweaksCatalog() {
        using T = TweakTarget;
        static const QVector<NvidiaRegTweak> kTweaks = {
            {.id = QStringLiteral("DisableDynamicPstate"),
             .name = QStringLiteral("Force P0 (disable dynamic P-state)"),
             .description = QStringLiteral("Locks the GPU to its highest performance state at all "
                            "times. Higher idle power draw; eliminates the clock-state "
                            "transition stutter some titles experience."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("DisableDynamicPstate"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("RMDisableGPUIdlePowerSaving"),
             .name = QStringLiteral("Disable GPU idle power saving"),
             .description = QStringLiteral("Prevents the driver from gating power on idle GPU blocks. "
                            "Companion to Force-P0 above."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("RMDisableGPUIdlePowerSaving"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("RMGpuPowerPolicyFlags"),
             .name = QStringLiteral("GPU power policy flags"),
             .description = QStringLiteral("Sets the driver's power-policy flag set to favor "
                            "performance over efficiency."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("RMGpuPowerPolicyFlags"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("RMEnablePreemption"),
             .name = QStringLiteral("Disable GPU preemption"),
             .description = QStringLiteral("Turns off cooperative GPU preemption. Reduces input lag "
                            "but risks compositor stuttering on multi-GPU-workload "
                            "scenarios."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("RMEnablePreemption"), .isDword = true, .dwordValue = 0},

            {.id = QStringLiteral("PowerMizerEnable"),
             .name = QStringLiteral("PowerMizer override on"),
             .description = QStringLiteral("Enables driver-level PowerMizer overrides."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("PowerMizerEnable"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("PowerMizerLevel"),
             .name = QStringLiteral("PowerMizer max performance"),
             .description = QStringLiteral("Forces the GPU into maximum-performance PowerMizer level."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("PowerMizerLevel"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("PowerMizerLevelAC"),
             .name = QStringLiteral("PowerMizer max performance (AC)"),
             .description = QStringLiteral("Maximum-performance PowerMizer level on AC power."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("PowerMizerLevelAC"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("PerfLevelSrc"),
             .name = QStringLiteral("Force max-clock source"),
             .description = QStringLiteral("Forces the perf-level source for every GPU domain to "
                            "the maximum value (0x2222)."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("PerfLevelSrc"), .isDword = true, .dwordValue = 0x2222},

            {.id = QStringLiteral("EnableMSI"),
             .name = QStringLiteral("Enable Message-Signaled Interrupts"),
             .description = QStringLiteral("Forces the GPU onto MSI / MSI-X interrupts. Lower "
                            "interrupt-handling overhead than legacy line-based IRQs."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("EnableMSI"), .isDword = true, .dwordValue = 1,
             .defaultEnabled = true},

            {.id = QStringLiteral("DisableAnsel"),
             .name = QStringLiteral("Disable NVIDIA Ansel"),
             .description = QStringLiteral("Disables Ansel screenshot integration in the driver."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("DisableAnsel"), .isDword = true, .dwordValue = 1,
             .defaultEnabled = true},

            {.id = QStringLiteral("RMHdcpKeyglobZero"),
             .name = QStringLiteral("Disable HDCP"),
             .description = QStringLiteral("Zeroes out the HDCP keyglob. Useful for capture pipelines "
                            "complaining about HDCP. Do not enable on a system that "
                            "watches DRMed media."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("RMHdcpKeyglobZero"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("EnableRID61684"),
             .name = QStringLiteral("Latency RID 61684"),
             .description = QStringLiteral("Undocumented NVIDIA performance tweak. Benchmark "
                            "before relying on it."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("EnableRID61684"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("EnableRID73779"),
             .name = QStringLiteral("Latency RID 73779"),
             .description = QStringLiteral("Companion latency tweak."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("EnableRID73779"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("EnableRID73780"),
             .name = QStringLiteral("Latency RID 73780"),
             .description = QStringLiteral("Companion latency tweak."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("EnableRID73780"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("EnableRID74361"),
             .name = QStringLiteral("Latency RID 74361"),
             .description = QStringLiteral("Companion latency tweak."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("EnableRID74361"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("PreferSystemMemoryContiguous"),
             .name = QStringLiteral("Prefer contiguous system memory"),
             .description = QStringLiteral("Hints the driver to allocate contiguous system memory "
                            "for command buffers."),
             .target = T::NvidiaClassSubkey,
             .valueName = QStringLiteral("PreferSystemMemoryContiguous"), .isDword = true, .dwordValue = 1,
             .defaultEnabled = true},

            // HAGS: HwSchMode 2 = enabled, 1 = disabled.
            {.id = QStringLiteral("HwSchModeOn"),
             .name = QStringLiteral("Hardware-Accelerated GPU Scheduling (HAGS)"),
             .description = QStringLiteral("Sets HwSchMode=2 so the GPU manages its own command "
                            "queue instead of the WDDM scheduler. Reduces CPU overhead "
                            "and can cut input latency on supported NVIDIA GTX 1000+ "
                            "cards. Requires reboot. Some games and capture tools "
                            "regress with HAGS on; revert if you see issues."),
             .target = T::GraphicsDrivers,
             .valueName = QStringLiteral("HwSchMode"), .isDword = true, .dwordValue = 2},

            {.id = QStringLiteral("TdrDisable"),
             .name = QStringLiteral("Disable GPU TDR (timeout detection / recovery)"),
             .description = QStringLiteral("Sets TdrLevel/TdrDelay/TdrDdiDelay/TdrDebugMode/TdrLimit"
                            "Count/TdrLimitTime/TdrTestMode all to 0. Prevents Windows "
                            "from resetting a hung GPU - the system will freeze instead. "
                            "Recommended only on systems where TDR is causing false "
                            "positives during long compute kernels or benchmarks."),
             .target = T::GraphicsDrivers,
             .valueName = QStringLiteral("TdrLevel"), .isDword = true, .dwordValue = 0,
             .extraValues = {
                 {.subpath = {}, .name = QStringLiteral("TdrDelay"),       .isDword = true, .dwordValue = 0},
                 {.subpath = {}, .name = QStringLiteral("TdrDdiDelay"),    .isDword = true, .dwordValue = 0},
                 {.subpath = {}, .name = QStringLiteral("TdrDebugMode"),   .isDword = true, .dwordValue = 0},
                 {.subpath = {}, .name = QStringLiteral("TdrLimitCount"),  .isDword = true, .dwordValue = 0},
                 {.subpath = {}, .name = QStringLiteral("TdrLimitTime"),   .isDword = true, .dwordValue = 0},
                 {.subpath = {}, .name = QStringLiteral("TdrTestMode"),    .isDword = true, .dwordValue = 0},
             }},

            {.id = QStringLiteral("DisableNvcDispCorePlugin"),
             .name = QStringLiteral("Disable NVIDIA Display Container plugin"),
             .description = QStringLiteral("Sets DisableLoad=1 and LogLevel=0 under the NVIDIA "
                            "Display Container service plugin so it stops loading at "
                            "startup. Cuts background CPU, but the display pages of the "
                            "NVIDIA App and the NVIDIA Control Panel read their settings "
                            "through this plugin and stop working, and multi-monitor "
                            "management becomes unavailable. Leave off if you use either."),
             .target = T::NvDisplayContainer,
             .valueName = QStringLiteral("DisableLoad"), .isDword = true, .dwordValue = 1,
             .extraValues = {
                 {.subpath = {}, .name = QStringLiteral("LogLevel"), .isDword = true, .dwordValue = 0},
             }},

            {.id = QStringLiteral("ExcludeWUDriversInQualityUpdate"),
             .name = QStringLiteral("Block Windows Update driver push"),
             .description = QStringLiteral("Sets ExcludeWUDriversInQualityUpdate=1 under the Windows "
                            "Update policy hive so quality updates skip driver downloads. "
                            "Does not block manual driver installs - only the silent "
                            "Windows Update channel."),
             .target = T::WindowsUpdatePolicy,
             .valueName = QStringLiteral("ExcludeWUDriversInQualityUpdate"), .isDword = true, .dwordValue = 1},

            {.id = QStringLiteral("HdmiAudioMsiMode"),
             .name = QStringLiteral("Enable MSI on NVIDIA HDMI audio"),
             .description = QStringLiteral("Switches the NVIDIA HDMI / DisplayPort audio device to "
                            "MSI interrupts (MSISupported=1) and lowers its device "
                            "priority (DevicePriority=0). Reduces interrupt-line "
                            "contention with the GPU. If audio drops out after "
                            "applying, switch it off from Safe Mode."),
             .target = T::HdmiAudioClassSubkey,
             .valueName = {},
             .extraValues = {
                 {.subpath = QStringLiteral("Device Parameters\\Interrupt Management\\MessageSignaledInterruptProperties"),
                  .name = QStringLiteral("MSISupported"), .isDword = true, .dwordValue = 1},
                 {.subpath = QStringLiteral("Device Parameters\\Interrupt Management\\Affinity Policy"),
                  .name = QStringLiteral("DevicePriority"), .isDword = true, .dwordValue = 0},
             }},

            // Idle timers to never and target power state to D0 (NVIDIA KB 5441).
            {.id = QStringLiteral("HdmiAudioSleepDisable"),
             .name = QStringLiteral("Disable NVIDIA HDMI audio sleep (PortCls)"),
             .description = QStringLiteral("Stops the NVIDIA HDMI/DisplayPort audio codec from "
                            "idling into D3, which causes the first ~0.5 s of audio "
                            "to cut off after silence on many TVs and AVRs. Per "
                            "NVIDIA KB 5441. Reverts on every NVIDIA driver update - "
                            "re-apply after a GPU driver upgrade."),
             .target = T::HdmiAudioClassSubkey,
             .valueName = {},
             .extraValues = {
                 {.subpath = QStringLiteral("PowerSettings"),
                  .name = QStringLiteral("ConservationIdleTime"),
                  .isDword = false, .isBinary = true,
                  .binaryValue = QByteArray("\xFF\xFF\xFF\xFF", 4)},
                 {.subpath = QStringLiteral("PowerSettings"),
                  .name = QStringLiteral("PerformanceIdleTime"),
                  .isDword = false, .isBinary = true,
                  .binaryValue = QByteArray("\xFF\xFF\xFF\xFF", 4)},
                 {.subpath = QStringLiteral("PowerSettings"),
                  .name = QStringLiteral("IdlePowerState"),
                  .isDword = false, .isBinary = true,
                  .binaryValue = QByteArray("\x00\x00\x00\x00", 4)},
             }},
        };
        return kTweaks;
    }

    namespace
    {
        // Returns each match as "{class-guid}\NNNN".
        QStringList enumerateClassSubkeysForNvidia(const wchar_t* classGuid) {
            QStringList out;
            HKEY classKey = nullptr;
            const std::wstring fullPath = std::wstring(L"SYSTEM\\CurrentControlSet\\Control\\Class\\")
                                        + classGuid;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullPath.c_str(),
                              0, KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                              &classKey) != ERROR_SUCCESS) {
                return out;
            }

            for (DWORD i = 0;; ++i) {
                wchar_t name[64] = {};
                DWORD nameLen = 64;
                const LSTATUS r = RegEnumKeyExW(classKey, i, name, &nameLen,
                                                nullptr, nullptr, nullptr, nullptr);
                if (r == ERROR_NO_MORE_ITEMS) break;
                if (r != ERROR_SUCCESS) continue;

                const QString sub = QString::fromWCharArray(name, static_cast<int>(nameLen));
                bool numeric = !sub.isEmpty();
                for (QChar c : sub) { if (!c.isDigit()) { numeric = false; break; } }
                if (!numeric) continue;

                HKEY adapter = nullptr;
                if (RegOpenKeyExW(classKey, name, 0, KEY_READ, &adapter) != ERROR_SUCCESS) continue;

                wchar_t devBuf[256] = {};
                DWORD devSize = sizeof(devBuf);
                DWORD type = 0;
                bool isNvidia = false;
                if (RegQueryValueExW(adapter, L"MatchingDeviceId", nullptr, &type,
                                     reinterpret_cast<LPBYTE>(devBuf), &devSize) == ERROR_SUCCESS) {
                    const QString did = QString::fromWCharArray(devBuf).toUpper();
                    if (did.contains(QStringLiteral("VEN_10DE"))) isNvidia = true;
                }
                RegCloseKey(adapter);
                if (isNvidia) {
                    out << QString::fromWCharArray(classGuid) + QStringLiteral("\\") + sub;
                }
            }
            RegCloseKey(classKey);
            return out;
        }

        QStringList enumerateNvidiaClassSubkeys() {
            return enumerateClassSubkeysForNvidia(L"{4d36e968-e325-11ce-bfc1-08002be10318}");
        }

        QStringList enumerateHdmiAudioClassSubkeys() {
            return enumerateClassSubkeysForNvidia(L"{4d36e96c-e325-11ce-bfc1-08002be10318}");
        }

        // HKLM-relative paths to write under. Class targets have one per adapter.
        QStringList resolveTargetSites(TweakTarget t, ApplyResult& r) {
            const QString classRoot = QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Class\\");
            QStringList sites;
            switch (t) {
                case TweakTarget::NvidiaClassSubkey: {
                    const QStringList classKeys = enumerateNvidiaClassSubkeys();
                    if (classKeys.isEmpty() && r.error.isEmpty()) {
                        r.error = QStringLiteral(
                            "No NVIDIA GPU class subkey found under "
                            "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-...}. "
                            "Confirm an NVIDIA driver is installed and try again.");
                    }
                    for (const auto& sub : classKeys)
                        sites << classRoot + sub;
                    return sites;
                }
                case TweakTarget::HdmiAudioClassSubkey:
                    for (const auto& sub : enumerateHdmiAudioClassSubkeys())
                        sites << classRoot + sub;
                    return sites;
                case TweakTarget::GraphicsDrivers:
                    sites << QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers");
                    return sites;
                case TweakTarget::NvDisplayContainer:
                    sites << QStringLiteral("SYSTEM\\CurrentControlSet\\Services\\"
                                            "NVDisplay.ContainerLocalSystem\\LocalSystem\\NvcDispCorePlugin");
                    return sites;
                case TweakTarget::WindowsUpdatePolicy:
                    sites << QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate");
                    return sites;
            }
            return sites;
        }

        // The tracker keeps a binary value as base64 in prevString.
        void recordPreviousValue(const QString& fullPath, const QString& valueName, TweakSnapshot& ts) {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullPath.toStdWString().c_str(),
                              0, KEY_READ, &key) != ERROR_SUCCESS) {
                return;
            }
            const std::wstring name = valueName.toStdWString();
            DWORD type = 0, size = 0;
            if (RegQueryValueExW(key, name.c_str(), nullptr, &type, nullptr, &size) == ERROR_SUCCESS
                && size > 0) {
                ts.existedBefore = true;
                if (type == REG_DWORD && size == sizeof(DWORD)) {
                    DWORD v = 0; DWORD sz = sizeof(v);
                    RegQueryValueExW(key, name.c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(&v), &sz);
                    ts.prevDword = v;
                } else if (type == REG_SZ || type == REG_EXPAND_SZ) {
                    std::vector<wchar_t> buf(size / sizeof(wchar_t) + 2, L'\0');
                    DWORD sz = size;
                    RegQueryValueExW(key, name.c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf.data()), &sz);
                    ts.prevString = QString::fromWCharArray(buf.data());
                    ts.isDword = false;
                } else if (type == REG_BINARY) {
                    QByteArray buf(static_cast<int>(size), '\0');
                    DWORD sz = size;
                    RegQueryValueExW(key, name.c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf.data()), &sz);
                    ts.prevString = QStringLiteral("[base64] ") + QString::fromLatin1(buf.toBase64());
                    ts.isDword = false;
                }
            }
            RegCloseKey(key);
        }

        // isBinary wins over isDword, and neither means REG_SZ.
        LSTATUS writeValue(const QString& fullPath, const QString& valueName,
                           bool isDword, quint32 dword, const QString& strv,
                           bool isBinary, const QByteArray& binv) {
            HKEY key = nullptr; DWORD disp = 0;
            const LSTATUS open = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                                 fullPath.toStdWString().c_str(),
                                                 0, nullptr, REG_OPTION_NON_VOLATILE,
                                                 KEY_READ | KEY_SET_VALUE, nullptr,
                                                 &key, &disp);
            if (open != ERROR_SUCCESS) return open;

            LSTATUS w;
            if (isBinary) {
                w = RegSetValueExW(key, valueName.toStdWString().c_str(), 0, REG_BINARY,
                                   reinterpret_cast<const BYTE*>(binv.constData()),
                                   static_cast<DWORD>(binv.size()));
            } else if (isDword) {
                const DWORD v = dword;
                w = RegSetValueExW(key, valueName.toStdWString().c_str(), 0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&v), sizeof(v));
            } else {
                const std::wstring sw = strv.toStdWString();
                w = RegSetValueExW(key, valueName.toStdWString().c_str(), 0, REG_SZ,
                                   reinterpret_cast<const BYTE*>(sw.c_str()),
                                   static_cast<DWORD>((sw.size() + 1) * sizeof(wchar_t)));
            }
            RegCloseKey(key);
            return w;
        }

        bool applyOneWrite(const QString& site,
                           const QString& subpath,
                           const QString& valueName,
                           bool isDword,
                           quint32 dwordValue,
                           const QString& stringValue,
                           bool isBinary,
                           const QByteArray& binaryValue,
                           const QString& tweakId,
                           ApplyResult& r) {
            const QString fullPath = subpath.isEmpty()
                ? site
                : site + QStringLiteral("\\") + subpath;

            TweakSnapshot ts;
            ts.domain = QStringLiteral("nvidia.regtweaks");
            ts.kind   = SnapshotKind::Registry;
            ts.label  = QStringLiteral("%1\\%2 = %3").arg(fullPath, valueName, tweakId);
            ts.hive   = QStringLiteral("HKLM");
            ts.subkey = fullPath;
            ts.name   = valueName;
            recordPreviousValue(fullPath, valueName, ts);

            const LSTATUS rc = writeValue(fullPath, valueName,
                                          isDword, dwordValue, stringValue,
                                          isBinary, binaryValue);
            if (rc != ERROR_SUCCESS) {
                r.failure << QStringLiteral("%1 @ %2\\%3 (rc=%4)")
                                 .arg(tweakId, fullPath, valueName).arg(rc);
                return false;
            }

            recordTweakSnapshot(std::move(ts));
            return true;
        }
    }

    ApplyResult applyRegistryTweaks(const QSet<QString>& tweakIds) {
        ApplyResult r;
        if (!vth::win::isElevated()) {
            r.error = QStringLiteral(
                "NVIDIA registry tweaks require administrator privileges. "
                "Re-launch Vethida elevated and try again.");
            return r;
        }

        const vth::TweakBatch batch(vth::newTweakBatchId(QStringLiteral("nvidia.regtweaks")));

        for (const auto& t : registryTweaksCatalog()) {
            if (!tweakIds.contains(t.id)) continue;

            const auto sites = resolveTargetSites(t.target, r);
            if (sites.isEmpty()) {
                r.failure << QStringLiteral("%1 (no target sites resolved)").arg(t.id);
                continue;
            }

            bool anyOk = false;
            for (const auto& site : sites) {
                bool siteOk = true;

                if (!t.valueName.isEmpty()) {
                    if (!applyOneWrite(site, /*subpath=*/{}, t.valueName,
                                       t.isDword, t.dwordValue, t.stringValue,
                                       t.isBinary, t.binaryValue,
                                       t.id, r)) {
                        siteOk = false;
                    }
                }
                for (const auto& ev : t.extraValues) {
                    if (!applyOneWrite(site, ev.subpath, ev.name,
                                       ev.isDword, ev.dwordValue, ev.stringValue,
                                       ev.isBinary, ev.binaryValue,
                                       t.id, r)) {
                        siteOk = false;
                    }
                }
                if (siteOk) anyOk = true;
            }

            if (anyOk) r.success << t.id;
        }

        r.ok = !r.success.isEmpty();
        return r;
    }

    namespace
    {
        // UINT32_MAX on failure.
        quint32 queryServiceStartType(SC_HANDLE scm, const wchar_t* serviceName) {
            SC_HANDLE svc = OpenServiceW(scm, serviceName, SERVICE_QUERY_CONFIG);
            if (!svc) return UINT32_MAX;
            DWORD bytesNeeded = 0;
            QueryServiceConfigW(svc, nullptr, 0, &bytesNeeded);
            if (bytesNeeded == 0) { CloseServiceHandle(svc); return UINT32_MAX; }
            std::vector<BYTE> buf(bytesNeeded);
            auto* cfg = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buf.data());
            DWORD got = 0;
            quint32 startType = UINT32_MAX;
            if (QueryServiceConfigW(svc, cfg, bytesNeeded, &got)) {
                startType = static_cast<quint32>(cfg->dwStartType);
            }
            CloseServiceHandle(svc);
            return startType;
        }

        LSTATUS stopAndDisableService(const wchar_t* serviceName) {
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (!scm) return static_cast<LSTATUS>(GetLastError());
            SC_HANDLE svc = OpenServiceW(scm, serviceName,
                SERVICE_STOP | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS);
            if (!svc) {
                const DWORD e = GetLastError();
                CloseServiceHandle(scm);
                return static_cast<LSTATUS>(e);
            }
            SERVICE_STATUS ss{};
            ControlService(svc, SERVICE_CONTROL_STOP, &ss);
            const BOOL ok = ChangeServiceConfigW(
                svc, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
            const DWORD err = ok ? ERROR_SUCCESS : GetLastError();
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return static_cast<LSTATUS>(err);
        }

        // Returns 1 if disabled, 0 if there's no such task, -1 if it failed. Queried
        // first because schtasks' error text depends on the display language.
        int disableScheduledTask(const QString& taskPath) {
            if (runSchtasks({ QStringLiteral("/Query"), QStringLiteral("/TN"), taskPath }) != 0)
                return 0;
            return runSchtasks({ QStringLiteral("/Change"), QStringLiteral("/TN"), taskPath,
                                 QStringLiteral("/Disable") }) == 0 ? 1 : -1;
        }
    }

    void applyDrvtlmRuntime() {
        const vth::TweakBatch batch(vth::newTweakBatchId(QStringLiteral("nvidia.runtime")));

        quint32 prevStart = UINT32_MAX;
        if (SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT)) {
            prevStart = queryServiceStartType(scm, L"NvTelemetryContainer");
            CloseServiceHandle(scm);
        }
        const LSTATUS rc = stopAndDisableService(L"NvTelemetryContainer");
        if (rc == ERROR_SUCCESS) {
            TweakSnapshot ts;
            ts.domain        = QStringLiteral("nvidia.runtime");
            ts.kind          = SnapshotKind::Service;
            ts.label         = QStringLiteral("NvTelemetryContainer = Disabled");
            ts.serviceName   = QStringLiteral("NvTelemetryContainer");
            ts.prevStartType = prevStart == UINT32_MAX ? quint32{2} : prevStart;
            ts.serviceWasRunning = true;
            recordTweakSnapshot(std::move(ts));
        } else if (rc != static_cast<LSTATUS>(ERROR_SERVICE_DOES_NOT_EXIST)) {
            qCWarning(catNvidia) << "could not disable NvTelemetryContainer, rc" << rc;
        }

        static const QStringList kTaskPaths = {
            QStringLiteral("\\NvTmMon_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"),
            QStringLiteral("\\NvTmRep_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"),
            QStringLiteral("\\NvTmRepOnLogon_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"),
        };
        for (const auto& path : kTaskPaths) {
            const int disabled = disableScheduledTask(path);
            if (disabled == 1) {
                TweakSnapshot ts;
                ts.domain         = QStringLiteral("nvidia.runtime");
                ts.kind           = SnapshotKind::Task;
                ts.label          = QStringLiteral("%1 = Disabled").arg(path);
                ts.taskPath       = path;
                ts.taskWasEnabled = true;
                recordTweakSnapshot(std::move(ts));
            } else if (disabled < 0) {
                qCWarning(catNvidia) << "could not disable" << path;
            }
        }
    }
}

namespace vth::win::nvidia
{
    QString downloadResultLabel(DownloadResult r) {
        switch (r) {
            case DownloadResult::Ok:           return QStringLiteral("OK");
            case DownloadResult::InvalidUrl:   return QStringLiteral("Invalid URL");
            case DownloadResult::NetworkError: return QStringLiteral("Network error");
            case DownloadResult::WriteError:   return QStringLiteral("File write error");
            case DownloadResult::Canceled:     return QStringLiteral("Canceled");
        }
        return {};
    }
    QString extractResultLabel(ExtractResult r) {
        switch (r) {
            case ExtractResult::Ok:     return QStringLiteral("Extracted");
            case ExtractResult::Failed: return QStringLiteral("Extraction failed");
        }
        return {};
    }
}

namespace vth::win::nvidia
{
namespace
{
        QString snapshotPath() {
            return paths::userData() + QStringLiteral("/nvidia-tweaks-state.json");
        }
        struct TweakTargetSite {
            QString rootPath;
            QString snapshotKey;
        };
        RegSnapshot snapshotValue(const QString& fullPath,
                                  const QString& valueName,
                                  TweakTarget target,
                                  const QString& snapshotKey) {
            RegSnapshot s;
            s.target = target;
            s.classSubkey = snapshotKey;
            s.valueName = valueName;
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullPath.toStdWString().c_str(),
                              0, KEY_READ, &key) != ERROR_SUCCESS) {
                return s;
            }
            DWORD type = 0, size = 0;
            const LSTATUS q = RegQueryValueExW(key, valueName.toStdWString().c_str(),
                                               nullptr, &type, nullptr, &size);
            if (q == ERROR_SUCCESS && size > 0) {
                s.existedBefore = true;
                if (type == REG_DWORD && size == sizeof(DWORD)) {
                    DWORD v = 0; DWORD sz = sizeof(v);
                    RegQueryValueExW(key, valueName.toStdWString().c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(&v), &sz);
                    s.prevDword = v;
                    s.isDword = true;
                } else if (type == REG_SZ || type == REG_EXPAND_SZ) {
                    std::vector<wchar_t> buf(size / sizeof(wchar_t) + 2, L'\0');
                    DWORD sz = size;
                    RegQueryValueExW(key, valueName.toStdWString().c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf.data()), &sz);
                    s.prevString = QString::fromWCharArray(buf.data());
                    s.isDword = false;
                } else if (type == REG_BINARY) {
                    QByteArray buf(static_cast<int>(size), '\0');
                    DWORD sz = size;
                    RegQueryValueExW(key, valueName.toStdWString().c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf.data()), &sz);
                    s.prevBinary = buf;
                    s.isBinary = true;
                    s.isDword = false;
                }
            }
            RegCloseKey(key);
            return s;
        }
}
}

namespace vth::win::nvidia
{
    QVector<RegSnapshot> loadPersistedSnapshots() {
        QVector<RegSnapshot> out;
        QFile f(snapshotPath());
        if (!f.exists() || !f.open(QIODevice::ReadOnly)) return out;
        const QByteArray raw = f.readAll();
        QJsonParseError err;
        const auto doc = QJsonDocument::fromJson(raw, &err);
        if (err.error != QJsonParseError::NoError || !doc.isArray()) {
            if (!raw.isEmpty())
                qCWarning(catNvidia) << "nvidia-tweaks-state.json unreadable:" << err.errorString();
            return out;
        }
        const QJsonArray arr = doc.array();
        out.reserve(arr.size());
        for (const auto& v : arr) {
            const auto o = v.toObject();
            RegSnapshot s;
            const auto targetVal = o.value(QStringLiteral("target"));
            s.target = targetVal.isUndefined()
                ? TweakTarget::NvidiaClassSubkey
                : static_cast<TweakTarget>(targetVal.toInt(
                    static_cast<int>(TweakTarget::NvidiaClassSubkey)));
            s.classSubkey   = o.value(QStringLiteral("subkey")).toString();
            s.valueName     = o.value(QStringLiteral("name")).toString();
            s.existedBefore = o.value(QStringLiteral("existed")).toBool();
            s.isDword       = o.value(QStringLiteral("isDword")).toBool();
            s.prevDword     = static_cast<quint32>(
                o.value(QStringLiteral("prevDword")).toVariant().toLongLong());
            s.prevString    = o.value(QStringLiteral("prevString")).toString();
            s.isBinary   = o.value(QStringLiteral("isBinary")).toBool(false);
            const QString prevB64 = o.value(QStringLiteral("prevBinary")).toString();
            if (!prevB64.isEmpty()) {
                s.prevBinary = QByteArray::fromBase64(prevB64.toLatin1());
            }
            out.append(s);
        }
        return out;
    }
    void savePersistedSnapshots(const QVector<RegSnapshot>& snaps) {
        paths::ensureDir(paths::userData());
        QSaveFile f(snapshotPath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
        QJsonArray arr;
        for (const auto& s : snaps) {
            QJsonObject o;
            o[QStringLiteral("target")]      = static_cast<int>(s.target);
            o[QStringLiteral("subkey")]      = s.classSubkey;
            o[QStringLiteral("name")]        = s.valueName;
            o[QStringLiteral("existed")]     = s.existedBefore;
            o[QStringLiteral("isDword")]     = s.isDword;
            o[QStringLiteral("prevDword")]   = static_cast<qint64>(s.prevDword);
            o[QStringLiteral("prevString")]  = s.prevString;
            o[QStringLiteral("isBinary")]    = s.isBinary;
            if (!s.prevBinary.isEmpty()) {
                o[QStringLiteral("prevBinary")]
                    = QString::fromLatin1(s.prevBinary.toBase64());
            }
            arr.append(o);
        }
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        f.commit();
    }
    ApplyResult revertRegistryTweaks() {
        ApplyResult r;
        if (!vth::win::isElevated()) {
            r.error = QStringLiteral(
                "Revert needs admin (HKLM writes). Re-launch Vethida elevated.");
            return r;
        }
        const QVector<RegSnapshot> snaps = loadPersistedSnapshots();
        if (snaps.isEmpty()) {
            r.ok = true;
            return r;
        }
        for (auto it = snaps.rbegin(); it != snaps.rend(); ++it) {
            const auto& s = *it;
            QString fullPath;
            switch (s.target) {
                case TweakTarget::NvidiaClassSubkey:
                case TweakTarget::HdmiAudioClassSubkey:
                    fullPath = QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Class\\")
                             + s.classSubkey;
                    break;
                case TweakTarget::GraphicsDrivers:
                case TweakTarget::NvDisplayContainer:
                case TweakTarget::WindowsUpdatePolicy:
                    fullPath = s.classSubkey;
                    break;
            }
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              fullPath.toStdWString().c_str(),
                              0, KEY_WRITE, &key) != ERROR_SUCCESS) {
                r.failure << QStringLiteral("%1\\%2").arg(s.classSubkey, s.valueName);
                continue;
            }
            LSTATUS w;
            if (!s.existedBefore) {
                w = RegDeleteValueW(key, s.valueName.toStdWString().c_str());
                if (w == ERROR_FILE_NOT_FOUND) w = ERROR_SUCCESS;
            } else if (s.isBinary) {
                w = RegSetValueExW(key, s.valueName.toStdWString().c_str(),
                                   0, REG_BINARY,
                                   reinterpret_cast<const BYTE*>(s.prevBinary.constData()),
                                   static_cast<DWORD>(s.prevBinary.size()));
            } else if (s.isDword) {
                const DWORD v = s.prevDword;
                w = RegSetValueExW(key, s.valueName.toStdWString().c_str(),
                                   0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&v), sizeof(v));
            } else {
                const std::wstring sw = s.prevString.toStdWString();
                w = RegSetValueExW(key, s.valueName.toStdWString().c_str(),
                                   0, REG_SZ,
                                   reinterpret_cast<const BYTE*>(sw.c_str()),
                                   static_cast<DWORD>((sw.size() + 1) * sizeof(wchar_t)));
            }
            RegCloseKey(key);
            if (w == ERROR_SUCCESS) r.success << QStringLiteral("%1\\%2").arg(s.classSubkey, s.valueName);
            else                    r.failure << QStringLiteral("%1\\%2").arg(s.classSubkey, s.valueName);
        }
        savePersistedSnapshots({});
        if (auto* tracker = ServiceRegistry::instance().get<TweakStateTracker>()) {
            tracker->clearDomain(QStringLiteral("nvidia.regtweaks"));
        }
        r.ok = !r.success.isEmpty();
        return r;
    }
}

