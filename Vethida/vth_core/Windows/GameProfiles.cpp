#include "GameProfiles.h"

#include "../Logger.h"
#include "../Runtime/Tier.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSet>
#include <QStringLiteral>

#include <windows.h>

#include <string>
#include <utility>

namespace vth::win::gameprofiles
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catGames, "vth.gameprofiles")

        QString expandEnv(const QString& path) {
            const std::wstring in = path.toStdWString();
            const DWORD needed = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
            if (needed == 0) return path;
            std::wstring out(needed, L'\0');
            const DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), needed);
            if (wrote == 0 || wrote > needed) return path;
            out.resize(wrote - 1);
            return QString::fromStdWString(out);
        }

        QString backupPathFor(const QString& settingsPath) {
            return settingsPath + QStringLiteral(".vth-bak");
        }

        QString stripQuotes(QString s) {
            s = s.trimmed();
            if (s.size() >= 2 && s.startsWith(QLatin1Char('"')) && s.endsWith(QLatin1Char('"'))) {
                s = s.mid(1, s.size() - 2);
            }
            return s;
        }

        bool isSectionHeader(const QString& trimmed) {
            return trimmed.size() >= 2
                && trimmed.startsWith(QLatin1Char('['))
                && trimmed.endsWith(QLatin1Char(']'));
        }

        bool isComment(const QString& trimmed) {
            return trimmed.startsWith(QStringLiteral("//"))
                || trimmed.startsWith(QLatin1Char('#'))
                || trimmed.startsWith(QLatin1Char(';'));
        }

        QVector<Setting> readSourceCfg(const QString& path, const QStringList& filterKeys) {
            QVector<Setting> out;
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return out;
            const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));

            for (const QString& raw : lines) {
                const QString trimmed = raw.trimmed();
                if (trimmed.isEmpty() || isComment(trimmed)) continue;

                QString key, value;
                const int eq = trimmed.indexOf(QLatin1Char('='));
                if (eq > 0) {
                    key   = trimmed.left(eq);
                    value = trimmed.mid(eq + 1);
                } else {
                    int sp = -1;
                    bool inQuote = false;
                    for (int i = 0; i < trimmed.size(); ++i) {
                        if (trimmed.at(i) == QLatin1Char('"')) inQuote = !inQuote;
                        else if (!inQuote && trimmed.at(i).isSpace()) { sp = i; break; }
                    }
                    if (sp <= 0) continue;
                    key   = trimmed.left(sp);
                    value = trimmed.mid(sp);
                }
                key   = stripQuotes(key);
                value = stripQuotes(value);
                if (key.isEmpty()) continue;
                if (!filterKeys.isEmpty() && !filterKeys.contains(key, Qt::CaseInsensitive)) continue;

                Setting s;
                s.key   = key;
                s.value = value;
                out.append(s);
            }
            return out;
        }

        bool writeSourceCfg(const QString& path, const QVector<Setting>& settings) {
            QFile in(path);
            QString original;
            if (in.open(QIODevice::ReadOnly | QIODevice::Text)) {
                original = QString::fromUtf8(in.readAll());
                in.close();
            }

            QStringList lines = original.split(QLatin1Char('\n'));
            QSet<QString> applied;

            for (QString& raw : lines) {
                const QString trimmed = raw.trimmed();
                if (trimmed.isEmpty() || isComment(trimmed)) continue;

                const int sp = trimmed.indexOf(QLatin1Char(' '));
                const int eq = trimmed.indexOf(QLatin1Char('='));
                const bool eqStyle = eq > 0 && (sp < 0 || eq < sp);
                const int splitAt = eqStyle ? eq : sp;
                if (splitAt <= 0) continue;
                const QString key = stripQuotes(trimmed.left(splitAt));

                for (const auto& s : settings) {
                    if (key.compare(s.key, Qt::CaseInsensitive) != 0) continue;
                    if (eqStyle) {
                        raw = s.key + QLatin1Char('=') + s.value;
                    } else if (trimmed.startsWith(QLatin1Char('"'))) {
                        raw = QStringLiteral("\"%1\" \"%2\"").arg(s.key, s.value);
                    } else {
                        raw = s.key + QLatin1Char(' ') + s.value;
                    }
                    applied.insert(s.key.toLower());
                }
            }

            QString result = lines.join(QLatin1Char('\n'));

            bool needsNewline = !result.endsWith(QLatin1Char('\n'));
            for (const auto& s : settings) {
                if (applied.contains(s.key.toLower())) continue;
                if (needsNewline) {
                    result += QLatin1Char('\n');
                    needsNewline = false;
                }
                result += s.key + QLatin1Char(' ') + s.value + QLatin1Char('\n');
            }

            QSaveFile out(path);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                return false;
            }
            out.write(result.toUtf8());
            return out.commit();
        }

        // QSettings cannot round-trip these files: UE section names contain
        // '/', which it treats as a group separator, and it rewrites the whole
        // file (reordering keys, dropping comments). Edit lines in place.
        QVector<Setting> readIni(const QString& path, const QStringList& filterKeys) {
            QVector<Setting> out;
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return out;
            const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));

            QString section;
            for (const QString& raw : lines) {
                const QString trimmed = raw.trimmed();
                if (trimmed.isEmpty() || isComment(trimmed)) continue;
                if (isSectionHeader(trimmed)) {
                    section = trimmed.mid(1, trimmed.size() - 2);
                    continue;
                }
                const int eq = trimmed.indexOf(QLatin1Char('='));
                if (eq <= 0) continue;
                const QString key = trimmed.left(eq).trimmed();
                if (!filterKeys.isEmpty()
                    && !filterKeys.contains(section + QLatin1Char('/') + key, Qt::CaseInsensitive)
                    && !filterKeys.contains(key, Qt::CaseInsensitive)) {
                    continue;
                }
                Setting s;
                s.section = section;
                s.key     = key;
                s.value   = stripQuotes(trimmed.mid(eq + 1));
                out.append(s);
            }
            return out;
        }

        bool writeIni(const QString& path, const QVector<Setting>& settings) {
            QFile in(path);
            QString original;
            if (in.open(QIODevice::ReadOnly | QIODevice::Text)) {
                original = QString::fromUtf8(in.readAll());
                in.close();
            }

            QStringList lines = original.split(QLatin1Char('\n'));
            QVector<bool> written(settings.size(), false);

            QString section;
            for (QString& raw : lines) {
                const QString trimmed = raw.trimmed();
                if (trimmed.isEmpty() || isComment(trimmed)) continue;
                if (isSectionHeader(trimmed)) {
                    section = trimmed.mid(1, trimmed.size() - 2);
                    continue;
                }
                const int eq = raw.indexOf(QLatin1Char('='));
                if (eq <= 0) continue;
                const QString key = raw.left(eq).trimmed();

                for (int i = 0; i < settings.size(); ++i) {
                    const Setting& s = settings.at(i);
                    if (section.compare(s.section, Qt::CaseInsensitive) != 0) continue;
                    if (key.compare(s.key, Qt::CaseInsensitive) != 0) continue;

                    // keep the file's own spacing and quoting (OW: Key = "1")
                    const bool spaced = raw.size() > eq + 1 && raw.at(eq + 1) == QLatin1Char(' ');
                    QString value = s.value;
                    if (raw.mid(eq + 1).trimmed().startsWith(QLatin1Char('"'))) {
                        value = QLatin1Char('"') + value + QLatin1Char('"');
                    }
                    raw = raw.left(eq + 1) + (spaced ? QStringLiteral(" ") : QString()) + value;
                    written[i] = true;
                }
            }

            auto headerIndex = [&lines](const QString& section) {
                for (int i = 0; i < lines.size(); ++i) {
                    const QString t = lines.at(i).trimmed();
                    if (isSectionHeader(t)
                        && t.mid(1, t.size() - 2).compare(section, Qt::CaseInsensitive) == 0) {
                        return i;
                    }
                }
                return -1;
            };

            for (int i = 0; i < settings.size(); ++i) {
                if (written.at(i)) continue;
                const Setting& s = settings.at(i);

                if (s.section.isEmpty()) {
                    lines.prepend(s.key + QLatin1Char('=') + s.value);
                    continue;
                }
                int header = headerIndex(s.section);
                if (header < 0) {
                    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
                        lines.removeLast();
                    }
                    lines.append(QLatin1Char('[') + s.section + QLatin1Char(']'));
                    header = lines.size() - 1;
                }
                int end = header + 1;
                while (end < lines.size() && !isSectionHeader(lines.at(end).trimmed())) ++end;
                while (end > header + 1 && lines.at(end - 1).trimmed().isEmpty()) --end;
                lines.insert(end, s.key + QLatin1Char('=') + s.value);
            }

            QSaveFile out(path);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                return false;
            }
            out.write(lines.join(QLatin1Char('\n')).toUtf8());
            return out.commit();
        }

        QString takeBackup(const QString& settingsPath) {
            const QString bak = backupPathFor(settingsPath);
            QFile::remove(bak);
            if (!QFile::copy(settingsPath, bak)) {
                qCWarning(catGames) << "Could not back up" << settingsPath;
                return {};
            }
            return bak;
        }
    }

    QVector<Profile> enumerate() {
        QVector<Profile> out;

        auto add = [&out](Profile p) {
            const QString resolved = QDir::cleanPath(expandEnv(p.settingsTemplate));
            p.settingsPath = QFile::exists(resolved) ? resolved : QString();
            out.append(std::move(p));
        };

        add(Profile{
            QStringLiteral("fortnite"),
            QStringLiteral("Fortnite"),
            QStringLiteral("Epic Games"),
            QStringLiteral("FN"),
            QStringLiteral("#737373"),
            {},
            QStringLiteral("%LOCALAPPDATA%\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini"),
            QStringLiteral("Render settings live in [/Script/FortniteGame.FortGameUserSettings], scalability in [ScalabilityGroups]."),
            ConfigFormat::Ini,
        });
        add(Profile{
            QStringLiteral("valorant"),
            QStringLiteral("Valorant"),
            QStringLiteral("Riot Games"),
            QStringLiteral("VL"),
            QStringLiteral("#ff4655"),
            {},
            QStringLiteral("%LOCALAPPDATA%\\VALORANT\\Saved\\Config"),
            QStringLiteral("Riot stores per-account settings under <RiotID>/Windows; back the whole folder up before editing."),
            ConfigFormat::Directory,
        });
        add(Profile{
            QStringLiteral("cs2"),
            QStringLiteral("Counter-Strike 2"),
            QStringLiteral("Valve"),
            QStringLiteral("CS"),
            QStringLiteral("#f7a800"),
            {},
            QStringLiteral("%PROGRAMFILES(X86)%\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\cfg\\autoexec.cfg"),
            QStringLiteral("Console cvars, one per line."),
            ConfigFormat::SourceCfg,
        });
        add(Profile{
            QStringLiteral("apex"),
            QStringLiteral("Apex Legends"),
            QStringLiteral("Respawn"),
            QStringLiteral("AP"),
            QStringLiteral("#da292a"),
            {},
            QStringLiteral("%USERPROFILE%\\Saved Games\\Respawn\\Apex\\local\\videoconfig.txt"),
            QStringLiteral("setting.* keys as quoted pairs; only existing keys are rewritten."),
            ConfigFormat::PlainKeyValue,
        });
        add(Profile{
            QStringLiteral("overwatch2"),
            QStringLiteral("Overwatch 2"),
            QStringLiteral("Blizzard"),
            QStringLiteral("OW"),
            QStringLiteral("#fa9c1e"),
            {},
            QStringLiteral("%USERPROFILE%\\Documents\\Overwatch\\Settings\\Settings_v0.ini"),
            QStringLiteral("Render settings live in [Render.13]; values are quoted."),
            ConfigFormat::Ini,
        });
        add(Profile{
            QStringLiteral("rocketleague"),
            QStringLiteral("Rocket League"),
            QStringLiteral("Psyonix"),
            QStringLiteral("RL"),
            QStringLiteral("#0089d0"),
            {},
            QStringLiteral("%USERPROFILE%\\Documents\\My Games\\Rocket League\\TAGame\\Config\\TASystemSettings.ini"),
            QStringLiteral("Video settings live in [SystemSettings]."),
            ConfigFormat::Ini,
        });
        add(Profile{
            QStringLiteral("dota2"),
            QStringLiteral("Dota 2"),
            QStringLiteral("Valve"),
            QStringLiteral("D2"),
            QStringLiteral("#df3030"),
            {},
            QStringLiteral("%PROGRAMFILES(X86)%\\Steam\\steamapps\\common\\dota 2 beta\\game\\dota\\cfg\\autoexec.cfg"),
            QStringLiteral("Console cvars, one per line."),
            ConfigFormat::SourceCfg,
        });
        add(Profile{
            QStringLiteral("lol"),
            QStringLiteral("League of Legends"),
            QStringLiteral("Riot Games"),
            QStringLiteral("LL"),
            QStringLiteral("#0bc4e2"),
            {},
            QStringLiteral("%SYSTEMDRIVE%\\Riot Games\\League of Legends\\Config\\game.cfg"),
            QStringLiteral("Sectioned ini; the client can rewrite it on exit."),
            ConfigFormat::Ini,
        });
        add(Profile{
            QStringLiteral("rust"),
            QStringLiteral("Rust"),
            QStringLiteral("Facepunch"),
            QStringLiteral("RU"),
            QStringLiteral("#cd412b"),
            {},
            QStringLiteral("%PROGRAMFILES(X86)%\\Steam\\steamapps\\common\\Rust\\cfg\\client.cfg"),
            QStringLiteral("client.cfg holds console cvars."),
            ConfigFormat::SourceCfg,
        });

        return out;
    }

    ReadResult readSettings(const Profile& p, const QStringList& keysOfInterest) {
        ReadResult r;
        if (p.settingsPath.isEmpty() || !QFile::exists(p.settingsPath)) {
            r.error = QStringLiteral("Settings file not found at %1").arg(p.settingsTemplate);
            return r;
        }
        switch (p.format) {
        case ConfigFormat::Ini:
            r.settings = readIni(p.settingsPath, keysOfInterest);
            break;
        case ConfigFormat::SourceCfg:
        case ConfigFormat::PlainKeyValue:
            r.settings = readSourceCfg(p.settingsPath, keysOfInterest);
            break;
        case ConfigFormat::Directory:
            r.error = QStringLiteral("%1 stores settings per account; Vethida cannot pick the right subdirectory automatically.").arg(p.name);
            return r;
        }
        r.ok = true;
        return r;
    }

    // writeSettings is gated; restoreBackup deliberately is not. Putting a
    // game's own file back is the undo direction, and a lapsed license must
    // never leave someone stuck with settings Vethida wrote.
    WriteResult writeSettings(const Profile& p, const QVector<Setting>& settings) {
        WriteResult r;
        if (!vth::meetsTier(vth::models::ProductTier::Pro)) {
            r.error = QStringLiteral("Writing a game profile needs an active Vethida license.");
            return r;
        }
        if (p.settingsPath.isEmpty()) {
            r.error = QStringLiteral("Settings file path is empty.");
            return r;
        }
        if (p.format == ConfigFormat::Directory) {
            r.error = QStringLiteral("%1 stores settings per account; Vethida cannot write here.").arg(p.name);
            return r;
        }

        if (QFile::exists(p.settingsPath)) {
            r.backupPath = takeBackup(p.settingsPath);
            if (r.backupPath.isEmpty()) {
                r.error = QStringLiteral("Could not back up %1; nothing was written.").arg(p.settingsPath);
                return r;
            }
        }

        bool ok = false;
        switch (p.format) {
        case ConfigFormat::Ini:
            ok = writeIni(p.settingsPath, settings);
            break;
        case ConfigFormat::SourceCfg:
        case ConfigFormat::PlainKeyValue:
            ok = writeSourceCfg(p.settingsPath, settings);
            break;
        case ConfigFormat::Directory:
            break;
        }
        if (!ok) {
            r.error = QStringLiteral("Could not write settings file. Check permissions.");
            return r;
        }
        qCInfo(catGames) << "Wrote" << settings.size() << "values to" << p.settingsPath;
        r.ok            = true;
        r.valuesWritten = static_cast<int>(settings.size());
        return r;
    }

    WriteResult restoreBackup(const Profile& p) {
        WriteResult r;
        const QString bak = backupPathFor(p.settingsPath);
        if (p.settingsPath.isEmpty() || !QFile::exists(bak)) {
            r.ok = true;
            return r;
        }
        QFile::remove(p.settingsPath);
        if (!QFile::copy(bak, p.settingsPath)) {
            r.error = QStringLiteral("Could not restore from backup at %1").arg(bak);
            return r;
        }
        qCInfo(catGames) << "Restored" << p.settingsPath << "from backup";
        r.ok         = true;
        r.backupPath = bak;
        return r;
    }

    QVector<Setting> recommendedProfile(const QString& profileId) {
        if (profileId == QStringLiteral("fortnite")) {
            const QString fort = QStringLiteral("/Script/FortniteGame.FortGameUserSettings");
            const QString sg   = QStringLiteral("ScalabilityGroups");
            return {
                {fort, QStringLiteral("PreferredFullscreenMode"),  QStringLiteral("0"),
                 QStringLiteral("Exclusive fullscreen")},
                {fort, QStringLiteral("FrameRateLimit"),           QStringLiteral("0.000000"),
                 QStringLiteral("Uncapped frame rate")},
                {fort, QStringLiteral("bUseVSync"),                QStringLiteral("False"),
                 QStringLiteral("VSync off")},
                {fort, QStringLiteral("bUseDynamicResolution"),    QStringLiteral("False"),
                 QStringLiteral("Dynamic resolution off")},
                {fort, QStringLiteral("bMotionBlur"),              QStringLiteral("False"),
                 QStringLiteral("Motion blur off")},
                {sg, QStringLiteral("sg.ResolutionQuality"),       QStringLiteral("100.000000"),
                 QStringLiteral("Native resolution")},
                {sg, QStringLiteral("sg.ViewDistanceQuality"),     QStringLiteral("3"),
                 QStringLiteral("View distance far")},
                {sg, QStringLiteral("sg.AntiAliasingQuality"),     QStringLiteral("0"),
                 QStringLiteral("Anti-aliasing off")},
                {sg, QStringLiteral("sg.ShadowQuality"),           QStringLiteral("0"),
                 QStringLiteral("Shadows off")},
                {sg, QStringLiteral("sg.PostProcessQuality"),      QStringLiteral("0"),
                 QStringLiteral("Post-processing low")},
                {sg, QStringLiteral("sg.TextureQuality"),          QStringLiteral("0"),
                 QStringLiteral("Textures low")},
                {sg, QStringLiteral("sg.EffectsQuality"),          QStringLiteral("1"),
                 QStringLiteral("Effects low")},
                {sg, QStringLiteral("sg.FoliageQuality"),          QStringLiteral("0"),
                 QStringLiteral("Foliage low")},
                {sg, QStringLiteral("sg.ShadingQuality"),          QStringLiteral("1"),
                 QStringLiteral("Shading low")},
            };
        }
        if (profileId == QStringLiteral("cs2")) {
            return {
                {{}, QStringLiteral("fps_max"),        QStringLiteral("0"),
                 QStringLiteral("Uncapped frame rate")},
                {{}, QStringLiteral("cl_radar_scale"), QStringLiteral("0.4"),
                 QStringLiteral("Compact radar")},
            };
        }
        if (profileId == QStringLiteral("apex")) {
            return {
                {{}, QStringLiteral("setting.csm_enabled"),               QStringLiteral("0"),
                 QStringLiteral("Cascade shadow maps off")},
                {{}, QStringLiteral("setting.dvs_enabled"),               QStringLiteral("0"),
                 QStringLiteral("Dynamic resolution off")},
                {{}, QStringLiteral("setting.particle_cpu_level"),        QStringLiteral("0"),
                 QStringLiteral("Particles low")},
                {{}, QStringLiteral("setting.mat_letterbox_aspect_goal"), QStringLiteral("0"),
                 QStringLiteral("No letterbox")},
            };
        }
        if (profileId == QStringLiteral("rocketleague")) {
            const QString sys = QStringLiteral("SystemSettings");
            return {
                {sys, QStringLiteral("MaxAnisotropy"),    QStringLiteral("1"),
                 QStringLiteral("Anisotropic filtering 1x")},
                {sys, QStringLiteral("MotionBlur"),       QStringLiteral("False"),
                 QStringLiteral("Motion blur off")},
                {sys, QStringLiteral("Bloom"),            QStringLiteral("False"),
                 QStringLiteral("Bloom off")},
                {sys, QStringLiteral("DepthOfField"),     QStringLiteral("False"),
                 QStringLiteral("Depth of field off")},
                {sys, QStringLiteral("AmbientOcclusion"), QStringLiteral("False"),
                 QStringLiteral("Ambient occlusion off")},
                {sys, QStringLiteral("Lensflares"),       QStringLiteral("False"),
                 QStringLiteral("Lens flares off")},
                {sys, QStringLiteral("LightShafts"),      QStringLiteral("False"),
                 QStringLiteral("Light shafts off")},
                {sys, QStringLiteral("DetailMode"),       QStringLiteral("0"),
                 QStringLiteral("Detail mode low")},
                {sys, QStringLiteral("MaxFPS"),           QStringLiteral("250"),
                 QStringLiteral("Frame rate cap 250")},
                {sys, QStringLiteral("bSmoothFrameRate"), QStringLiteral("False"),
                 QStringLiteral("Frame rate smoothing off")},
            };
        }
        if (profileId == QStringLiteral("overwatch2")) {
            const QString render = QStringLiteral("Render.13");
            return {
                {render, QStringLiteral("RenderScale"),          QStringLiteral("100"),
                 QStringLiteral("Native resolution")},
                {render, QStringLiteral("ReducedBufferingMode"), QStringLiteral("1"),
                 QStringLiteral("Reduced buffering on")},
                {render, QStringLiteral("DynamicReflections"),   QStringLiteral("0"),
                 QStringLiteral("Dynamic reflections off")},
                {render, QStringLiteral("ShadowDetail"),         QStringLiteral("0"),
                 QStringLiteral("Shadows low")},
                {render, QStringLiteral("AntialiasQuality"),     QStringLiteral("0"),
                 QStringLiteral("Anti-aliasing off")},
            };
        }
        if (profileId == QStringLiteral("rust")) {
            return {
                {{}, QStringLiteral("graphics.shadowdistance"), QStringLiteral("100"),
                 QStringLiteral("Shadow distance 100")},
                {{}, QStringLiteral("graphics.dof"),            QStringLiteral("False"),
                 QStringLiteral("Depth of field off")},
                {{}, QStringLiteral("graphics.motionblur"),     QStringLiteral("False"),
                 QStringLiteral("Motion blur off")},
                {{}, QStringLiteral("graphics.fov"),            QStringLiteral("90"),
                 QStringLiteral("FOV 90")},
            };
        }
        if (profileId == QStringLiteral("lol")) {
            const QString general = QStringLiteral("General");
            return {
                {general, QStringLiteral("FrameRateCap"),           QStringLiteral("0"),
                 QStringLiteral("Uncapped frame rate")},
                {general, QStringLiteral("WaitForVerticalSync"),    QStringLiteral("0"),
                 QStringLiteral("VSync off")},
                {general, QStringLiteral("ShadowQuality"),          QStringLiteral("0"),
                 QStringLiteral("Shadows off")},
                {general, QStringLiteral("EnvironmentQuality"),     QStringLiteral("1"),
                 QStringLiteral("Environment low")},
                {general, QStringLiteral("EffectsQuality"),         QStringLiteral("1"),
                 QStringLiteral("Effects low")},
                {general, QStringLiteral("DefaultParticleQuality"), QStringLiteral("1"),
                 QStringLiteral("Particles low")},
            };
        }
        return {};
    }

}
