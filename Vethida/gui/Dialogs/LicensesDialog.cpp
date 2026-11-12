#include "pch.h"
#include "LicensesDialog.h"

#include "../../vth_core/SiteUrls.h"

#include "Confirm.h"
#include "ToolModal.h"

#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/PanelDialog.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

namespace vth::dialogs
{
    namespace
    {
        namespace tokens = vth::theme::tokens;

        const QString kNoticesUrl = vth::site::kThirdParty;

        // The texts embedded in the executable, and the names they're saved under.
        struct Doc
        {
            const char* resource;
            const char* fileName;
        };
        constexpr Doc kDocs[] = {
            { ":/legal/THIRD_PARTY_NOTICES.txt",         "THIRD_PARTY_NOTICES.txt"          },
            { ":/legal/LGPL-3.0.txt",                    "LGPL-3.0.txt"                     },
            { ":/legal/GPL-3.0.txt",                     "GPL-3.0.txt"                      },
        };

        // Name, license, and what it does here.
        struct Component
        {
            const char* name;
            const char* license;
            const char* role;
        };
        constexpr Component kComponents[] = {
            { "Qt",                          "GNU LGPL v3",
              QT_TR_NOOP("User interface and platform toolkit. Dynamically linked.") },
            { "LZMA SDK",                    "Public domain",
              QT_TR_NOOP("Extracting 7z payloads.") },
            { "Tabler Icons",                "MIT",
              QT_TR_NOOP("Interface icons. Recoloured at run time; paths unmodified.") },
            // Nothing of NVIDIA's ships. nvapi64.dll comes from the installed display
            // driver, and Vethida resolves entry points from it at run time.
            { "NVIDIA NVAPI",                "Not redistributed",
              QT_TR_NOOP("GPU temperature and driver profile settings, through the NVIDIA driver already on this PC.") },
            { "sentry-native",               "MIT",
              QT_TR_NOOP("Crash reporting. Loaded only if you opted in.") },
            { "WinSparkle",                  "MIT",
              QT_TR_NOOP("Checking for, verifying and installing updates.") },
        };
    }

    LicensesPanel::LicensesPanel(QWidget* parent)
        : QWidget(parent)
    {
        buildUi();
    }

