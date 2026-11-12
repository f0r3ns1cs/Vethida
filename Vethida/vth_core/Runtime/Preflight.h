#pragma once

#include <QString>
#include <QVector>
#include <QStringList>

// Checks to run before applying tweaks.
namespace vth::preflight
{
    enum class Outcome {
        Ok,
        Warn,
        Block,
    };

    struct CheckResult {
        Outcome  outcome = Outcome::Ok;
        QString  message;
        QString  remediation;
        QString  id;
    };

    struct Report {
        QVector<CheckResult> results;

        bool hasBlockers() const noexcept {
            for (const auto& r : results) if (r.outcome == Outcome::Block) return true;
            return false;
        }

        QVector<CheckResult> blockers() const;
        bool allOk() const noexcept {
            for (const auto& r : results) if (r.outcome != Outcome::Ok) return false;
            return true;
        }
        bool hasWarnings() const noexcept {
            for (const auto& r : results) if (r.outcome == Outcome::Warn) return true;
            return false;
        }
        QVector<CheckResult> warnings() const;
    };

    Report check();
}
