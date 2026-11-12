#include "Nvram.h"

#include "BiosParser.h"
#include "../Logger.h"
#include "../Runtime/Tier.h"
#include "../Runtime/Paths.h"
#include "../Runtime/ServiceRegistry.h"
#include "../Runtime/Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QTextStream>

namespace vth::win::nvram
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catNvram, "vth.nvram")

        QString trimStart(const QString& s) {
            int start = 0;
            while (start < s.size() && s.at(start).isSpace()) ++start;
            return s.mid(start);
        }

        const QRegularExpression& reFileName() {
            static const QRegularExpression r(QStringLiteral(R"(^//\s*Script File Name\s*:\s*(.+)$)"));
            return r;
        }
        const QRegularExpression& reCreated() {
            static const QRegularExpression r(QStringLiteral(R"(^//\s*Created on\s+(.+)$)"));
            return r;
        }
        const QRegularExpression& reVersion() {
            static const QRegularExpression r(QStringLiteral(R"(^//\s*AMISCE Utility\.\s*Ver\s+(.+)$)"));
            return r;
        }
        const QRegularExpression& reCrc32() {
            static const QRegularExpression r(QStringLiteral(R"(^HIICrc32\s*=\s*(\S+))"));
            return r;
        }
        const QRegularExpression& reSetupQuestion() {
            static const QRegularExpression r(QStringLiteral(R"(^Setup Question\s*=\s*(.+)$)"));
            return r;
        }
        const QRegularExpression& reHelpString() {
            static const QRegularExpression r(QStringLiteral(R"(^Help String\s*=\s*(.+)$)"));
            return r;
        }
        const QRegularExpression& reToken() {
            static const QRegularExpression r(QStringLiteral(R"(^Token\s*=\s*(\S+))"));
            return r;
        }
        const QRegularExpression& reOffset() {
            static const QRegularExpression r(QStringLiteral(R"(^Offset\s*=\s*(\S+))"));
            return r;
        }
        const QRegularExpression& reWidth() {
            static const QRegularExpression r(QStringLiteral(R"(^Width\s*=\s*(\S+))"));
            return r;
        }
        const QRegularExpression& reOption() {
            static const QRegularExpression r(QStringLiteral(R"((\*?)\[([^\]]+)\]([^/\r\n]*))"));
            return r;
        }
        const QRegularExpression& reIntValue() {
            static const QRegularExpression r(QStringLiteral(R"(^Value\s*=\s*<(\d+)>)"));
            return r;
        }
        const QRegularExpression& reStringValue() {
            static const QRegularExpression r(QStringLiteral(R"VTH(^Value\s*=\s*"([^"]*)")VTH"));
            return r;
        }
        const QRegularExpression& reRawValue() {
            static const QRegularExpression r(QStringLiteral(R"(^Value\s*=\s*(.+)$)"));
            return r;
        }

        void finalizeEntry(Entry& e, Document& doc) {
            e.originalValue = e.currentValue();
            doc.entries.append(e);
        }

        QString patchOptionsLine(const QString& line, const Entry& e) {
            const QString currentValue = e.currentValue();
            QRegularExpressionMatchIterator it = reOption().globalMatch(line);
            QString rebuilt;
            int last = 0;
            while (it.hasNext()) {
                const QRegularExpressionMatch m = it.next();
                rebuilt.append(line.mid(last, m.capturedStart() - last));
                const QString code  = m.captured(2).trimmed();
                const QString label = m.captured(3);
                const QString prefix = (code == currentValue)
                    ? QStringLiteral("*") : QString{};
                rebuilt.append(prefix);
                rebuilt.append(QLatin1Char('['));
                rebuilt.append(code);
                rebuilt.append(QLatin1Char(']'));
                rebuilt.append(label);
                last = m.capturedEnd();
            }
            rebuilt.append(line.mid(last));
            return rebuilt;
        }
    }

    QString Entry::currentValue() const {
        if (valueKind == ValueKind::Options) {
            for (const auto& o : options) {
                if (o.selected) return o.code;
            }
            return {};
        }
        return value;
    }

    QString Entry::currentDisplayValue() const {
        if (valueKind == ValueKind::Options) {
            for (const auto& o : options) {
                if (o.selected) {
                    return QStringLiteral("%1 [%2]").arg(o.label, o.code);
                }
            }
            return QStringLiteral("(none)");
        }
        return value;
    }

    Document parseFile(const QString& filePath) {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            Document empty;
            empty.sourcePath = filePath;
            return empty;
        }
        QStringList lines;
        QTextStream in(&f);
        while (!in.atEnd()) lines << in.readLine();
        return parseLines(lines, filePath);
    }

    Document parseLines(const QStringList& lines, const QString& sourcePath) {
        Document doc;
        doc.rawLines    = lines;
        doc.sourcePath  = sourcePath;

        Entry current;
        bool haveCurrent       = false;
        bool inOptions         = false;

        for (int i = 0; i < lines.size(); ++i) {
            const QString line = trimStart(lines[i]);

            const bool lineIsCommented = line.startsWith(QStringLiteral("//"));
            const QString uncommented = trimStart(lineIsCommented ? line.mid(2) : line);

            QRegularExpressionMatch m;

            if ((m = reFileName().match(uncommented)).hasMatch()) {
                doc.header.fileName = m.captured(1).trimmed();
                continue;
            }
            if ((m = reCreated().match(uncommented)).hasMatch()) {
                doc.header.createdDate = m.captured(1).trimmed();
                continue;
            }
            if ((m = reVersion().match(uncommented)).hasMatch()) {
                doc.header.toolVersion = m.captured(1).trimmed();
                continue;
            }
            if ((m = reCrc32().match(uncommented)).hasMatch()) {
                doc.header.crc32 = m.captured(1).trimmed();
                continue;
            }

            if ((m = reSetupQuestion().match(uncommented)).hasMatch()) {
                if (haveCurrent) {
                    finalizeEntry(current, doc);
                    current = {};
                }
                current.setupQuestion = m.captured(1).trimmed();
                current.sourceLine    = i + 1;
                current.isCommented   = lineIsCommented;
                inOptions             = false;
                haveCurrent           = true;
                continue;
            }

            if (!haveCurrent) continue;

            if ((m = reHelpString().match(uncommented)).hasMatch()) {
                current.helpString = m.captured(1).trimmed();
                continue;
            }
            if ((m = reToken().match(uncommented)).hasMatch()) {
                current.token = m.captured(1).trimmed();
                continue;
            }
            if ((m = reOffset().match(uncommented)).hasMatch()) {
                current.offset = m.captured(1).trimmed();
                continue;
            }
            if ((m = reWidth().match(uncommented)).hasMatch()) {
                current.width = m.captured(1).trimmed();
                continue;
            }

            QString optionSource = uncommented;
            const int optIdx = optionSource.indexOf(QStringLiteral("Options"));
            if (optIdx >= 0) {
                const int eq = optionSource.indexOf(QLatin1Char('='), optIdx);
                if (eq >= 0) {
                    optionSource = optionSource.mid(eq + 1);
                    inOptions          = true;
                    current.valueKind  = ValueKind::Options;
                }
            }
            if (inOptions) {
                QRegularExpressionMatchIterator it = reOption().globalMatch(optionSource);
                int matchCount = 0;
                while (it.hasNext()) {
                    ++matchCount;
                    const QRegularExpressionMatch om = it.next();
                    Option o;
                    o.selected = (om.captured(1) == QStringLiteral("*"));
                    o.code     = om.captured(2).trimmed();
                    o.label    = om.captured(3).trimmed();
                    current.options.append(o);
                }
                if (matchCount > 0) continue;
                inOptions = false;
            }

            if ((m = reIntValue().match(uncommented)).hasMatch()) {
                current.valueKind = ValueKind::Numeric;
                current.value     = m.captured(1);
                continue;
            }
            if ((m = reStringValue().match(uncommented)).hasMatch()) {
                current.valueKind = ValueKind::Text;
                current.value     = m.captured(1);
                continue;
            }
            if ((m = reRawValue().match(uncommented)).hasMatch()) {
                current.valueKind = ValueKind::Raw;
                current.value     = m.captured(1).trimmed();
                continue;
            }

            if (uncommented.trimmed().isEmpty() && !current.token.isEmpty()) {
                finalizeEntry(current, doc);
                current = {};
                haveCurrent       = false;
                inOptions         = false;
            }
        }

        if (haveCurrent) {
            finalizeEntry(current, doc);
        }
        return doc;
    }

    QString serialize(const Document& doc) {
        // By line, because tokens repeat across variable stores, and a token match
        // would write one setting's value over another's.
        QHash<int, const Entry*> modifiedByLine;
        for (const auto& e : doc.entries) {
            if (e.isModified() && !e.isCommented) {
                modifiedByLine.insert(e.sourceLine, &e);
            }
        }

        if (modifiedByLine.isEmpty()) {
            return doc.rawLines.join(QChar::fromLatin1('\n'));
        }

        QStringList output = doc.rawLines;
        const Entry* active = nullptr;
        bool inOptions = false;

        for (int i = 0; i < output.size(); ++i) {
            const QString line = output[i];
            const QString trimmed = trimStart(line);

            if (trimmed.startsWith(QStringLiteral("//"))) continue;

            if (reSetupQuestion().match(trimmed).hasMatch()) {
                inOptions = false;
                active = modifiedByLine.value(i + 1, nullptr);
                continue;
            }

            if (!active) continue;

            const bool startsOptionsBlock =
                trimmed.contains(QStringLiteral("Options\t=")) ||
                trimmed.contains(QStringLiteral("Options ="));
            if (startsOptionsBlock) {
                inOptions = true;
                if (active->valueKind == ValueKind::Options) {
                    output[i] = patchOptionsLine(line, *active);
                    continue;
                }
            }

            if (inOptions && active->valueKind == ValueKind::Options) {
                if (reOption().match(trimmed).hasMatch()) {
                    output[i] = patchOptionsLine(line, *active);
                    continue;
                }
                inOptions = false;
            }

            if (!trimmed.startsWith(QStringLiteral("Value"))) continue;

            if (active->valueKind == ValueKind::Numeric) {
                const auto vm = reIntValue().match(trimmed);
                if (vm.hasMatch()) {
                    const QString from = QStringLiteral("<%1>").arg(vm.captured(1));
                    const QString to   = QStringLiteral("<%1>").arg(active->value);
                    output[i] = QString(line).replace(from, to);
                }
            } else if (active->valueKind == ValueKind::Text) {
                const auto vm = reStringValue().match(trimmed);
                if (vm.hasMatch()) {
                    const QString from = QStringLiteral("\"%1\"").arg(vm.captured(1));
                    const QString to   = QStringLiteral("\"%1\"").arg(active->value);
                    output[i] = QString(line).replace(from, to);
                }
            } else {
                output[i] = QStringLiteral("Value = ") + active->value;
            }
            active = nullptr;
            inOptions = false;
        }
        return output.join(QChar::fromLatin1('\n'));
    }

    ScewinPaths locateScewin() {
        auto checkDir = [](const QString& dir) -> ScewinPaths {
            ScewinPaths p;
            if (dir.isEmpty()) return p;
            p.exePath   = dir + QStringLiteral("/SCEWIN_64.exe");
            p.fldrvPath = dir + QStringLiteral("/amifldrv64.sys");
            p.gendrvPath= dir + QStringLiteral("/amigendrv64.sys");
            p.available = QFileInfo(p.exePath).exists()
                       && QFileInfo(p.fldrvPath).exists()
                       && QFileInfo(p.gendrvPath).exists();
            return p;
        };

        QStringList candidates;
        if (auto* settings = ServiceRegistry::instance().get<Settings>()) {
            const QString configured = settings->value(
                QStringLiteral("nvram/toolDirectory")).toString();
            if (!configured.isEmpty()) candidates << configured;
        }
        candidates << paths::installDir() + QStringLiteral("/tools/ami-scewin");

        for (const auto& dir : candidates) {
            const auto p = checkDir(dir);
            if (p.available) return p;
        }
        return ScewinPaths{};
    }

    bool isAmiFirmware() {
        // "American Megatrends Inc.", "American Megatrends International, LLC" or "AMI".
        const QString vendor = vth::win::biosparser::read().bios.vendor;
        return vendor.contains(QStringLiteral("American Megatrends"), Qt::CaseInsensitive)
            || vendor.compare(QStringLiteral("AMI"), Qt::CaseInsensitive) == 0;
    }

    QString baselinePath() {
        return paths::userData() + QStringLiteral("/nvram-baseline.txt");
    }

    bool hasBaseline() {
        const QFileInfo fi(baselinePath());
        return fi.exists() && fi.size() > 0;
    }

    bool saveBaselineIfAbsent(const QString& exportedPath) {
        if (hasBaseline())
            return true;
        if (!QFileInfo(exportedPath).exists())
            return false;
        if (!paths::ensureDir(paths::userData()))
            return false;
        // A zero-byte leftover from an interrupted copy would block QFile::copy.
        QFile::remove(baselinePath());
        const bool ok = QFile::copy(exportedPath, baselinePath());
        if (!ok)
            qCWarning(catNvram) << "could not save NVRAM baseline to" << baselinePath();
        return ok;
    }

    namespace
    {
        RunResult runScewin(const ScewinPaths& paths,
                            const QStringList& args,
                            const QString& workingDir) {
            RunResult r;
            if (!paths.available) {
                r.message = QStringLiteral("SCEWIN tool files not found.");
                return r;
            }

            // So an old log can't be taken for this run's.
            QFile::remove(workingDir + QStringLiteral("/log-file.txt"));

            QProcess p;
            p.setProgram(paths.exePath);
            p.setArguments(args);
            p.setWorkingDirectory(workingDir);
            p.start();
            if (!p.waitForStarted(5000)) {
                r.message = QStringLiteral(
                    "Windows would not start SCEWIN_64.exe. The usual cause is that it or "
                    "its driver was blocked before it ran - check that the three files in "
                    "the SCEWIN folder are present and unblocked, and that no security "
                    "product has quarantined them.");
                return r;
            }
            // A board with a large setup database can take well over a minute, and
            // stopping it during an import is dangerous.
            if (!p.waitForFinished(180'000)) {
                p.kill();
                p.waitForFinished(5000);
                r.message = QStringLiteral(
                    "SCEWIN did not finish within three minutes and was stopped. Nothing "
                    "was reported as written. Reboot before trying again, and do not "
                    "assume the previous attempt completed.");
                return r;
            }
            r.exitCode = p.exitCode();
            const QString stdoutText = QString::fromLocal8Bit(p.readAllStandardOutput());
            const QString stderrText = QString::fromLocal8Bit(p.readAllStandardError());

            QFile log(workingDir + QStringLiteral("/log-file.txt"));
            QString logText;
            if (log.open(QIODevice::ReadOnly)) {
                logText = QString::fromLocal8Bit(log.readAll());
            }

            r.success = p.exitStatus() == QProcess::NormalExit && r.exitCode == 0;
            r.message = !logText.trimmed().isEmpty()    ? logText.trimmed()
                      : !stdoutText.trimmed().isEmpty() ? stdoutText.trimmed()
                      : stderrText.trimmed();

            // It usually fails this quietly when Windows won't load its driver.
            if (!r.success && r.message.isEmpty()) {
                r.message = QStringLiteral(
                    "SCEWIN exited with code %1 and reported nothing. This usually means "
                    "Windows refused to load the driver it needs. Check Windows Security "
                    "> Device security > Core isolation, and Smart App Control under App "
                    "& browser control; either one will block it.")
                    .arg(r.exitCode);
            }
            return r;
        }
    }

    RunResult exportSnapshot(const ScewinPaths& paths, const QString& outputPath) {
        const QString workingDir = QFileInfo(paths.exePath).absolutePath();
        RunResult r = runScewin(paths,
                                {QStringLiteral("/o"),
                                 QStringLiteral("/s"),
                                 outputPath},
                                workingDir);
        if (r.success)
            return r;

        // retry with /d for Aptio V < 5.008 (only as a retry, it skips a safety check)
        if (!r.message.contains(QStringLiteral("Platform identification"),
                                Qt::CaseInsensitive))
            return r;

        qCWarning(catNvram) << "export: platform identification failed, retrying with /d";
        RunResult retry = runScewin(paths,
                                    {QStringLiteral("/o"),
                                     QStringLiteral("/s"),
                                     outputPath,
                                     QStringLiteral("/d")},
                                    workingDir);
        if (!retry.success && retry.message.isEmpty())
            retry.message = r.message;
        return retry;
    }

    RunResult importSnapshot(const ScewinPaths& paths, const QString& inputPath) {
        if (!vth::meetsTier(vth::models::ProductTier::Ultimate)) {
            RunResult r;
            r.message = QStringLiteral("Writing firmware settings needs an active "
                                       "Vethida Ultimate license.");
            return r;
        }
        if (!QFileInfo(inputPath).exists()) {
            RunResult r;
            r.message = QStringLiteral("Import file does not exist: ")
                           + inputPath;
            return r;
        }
        const QString workingDir = QFileInfo(paths.exePath).absolutePath();
        // /q hides the normal "unmatched question" warnings. Write failures still show.
        return runScewin(paths,
                         {QStringLiteral("/i"),
                          QStringLiteral("/s"),
                          inputPath,
                          QStringLiteral("/q")},
                         workingDir);
    }
}

namespace vth::win::nvram
{
void Entry::revertToOriginal() {
    if (valueKind == ValueKind::Options) {
        for (auto& o : options) {
            o.selected = (o.code == originalValue);
        }
    } else {
        value = originalValue;
    }
}
}
