#include "pch.h"
#include "TweakCategories.h"

#include <QCoreApplication>
#include <QSet>
#include <QStringLiteral>

#include <iterator>

namespace vth::ui::models
{
    namespace
    {
        // QT_TR_NOOP, because lupdate can't see a bare literal.
        struct Def
        {
            const char* key;
            const char* label;
            const char* icon;
            const char* blurb;
        };

        const Def kDefs[] = {
            { "general",    QT_TR_NOOP("General"),    "layout-panel-top",
              QT_TR_NOOP("Explorer, search, and general Windows behavior") },
            { "appearance", QT_TR_NOOP("Appearance"), "palette",
              QT_TR_NOOP("Animations, transparency, and visual effects") },
            { "storage",    QT_TR_NOOP("Storage"),    "folder-cog",
              QT_TR_NOOP("Filesystem, indexing, and disk behavior") },
            { "power",      QT_TR_NOOP("Power"),      "power",
              QT_TR_NOOP("Sleep, processor power, and device power saving") },
            { "advanced",   QT_TR_NOOP("Advanced"),   "layers",
              QT_TR_NOOP("Kernel, memory, and scheduler internals") },
        };
    }

    const QVector<TweakCategory>& tweakCategories()
    {
        static const QVector<TweakCategory> kCats = [] {
            QVector<TweakCategory> out;
            out.reserve(static_cast<int>(std::size(kDefs)));
            for (const Def& d : kDefs) {
                out.append(TweakCategory{
                    QString::fromLatin1(d.key),
                    QCoreApplication::translate("vth::ui::models", d.label),
                    QString::fromLatin1(d.icon),
                    QCoreApplication::translate("vth::ui::models", d.blurb),
                });
            }
            return out;
        }();
        return kCats;
    }

    bool categoryBelongsToAnotherPage(const QString& key)
    {
        static const QSet<QString> kOther = {
            QStringLiteral("bios"),          // its own page
            QStringLiteral("networking"),    // Network page
            QStringLiteral("gpu"),           // GPU page
            QStringLiteral("debloating"),    // Debloat page
            QStringLiteral("browseroffice"), // Debloat page
            // Sections on the Tweaks tab, added to its rail explicitly.
            QStringLiteral("privacy"),
            QStringLiteral("security"),
        };
        return kOther.contains(key);
    }
}