    void LicensesPanel::buildUi()
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                 tokens::PageMarginH, tokens::PageMarginBottom);
        root->setSpacing(tokens::CardMargin);

        // The disclosure scrolls; the two actions below it do not.
        auto* scroll = new QScrollArea(this);
        scroll->setObjectName(QStringLiteral("settingsScroll"));
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        auto* host = new QWidget(scroll);
        host->setObjectName(QStringLiteral("scrollContentHost"));
        auto* col = new QVBoxLayout(host);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(tokens::CardMargin);
        addDisclosure(col, host);
        col->addStretch(1);

        scroll->setWidget(host);
        root->addWidget(scroll, 1);

        auto* save = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary,
            tr("Save license texts"), this);
        connect(save, &QPushButton::clicked, this, &LicensesPanel::saveTexts);

        auto* open = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary,
            tr("Read the full notices"), this);
        connect(open, &QPushButton::clicked, this,
                []() { QDesktopServices::openUrl(QUrl(kNoticesUrl)); });

        root->addLayout(vth::dialogs::buttonRow({ save, open }));
    }

    void LicensesPanel::addDisclosure(QVBoxLayout* col, QWidget* host)
    {
        const QString qt = QStringLiteral(QT_VERSION_STR);
        const QString contact = QStringLiteral("legal@vethida.com");

        // Qt paints the anchors with QPalette::Link, so they follow the theme.
        const auto heading = [col, host](const QString& text) {
            auto* l = new QLabel(text, host);
            l->setProperty("uiRole", QStringLiteral("sectionTitle"));
            l->setContentsMargins(0, tokens::CompactGap, 0, 0);
            col->addWidget(l);
        };
        const auto body = [col, host](const QString& text) {
            auto* l = new QLabel(text, host);
            l->setProperty("uiRole", QStringLiteral("bodyText"));
            l->setWordWrap(true);
            l->setTextFormat(Qt::RichText);
            l->setOpenExternalLinks(true);
            // Not TextBrowserInteraction, which adds a focus stop per paragraph.
            l->setTextInteractionFlags(Qt::TextSelectableByMouse
                                       | Qt::LinksAccessibleByMouse);
            col->addWidget(l);
        };

        body(tr("Vethida is proprietary software. It uses the open-source "
                "components below, each under its own license; those licenses "
                "cover only the named components and not Vethida's own code."));

        heading(tr("Components"));
        for (const Component& c : kComponents) {
            auto* entry = new QWidget(host);
            auto* entryCol = new QVBoxLayout(entry);
            entryCol->setContentsMargins(0, 0, 0, 0);
            entryCol->setSpacing(tokens::Space2);

            auto* nameRow = new QHBoxLayout;
            nameRow->setContentsMargins(0, 0, 0, 0);
            nameRow->setSpacing(tokens::CompactGap);
            auto* name = new QLabel(QString::fromLatin1(c.name), entry);
            name->setObjectName(QStringLiteral("settingsRowTitle"));
            nameRow->addWidget(name, 0, Qt::AlignVCenter);
            auto* license = new QLabel(QString::fromLatin1(c.license), entry);
            license->setProperty("uiRole", QStringLiteral("chip"));
            nameRow->addWidget(license, 0, Qt::AlignVCenter);
            nameRow->addStretch(1);
            entryCol->addLayout(nameRow);

            auto* role = new QLabel(tr(c.role), entry);
            role->setObjectName(QStringLiteral("settingsRowDesc"));
            role->setWordWrap(true);
            entryCol->addWidget(role);

            col->addWidget(entry);
        }

        heading(tr("Qt, and your right to replace it"));
        body(tr("Vethida does not include the Qt framework in its own code; "
                "it links the Qt libraries dynamically at run time, version %1, "
                "under the GNU Lesser General Public License version 3 "
                "(LGPLv3). Qt is copyright The Qt Company Ltd and other "
                "contributors, and remains their work, licensed to you by them "
                "rather than by Vethida.")
                 .arg(qt));
        body(tr("You may modify the Qt libraries and run Vethida against "
                "your modified version. They are installed as separate files "
                "beside the Vethida executable, so replacing them needs no "
                "special tooling: build a compatible Qt of the same major "
                "version and overwrite the Qt files in the installation "
                "folder. Vethida applies no technical measure that prevents "
                "this, and doing it does not affect your license to use "
                "Vethida itself. You may also reverse engineer Vethida to the "
                "extent needed to debug your own modifications to those "
                "libraries."));

        heading(tr("Getting the source code"));
        body(tr("The complete corresponding source for the exact Qt build "
                "this release links, together with the scripts used to produce "
                "it, is available to anyone who has this software for at least "
                "three years from the date this version was distributed, at no "
                "charge beyond the cost of delivery."));
        body(tr("This notice is the written offer LGPLv3 requires. Write to "
                "%1 with the subject <b>Qt Source Request</b>, naming the "
                "Vethida version you have, and the source will be sent to "
                "you.")
                 .arg(contact));

        heading(tr("Licenses and attribution"));
        body(tr("The full license texts - including the GNU LGPL v3 and the "
                "GNU GPL v3 it incorporates by reference - are held inside this "
                "application. <b>Save license texts</b> writes them to a folder "
                "of your choosing, and needs no internet connection. The same "
                "notices, in a readable form, are at "
                "<a href=\"%1\">%1</a>.")
                 .arg(kNoticesUrl));
        body(tr("Vethida imposes no restriction on these components beyond "
                "what their own licenses state. They come with no warranty of "
                "any kind, to the extent the law allows, as those licenses set "
                "out; that disclaimer belongs to them and does not change any "
                "warranty Vethida gives for its own software under its End User "
                "License Agreement."));
        body(tr("Qt is a registered trademark of The Qt Company Ltd. Every "
                "other name here belongs to its respective owner and is named "
                "to attribute it."));
    }

    void LicensesPanel::saveTexts()
    {
        const QString base = QFileDialog::getExistingDirectory(
            this, tr("Choose where to save the license texts"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (base.isEmpty())
            return;

        QDir dir(base);
        const QString folder = QStringLiteral("Vethida licenses");
        if (!dir.exists(folder) && !dir.mkdir(folder)) {
            warn(this, tr("License texts not saved"),
                 tr("Vethida could not create a folder in %1. Choose somewhere "
                    "you can write to and try again.").arg(QDir::toNativeSeparators(base)));
            return;
        }
        dir.cd(folder);

        QStringList failed;
        for (const Doc& d : kDocs) {
            const QString target = dir.filePath(QString::fromLatin1(d.fileName));
            // QFile::copy refuses to overwrite, so saving twice would fail on
            // every file the second time.
            QFile::remove(target);
            if (!QFile::copy(QString::fromLatin1(d.resource), target)) {
                failed << QString::fromLatin1(d.fileName);
                continue;
            }
            // A file copied out of the Qt resource system inherits its read-only
            // permissions, which leaves the user with texts they cannot delete.
            QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner
                                              | QFile::ReadUser | QFile::WriteUser);
        }

        const QString where = QDir::toNativeSeparators(dir.absolutePath());
        if (failed.isEmpty()) {
            inform(this, tr("License texts saved"),
                   tr("The full license texts are in %1.").arg(where));
            return;
        }
        warn(this, tr("Some license texts not saved"),
             tr("These could not be written to %1: %2. They can be requested "
                "from legal@vethida.com.")
                 .arg(where, failed.join(QStringLiteral(", "))));
    }

    void showLicensesModal(QWidget* host)
    {
        showContentModal(host, QObject::tr("Open-source licenses"),
                         new LicensesPanel, vth::widgets::PanelDialog::Mode::Cover);
    }
}
