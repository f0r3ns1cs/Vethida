#pragma once

#include <QLabel>
#include <QString>
#include <QStringLiteral>
#include <QWidget>

namespace vth::widgets
{
    // The read-only value beside a field label in a details dialog: selectable,
    // wrapping, and a dash when there is nothing to show.
    inline QLabel* makeFieldValue(const QString& text, QWidget* parent)
    {
        auto* label = new QLabel(text.isEmpty() ? QStringLiteral("-") : text, parent);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setWordWrap(true);
        return label;
    }
}
