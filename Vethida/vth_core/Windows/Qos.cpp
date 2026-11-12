#include "Qos.h"

#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStringLiteral>

#include <windows.h>

namespace vth::win::qos
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catQos, "vth.qos")

        QString storePath() {
            return paths::protectedData() + QStringLiteral("/qos-policies.json");
        }

        Direction directionFromString(const QString& s) {
            if (s.compare(QStringLiteral("Inbound"), Qt::CaseInsensitive) == 0)
                return Direction::Inbound;
            if (s.compare(QStringLiteral("Both"), Qt::CaseInsensitive) == 0)
                return Direction::Both;
            return Direction::Outbound;
        }

        Protocol protocolFromString(const QString& s) {
            if (s.compare(QStringLiteral("UDP"), Qt::CaseInsensitive) == 0)
                return Protocol::UDP;
            if (s.compare(QStringLiteral("Both"), Qt::CaseInsensitive) == 0)
                return Protocol::Both;
            return Protocol::TCP;
        }

        QJsonObject toJson(const Policy& p) {
            QJsonObject o;
            o[QStringLiteral("id")]           = p.id;
            o[QStringLiteral("name")]         = p.name;
            o[QStringLiteral("application")]  = p.application;
            o[QStringLiteral("direction")]    = directionLabel(p.direction);
            o[QStringLiteral("protocol")]     = protocolLabel(p.protocol);
            o[QStringLiteral("portRange")]    = p.portRange;
            o[QStringLiteral("dscp")]         = p.dscp;
            o[QStringLiteral("throttleRate")] = p.throttleRate;
            o[QStringLiteral("enabled")]      = p.enabled;
            return o;
        }

        Policy fromJson(const QJsonObject& o) {
            Policy p;
            p.id           = o.value(QStringLiteral("id")).toString();
            p.name         = o.value(QStringLiteral("name")).toString();
            p.application  = o.value(QStringLiteral("application")).toString();
            p.direction    = directionFromString(o.value(QStringLiteral("direction")).toString());
            p.protocol     = protocolFromString(o.value(QStringLiteral("protocol")).toString());
            p.portRange    = o.value(QStringLiteral("portRange")).toString();
            p.dscp         = o.value(QStringLiteral("dscp")).toInt();
            p.throttleRate = o.value(QStringLiteral("throttleRate")).toString(QStringLiteral("Unlimited"));
            p.enabled      = o.value(QStringLiteral("enabled")).toBool(true);
            return p;
        }

        bool saveStore(const QVector<Policy>& policies) {
            paths::ensureDir(paths::protectedData());

            QJsonArray arr;
            for (const auto& p : policies) arr.append(toJson(p));

            QSaveFile f(storePath());
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qCWarning(catQos) << "save: open failed:" << f.errorString();
                return false;
            }
            f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
            if (!f.commit()) {
                qCWarning(catQos) << "save: commit failed:" << f.errorString();
                return false;
            }
            return true;
        }

        QString generatePolicyId() {
            return QStringLiteral("p") + QString::number(QDateTime::currentMSecsSinceEpoch(), 16);
        }

        // PowerShell ends a single-quoted string at any of these marks, so double them all.
        QString quoteForPs(const QString& s) {
            QString out;
            out.reserve(s.size() + 2);
            for (const QChar c : s) {
                const char16_t u = c.unicode();
                const bool quote = u == 0x27 || u == 0x2018 || u == 0x2019
                                || u == 0x201A || u == 0x201B;
                out.append(c);
                if (quote)
                    out.append(c);
            }
            return QStringLiteral("'") + out + QStringLiteral("'");
        }

        bool errorIndicatesAccessDenied(const QString& err) {
            return err.contains(QStringLiteral("AccessDenied"), Qt::CaseInsensitive)
                || err.contains(QStringLiteral("Access is denied"), Qt::CaseInsensitive)
                || err.contains(QStringLiteral("requires elevation"), Qt::CaseInsensitive)
                || err.contains(QStringLiteral("Requested registry access is not allowed"), Qt::CaseInsensitive);
        }

        bool errorIndicatesNotFound(const QString& err) {
            return err.contains(QStringLiteral("NotFound"), Qt::CaseInsensitive)
                || err.contains(QStringLiteral("Cannot find"), Qt::CaseInsensitive)
                || err.contains(QStringLiteral("not exist"), Qt::CaseInsensitive);
        }

        // Absolute, because this runs elevated and PATH could be hijacked.
        QString pinnedPowerShell() {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            const QString system32 = (n > 0 && n < MAX_PATH)
                ? QString::fromWCharArray(dir, static_cast<int>(n))
                : QStringLiteral("C:\\Windows\\System32");
            return system32 + QStringLiteral("\\WindowsPowerShell\\v1.0\\powershell.exe");
        }

        Result runPowerShell(const QString& script, QString* errorOut) {
            QProcess p;
            p.setProgram(pinnedPowerShell());
            p.setArguments({
                QStringLiteral("-NoProfile"),
                QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"), script,
            });
            p.start();
            if (!p.waitForStarted(5000)) {
                if (errorOut) *errorOut = QStringLiteral("Windows did not accept the change.");
                return Result::OtherError;
            }
            if (!p.waitForFinished(15000)) {
                p.kill();
                p.waitForFinished(2000);
                if (errorOut) *errorOut = QStringLiteral("Windows did not answer in time.");
                return Result::OtherError;
            }
            const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
            const QString err = QString::fromLocal8Bit(p.readAllStandardError());

            if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0)
                return Result::Ok;
            if (errorOut) *errorOut = err.isEmpty() ? out : err;
            if (errorIndicatesAccessDenied(err)) return Result::AccessDenied;
            if (errorIndicatesNotFound(err))     return Result::NotFound;
            return Result::OtherError;
        }
    }

    QString directionLabel(Direction d) {
        switch (d) {
            case Direction::Inbound:  return QStringLiteral("Inbound");
            case Direction::Outbound: return QStringLiteral("Outbound");
            case Direction::Both:     return QStringLiteral("Both");
        }
        return {};
    }

    QString protocolLabel(Protocol p) {
        switch (p) {
            case Protocol::TCP:  return QStringLiteral("TCP");
            case Protocol::UDP:  return QStringLiteral("UDP");
            case Protocol::Both: return QStringLiteral("Both");
        }
        return {};
    }

    QVector<Policy> loadStore() {
        QFile f(storePath());
        if (!f.exists()) return {};
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(catQos) << "load: open failed:" << f.errorString();
            return {};
        }
        QJsonParseError err;
        const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError) {
            qCWarning(catQos) << "load: parse failed:" << err.errorString();
            return {};
        }
        if (!doc.isArray()) return {};

        QVector<Policy> out;
        const auto arr = doc.array();
        out.reserve(arr.size());
        for (const auto& v : arr) {
            if (v.isObject()) out.append(fromJson(v.toObject()));
        }
        return out;
    }

    Policy createPolicy(Policy templ) {
        auto policies = loadStore();
        if (templ.id.isEmpty()) templ.id = generatePolicyId();
        policies.append(templ);
        saveStore(policies);
        return templ;
    }

    bool deletePolicy(const QString& id) {
        auto policies = loadStore();
        if (policies.removeIf([&id](const Policy& p) { return p.id == id; }) == 0)
            return false;
        return saveStore(policies);
    }

    bool togglePolicy(const QString& id) {
        auto policies = loadStore();
        for (auto& p : policies) {
            if (p.id == id) {
                p.enabled = !p.enabled;
                return saveStore(policies);
            }
        }
        return false;
    }

    double parseThrottleMbps(const QString& rate) {
        static const QRegularExpression re(
            QStringLiteral(R"(^\s*(\d+(?:\.\d+)?)\s*Mbps\s*$)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(rate);
        if (!m.hasMatch()) return 0.0;
        bool ok = false;
        const double v = m.captured(1).toDouble(&ok);
        return ok ? v : 0.0;
    }

    Summary summarize(const QVector<Policy>& policies) {
        Summary s;
        s.total = policies.size();
        for (const auto& p : policies) {
            if (p.enabled)
                ++s.active;
        }
        return s;
    }

    QString resultLabel(Result r) {
        switch (r) {
            case Result::Ok:              return QStringLiteral("Applied");
            case Result::AccessDenied:    return QStringLiteral("Access denied (admin required)");
            case Result::NotFound:        return QStringLiteral("Policy not found on system");
            case Result::InvalidArgument: return QStringLiteral("Invalid argument");
            case Result::OtherError:      return QStringLiteral("Failed");
        }
        return {};
    }

    Result applyToSystem(const Policy& p, QString* errorOut) {
        if (p.name.isEmpty()) {
            if (errorOut) *errorOut = QStringLiteral("Policy name is required");
            return Result::InvalidArgument;
        }

        QStringList args;
        args << QStringLiteral("-Name ") + quoteForPs(p.name);
        args << QStringLiteral("-DSCPAction ") + QString::number(p.dscp);

        if (!p.application.isEmpty())
            args << QStringLiteral("-AppPathNameMatchCondition ") + quoteForPs(p.application);

        // Inbound matches the source port, anything else the destination port.
        if (!p.portRange.isEmpty()) {
            args << (p.direction == Direction::Inbound
                         ? QStringLiteral("-IPSrcPortMatchCondition ")
                         : QStringLiteral("-IPDstPortMatchCondition "))
                        + quoteForPs(p.portRange);
        }

        args << QStringLiteral("-IPProtocolMatchCondition ") + protocolLabel(p.protocol);
        args << QStringLiteral("-NetworkProfile All");

        if (const double mbps = parseThrottleMbps(p.throttleRate); mbps > 0.0) {
            const qint64 bps = static_cast<qint64>(mbps * 1'000'000.0);
            args << QStringLiteral("-ThrottleRateActionBitsPerSecond ") + QString::number(bps);
        }

        // If New fails because the rule already exists, Set updates it.
        const QString joined = args.join(QLatin1Char(' '));
        return runPowerShell(QStringLiteral("$ErrorActionPreference='Stop'; "
                                            "try { New-NetQosPolicy ")
                                 + joined
                                 + QStringLiteral(" | Out-Null } catch { Set-NetQosPolicy ")
                                 + joined
                                 + QStringLiteral(" | Out-Null }"),
                             errorOut);
    }

    Result removeFromSystem(const QString& policyName, QString* errorOut) {
        if (policyName.isEmpty()) {
            if (errorOut) *errorOut = QStringLiteral("Policy name is required");
            return Result::InvalidArgument;
        }
        const QString cmd = QStringLiteral("$ErrorActionPreference='Stop'; "
                                           "Remove-NetQosPolicy -Name ")
                            + quoteForPs(policyName)
                            + QStringLiteral(" -Confirm:$false | Out-Null");
        return runPowerShell(cmd, errorOut);
    }

    // DSCP values are from RFC 4594: EF is 46, AF41 is 34 and AF31 is 26.
    const QVector<Preset>& presets() {
        static const QVector<Preset> kPresets = {
            Preset{
                QStringLiteral("gaming"),
                QStringLiteral("Gaming"),
                QStringLiteral(
                    "Low-latency outbound traffic for online games. Marks this "
                    "traffic as high priority for network equipment that honors it."),
                46,
                Direction::Both,
                Protocol::UDP,
                QStringLiteral("gamepad"),
            },
            Preset{
                QStringLiteral("voice"),
                QStringLiteral("Voice call"),
                QStringLiteral(
                    "VoIP, Discord and Teams audio. Marks this traffic as high "
                    "priority for network equipment that honors it."),
                46,
                Direction::Both,
                Protocol::UDP,
                QStringLiteral("phone"),
            },
            Preset{
                QStringLiteral("video"),
                QStringLiteral("Streaming"),
                QStringLiteral(
                    "Live streaming or video calls. Marks this traffic as high "
                    "priority for network equipment that honors it."),
                34,
                Direction::Both,
                Protocol::Both,
                QStringLiteral("projection-screen"),
            },
            Preset{
                QStringLiteral("work"),
                QStringLiteral("Work / Remote"),
                QStringLiteral(
                    "Business apps, remote desktop and VPN. Marks this traffic "
                    "as high priority for network equipment that honors it."),
                26,
                Direction::Outbound,
                Protocol::TCP,
                QStringLiteral("briefcase"),
            },
            Preset{
                QStringLiteral("default"),
                QStringLiteral("Default"),
                QStringLiteral(
                    "No special marking, for background sync and updates."),
                0,
                Direction::Outbound,
                Protocol::Both,
                QStringLiteral("minus"),
            },
        };
        return kPresets;
    }
}

namespace vth::win::qos
{
namespace
{
        QString directionToJson(Direction d) {
            switch (d) {
                case Direction::Inbound:  return QStringLiteral("Inbound");
                case Direction::Outbound: return QStringLiteral("Outbound");
                case Direction::Both:     return QStringLiteral("Both");
            }
            return QStringLiteral("Outbound");
        }
        QString protocolToJson(Protocol p) {
            switch (p) {
                case Protocol::TCP:  return QStringLiteral("TCP");
                case Protocol::UDP:  return QStringLiteral("UDP");
                case Protocol::Both: return QStringLiteral("Both");
            }
            return QStringLiteral("TCP");
        }
}
}

namespace vth::win::qos
{
    bool updatePolicy(const Policy& updated) {
        auto policies = loadStore();
        for (int i = 0; i < policies.size(); ++i) {
            if (policies[i].id == updated.id) {
                policies[i] = updated;
                saveStore(policies);
                return true;
            }
        }
        return false;
    }
}
