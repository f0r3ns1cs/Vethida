#include "pch.h"
#include "TweaksPage.h"

#include "../Models/TweakCategories.h"

#include "HubPage.h"
#include "TweakController.h"

#include "../Views/TweakSectionView.h"

#include "../../vth_core/Tweaks/TweakEngine.h"

#include <QLabel>
#include <QSet>
#include <QShowEvent>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        // The privacy and security catalog categories, shown together as one
        // rail section. The '+' key is read by recommend::categoryMatches.
        const QLatin1StringView kPrivacySecurityKey("privacy+security");
    }

    TweaksPage::TweaksPage(TweakController* controller, QWidget* parent)
        : QWidget(parent)
        , m_controller(controller)
    {
        buildUi();
        connect(m_controller, &TweakController::operationFinished,
                this, &TweaksPage::onOperationFinished);
    }

    void TweaksPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!m_shownOnce) {
            m_shownOnce = true;
            m_controller->refreshDetection();
        }
    }

    void TweaksPage::buildUi()
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // Bucket the catalog by category in first-seen order.
        const auto& catalog = vth::tweaks::catalog();
        QHash<QString, int> byKey;
        const auto ensureCat = [&](const QString& key, const QString& label,
                                   const QString& icon, const QString& blurb) {
            if (byKey.contains(key))
                return byKey.value(key);
            const int idx = m_cats.size();
            m_cats.push_back({ key, label, icon, blurb });
            m_catSpecs.push_back({});
            byKey.insert(key, idx);
            return idx;
        };
        // Already translated in the table.
        for (const auto& c : models::tweakCategories())
            ensureCat(c.key, c.label, c.icon, c.blurb);
        for (const auto& t : catalog) {
            if (models::categoryBelongsToAnotherPage(t.category))
                continue;
            // A category no table row claims lands here. No blurb, because nothing here
            // knows what it contains.
            const int idx = ensureCat(t.category,
                                      t.category.isEmpty() ? tr("Other") : t.category,
                                      QStringLiteral("adjustments-horizontal"),
                                      QString());
            m_catSpecs[idx].push_back(&t);
        }
        // Drop categories that ended up empty (a table entry with no tweaks on this PC).
        for (int i = m_cats.size() - 1; i >= 0; --i) {
            if (m_catSpecs.at(i).isEmpty()) {
                m_cats.remove(i);
                m_catSpecs.remove(i);
            }
        }
        m_hub = new HubPage(
            tr("Tweaks"),
            tr("Windows and driver settings, grouped by what they affect."),
            this);
        root->addWidget(m_hub, 1);

        if (!m_controller->elevated()) {
            auto* note = new QLabel(
                tr("Browse freely. To apply tweaks, open Vethida as administrator."), m_hub);
            note->setProperty("uiRole", QStringLiteral("bodyText"));
            note->setWordWrap(true);
            m_hub->addBodyWidget(note);
        }

        m_rebootNote = new QLabel(tr("Some tweaks need a restart to take effect."), m_hub);
        m_rebootNote->setProperty("uiRole", QStringLiteral("bodyText"));
        m_rebootNote->setVisible(false);
        m_hub->addBodyWidget(m_rebootNote);

        for (int i = 0; i < m_cats.size(); ++i) {
            m_hub->addPageCard(
                m_cats.at(i).icon, m_cats.at(i).label, m_cats.at(i).blurb,
                [this, i](QWidget* p) -> QWidget* { return buildCategoryPane(i, p); });
        }

        // one section for both categories, subcategories become the headings
        TweakController* controller = m_controller;
        m_hub->addPageCard(
            QStringLiteral("shield-check"),
            tr("Privacy & security"),
            tr("What Windows collects and shares, and how it protects you"),
            [controller](QWidget* p) -> QWidget* {
                const QString key(kPrivacySecurityKey);
                auto* view = new TweakSectionView(controller, key, QString(), p,
                                                  QString(), TweakSectionView::Chrome::None);
                view->setBulkCategory(key);
                return view;
            });
    }

    // One category as a rail pane.
    QWidget* TweaksPage::buildCategoryPane(int cat, QWidget* parent)
    {
        auto* view = new TweakSectionView(m_controller, m_cats.at(cat).key,
                                          QString(), parent, QString(),
                                          TweakSectionView::Chrome::None);
        view->setBulkCategory(m_cats.at(cat).key);
        return view;
    }

    void TweaksPage::onOperationFinished(const QString& id,
                                         const vth::tweaks::ApplyOutcome& outcome)
    {
        const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
        if (outcome.ok && spec && spec->requiresReboot && m_rebootNote)
            m_rebootNote->setVisible(true);
    }
}
