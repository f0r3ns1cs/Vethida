#pragma once

// vth_core unit tests (catalog rules etc). Doesn't touch the real registry or run
// commands. Build vth_tests (x64) and run it.

#include "../vth_core/Tweaks/TweakEngine.h"
#include "../vth_core/Windows/HealthScore.h"
#include "../vth_core/Models/LicenseModels.h"
#include "../vth_core/Runtime/Paths.h"
#include "../vth_core/Services/SessionProjection.h"
#include "../vth_core/Services/IRegistryEditor.h"
#include "../vth_core/SystemInfo.h"
#include "../vth_core/Windows/Bloatware.h"
#include "../vth_core/Tweaks/DeviceOps.h"

#include <QHash>

#include <QObject>
#include <QSet>
#include <QtTest/QtTest>

namespace tw = vth::tweaks;

// fake in-memory registry for detection tests
class FakeRegistry final : public vth::services::IRegistryEditor
{
public:
    QHash<QString, QVariant> values;   // "path|name" -> value

    QVariant getValue(const QString& p, const QString& n) const override
    {
        return values.value(p + QLatin1Char('|') + n);
    }
    bool setValue(const QString& p, const QString& n, const QVariant& v,
                  vth::services::RegistryValueKind) override
    {
        values.insert(p + QLatin1Char('|') + n, v);
        return true;
    }
    bool deleteValue(const QString& p, const QString& n) override
    {
        values.remove(p + QLatin1Char('|') + n);
        return true;
    }
    bool valueExists(const QString& p, const QString& n) const override
    {
        return values.contains(p + QLatin1Char('|') + n);
    }
    std::vector<QString> enumerateValueNames(const QString&) const override { return {}; }
    std::vector<QString> enumerateSubKeys(const QString&) const override { return {}; }
    bool keyExists(const QString&) const override { return true; }
    bool createKey(const QString&) override { return true; }
    bool deleteKeyTree(const QString&) override { return true; }
};

class CoreTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void catalogIsNotEmpty()
    {
        QVERIFY(tw::catalog().size() > 50);
    }

    void idsAreUnique()
    {
        QSet<QString> seen;
        for (const tw::TweakSpec& t : tw::catalog()) {
            QVERIFY2(!t.id.isEmpty(), "a tweak has no id");
            QVERIFY2(!seen.contains(t.id), qPrintable(QStringLiteral("duplicate id ") + t.id));
            seen.insert(t.id);
        }
    }

    void everyTweakIsNamedAndDescribed()
    {
        for (const tw::TweakSpec& t : tw::catalog()) {
            QVERIFY2(!t.name.trimmed().isEmpty(), qPrintable(t.id + QStringLiteral(": no name")));
            QVERIFY2(!t.description.trimmed().isEmpty(),
                     qPrintable(t.id + QStringLiteral(": no description")));
            QVERIFY2(!t.category.isEmpty(), qPrintable(t.id + QStringLiteral(": no category")));
        }
    }

    void everyTweakDoesSomething()
    {
        for (const tw::TweakSpec& t : tw::catalog()) {
            const bool any = !t.registryOps.isEmpty() || !t.commandOps.isEmpty()
                          || !t.dynamicOps.isEmpty();
            QVERIFY2(any, qPrintable(t.id + QStringLiteral(": no operations")));
        }
    }

    void excludesResolveAndAreSymmetric()
    {
        for (const tw::TweakSpec& t : tw::catalog()) {
            for (const QString& other : t.excludes) {
                QVERIFY2(other != t.id, qPrintable(t.id + QStringLiteral(" excludes itself")));
                const tw::TweakSpec* o = tw::find(other);
                QVERIFY2(o, qPrintable(t.id + QStringLiteral(" excludes unknown ") + other));
                QVERIFY2(o->excludes.contains(t.id),
                         qPrintable(other + QStringLiteral(" does not exclude ") + t.id));
            }
        }
    }

    void warningsAndEffectsAreDistinct()
    {
        for (const tw::TweakSpec& t : tw::catalog()) {
            for (const QString& w : t.warnings) {
                QVERIFY2(!w.trimmed().isEmpty(), qPrintable(t.id + QStringLiteral(": empty warning")));
                QVERIFY2(!t.effects.contains(w),
                         qPrintable(t.id + QStringLiteral(": line is both a warning and an effect")));
            }
            for (const QString& e : t.effects)
                QVERIFY2(!e.trimmed().isEmpty(), qPrintable(t.id + QStringLiteral(": empty effect")));
        }
    }

    void confirmationFollowsTheRule()
    {
        int asks = 0;
        for (const tw::TweakSpec& t : tw::catalog()) {
            const bool expected = !t.reversible || t.askFirst;
            QCOMPARE(tw::needsUserConfirmation(t), expected);
            QCOMPARE(tw::isOneWay(t), !t.reversible);
            if (expected)
                ++asks;
        }
        // too many prompts and people stop reading them
        QVERIFY2(asks <= 25, qPrintable(QStringLiteral("too many prompts: %1").arg(asks)));
    }

    void impactLabelsAreWords()
    {
        for (tw::Impact i : { tw::Impact::Low, tw::Impact::Medium, tw::Impact::High,
                              tw::Impact::Critical })
            QVERIFY(!tw::impactLabel(i).isEmpty());
    }

    void scoreBandsCoverTheRange()
    {
        namespace health = vth::win::health;
        QCOMPARE(health::scoreBand(0),   QStringLiteral("Not optimized yet"));
        QCOMPARE(health::scoreBand(14),  QStringLiteral("Not optimized yet"));
        QCOMPARE(health::scoreBand(15),  QStringLiteral("Lightly optimized"));
        QCOMPARE(health::scoreBand(40),  QStringLiteral("Partly optimized"));
        QCOMPARE(health::scoreBand(70),  QStringLiteral("Well optimized"));
        QCOMPARE(health::scoreBand(90),  QStringLiteral("Fully optimized"));
        QCOMPARE(health::scoreBand(100), QStringLiteral("Fully optimized"));
    }

    // A value other than the tweak's own means "not applied", never "unknown".
    void detectionReadsOffForAnyOtherValue()
    {
        const QString path = QStringLiteral("HKCU\\Software\\VethidaTest");
        tw::TweakSpec spec;
        spec.id = QStringLiteral("test.detect");
        tw::RegistryOp op;
        op.path = path;
        op.name = QStringLiteral("Value");
        op.appliedValue = 1u;
        op.revertValue = 0u;
        spec.registryOps = { op };

        FakeRegistry reg;
        QCOMPARE(tw::detect(spec, reg), tw::DetectState::Off);          // absent
        reg.values.insert(path + QStringLiteral("|Value"), 7u);
        QCOMPARE(tw::detect(spec, reg), tw::DetectState::Off);          // a third value
        reg.values.insert(path + QStringLiteral("|Value"), 0u);
        QCOMPARE(tw::detect(spec, reg), tw::DetectState::Off);          // the default
        reg.values.insert(path + QStringLiteral("|Value"), 1u);
        QCOMPARE(tw::detect(spec, reg), tw::DetectState::On);
    }

    // Two page files must not read as the empty "no page file" list.
    void multiStringValuesCompareAsLists()
    {
        const QString path = QStringLiteral("HKCU\\Software\\VethidaTest");
        tw::TweakSpec spec;
        spec.id = QStringLiteral("test.multi");
        tw::RegistryOp op;
        op.path = path;
        op.name = QStringLiteral("PagingFiles");
        op.kind = vth::services::RegistryValueKind::MultiString;
        op.appliedValue = QStringList();
        spec.registryOps = { op };

        FakeRegistry reg;
        reg.values.insert(path + QStringLiteral("|PagingFiles"),
                          QStringList{ QStringLiteral("C:\\pagefile.sys 0 0"),
                                       QStringLiteral("D:\\pagefile.sys 0 0") });
        QCOMPARE(tw::detect(spec, reg), tw::DetectState::Off);
        reg.values.insert(path + QStringLiteral("|PagingFiles"), QStringList());
        QCOMPARE(tw::detect(spec, reg), tw::DetectState::On);
    }

    // A restart is shown by the flag, so the words never repeat it.
    void restartIsNotRestatedInText()
    {
        for (const tw::TweakSpec& t : tw::catalog()) {
            if (!t.requiresReboot)
                continue;
            QVERIFY2(!t.description.contains(QStringLiteral("Takes effect after a restart")),
                     qPrintable(t.id));
        }
    }

    // Only tweaks that act on NVIDIA's driver carry the NVIDIA mark.
    void schedulerPreemptionIsNotNvidiaOnly()
    {
        const tw::TweakSpec* t = tw::find(QStringLiteral("gpu.disable-scheduler-preemption"));
        QVERIFY(t);
        QVERIFY(tw::hardwareVendorKey(*t).isEmpty());
    }

    // "Remove recommended" never takes apps people choose for themselves.
    void bloatwareKeepsChosenAppsOutOfRecommended()
    {
        namespace bw = vth::win::bloatware;
        for (const char* pkg : { "SpotifyAB.SpotifyMusic", "4DF9E0F8.Netflix",
                                 "BytedancePte.Ltd.TikTok", "Disney.37853FC22B2CE",
                                 "Facebook.317180B0BB486", "Microsoft.YourPhone",
                                 "MicrosoftWindows.CrossDevice", "Microsoft.MSTeams",
                                 "Microsoft.WindowsCalculator", "Microsoft.ScreenSketch",
                                 "Microsoft.Windows.Photos" }) {
            const bw::Entry* e = bw::findByName(QString::fromLatin1(pkg));
            QVERIFY2(e, pkg);
            QVERIFY2(e->severity != bw::Severity::Recommended, pkg);
        }
        for (const bw::Entry& e : bw::catalog())
            QVERIFY2(!e.description.contains(QStringLiteral("UWP")), qPrintable(e.name));
    }

    // power API, so it doesn't depend on the Windows language. Read-only.
    void powerSettingsReadInAnyLanguage()
    {
        namespace dev = vth::tweaks::devices;
        const auto byAlias = dev::powercfgQueryAcIndex(QStringLiteral("SUB_PROCESSOR"),
                                                       QStringLiteral("IDLEDISABLE"));
        const auto byGuid = dev::powercfgQueryAcIndex(
            QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"),
            QStringLiteral("5d76a2ca-e8c0-402f-a133-2158492d58ad"));
        QVERIFY(byAlias.has_value());
        QVERIFY(byGuid.has_value());
        QCOMPARE(*byAlias, *byGuid);
    }

    void hardwareNamesDropTrademarks()
    {
        QCOMPARE(vth::win::tidyHardwareName(QStringLiteral("Intel(R) Iris(R) Xe Graphics")),
                 QStringLiteral("Intel Iris Xe Graphics"));
        QCOMPARE(vth::win::tidyHardwareName(QStringLiteral("11th Gen Intel(R) Core(TM) i5-1145G7")),
                 QStringLiteral("11th Gen Intel Core i5-1145G7"));
    }

    // Checked against a key from the licensing API.
    void licenseKeysAreCheckedFiveByFive()
    {
        using vth::models::LicenseValidation;
        const QString key = QStringLiteral("S7ESR-8HDH5-GJSVV-8GD26-21X3S");
        QVERIFY(LicenseValidation::isValidKeyFormat(key));
        QCOMPARE(LicenseValidation::formatKey(QStringLiteral(" s7esr8hdh5 gjsvv-8gd26-21x3s\n")), key);
        // One wrong character fails the group's check character.
        QVERIFY(!LicenseValidation::isValidKeyFormat(QStringLiteral("S7ESR-8HDH6-GJSVV-8GD26-21X3S")));
        QVERIFY(!LicenseValidation::isValidKeyFormat(QStringLiteral("38b1460a-5104-4067-a91d-77b872934d51")));
        QVERIFY(!LicenseValidation::isValidKeyFormat(QString()));
        QVERIFY(!LicenseValidation::maskKey(key).contains(QStringLiteral("21X3S")));
        QVERIFY(LicenseValidation::maskKey(key).startsWith(QStringLiteral("S7ESR")));
    }

    // offline shows in the title bar instead of just "Free"
    void offlineLicenseBadgeIsUrgent()
    {
        using namespace vth::services;
        SessionSnapshot snap;
        snap.status = vth::models::LicenseStatus::Valid;
        snap.tier   = vth::models::ProductTier::Free;
        snap.offline = true;
        const RibbonBadge stale = describeRibbonBadge(snap);
        QVERIFY(stale.urgent);

        snap.offline = false;
        snap.tier  = vth::models::ProductTier::Pro;
        const RibbonBadge fresh = describeRibbonBadge(snap);
        QVERIFY(!fresh.urgent);

        snap.status = vth::models::LicenseStatus::Revoked;
        QVERIFY(describeRibbonBadge(snap).urgent);
    }

    // elevated state lives apart from the user profile
    void actedOnStateIsNotInTheSettingsFolder()
    {
        const QString state = vth::paths::protectedData();
        QVERIFY(QDir(state).exists());
        QVERIFY(state != vth::paths::userData());
    }
};
