#pragma once

#include <QString>
#include <QVector>

namespace vth::ui::models
{
    // The tweak categories the Tweaks tab renders, and their rail words.
    struct TweakCategory
    {
        QString key;
        QString label;
        QString icon;
        // Shown in the content header of the selected category.
        QString blurb;
    };

    // In display order.
    const QVector<TweakCategory>& tweakCategories();

        // Categories rendered by another page.
    bool categoryBelongsToAnotherPage(const QString& key);
}
