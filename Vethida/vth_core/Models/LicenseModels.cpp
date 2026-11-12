#include "LicenseModels.h"

#include <QtCore/QRegularExpression>

namespace vth::models
{
    QString licenseErrorLabel(LicenseOpError error)
    {
        switch (error) {
        case LicenseOpError::None:
            return {};
        case LicenseOpError::Network:
            return QObject::tr("Could not reach the licensing server. Check the "
                               "connection and try again.");
        case LicenseOpError::RateLimited:
            return QObject::tr("Too many attempts. Wait a minute and try again.");
        case LicenseOpError::KeyFormat:
            return QObject::tr("That is not a license key. Copy it from the "
                               "email we sent after checkout and paste it here.");
        case LicenseOpError::KeyUnknown:
            return QObject::tr("No license matches that key. Check it for typos.");
        case LicenseOpError::WrongProduct:
            return QObject::tr("That key is for a different product, not Vethida.");
        case LicenseOpError::KeyRevoked:
            return QObject::tr("This license was disabled, usually after a refund. "
                               "Contact support if that is unexpected.");
        case LicenseOpError::KeyExpired:
            return QObject::tr("This license has expired. Renew the subscription "
                               "to use it again.");
        case LicenseOpError::MachineLimitReached:
            return QObject::tr("This license is already in use on as many PCs as it "
                               "allows. Deactivate it on another PC in Vethida's "
                               "Account page, then try again.");
        case LicenseOpError::Server:
            return QObject::tr("The licensing server had a problem. Try again in a "
                               "few minutes.");
        case LicenseOpError::Unknown:
            break;
        }
        return QObject::tr("Something went wrong. Try again, and contact support if "
                           "it keeps happening.");
    }

    namespace
    {
        const QString& alphabet()
        {
            static const QString a = QStringLiteral("0123456789ABCDEFGHJKMNPQRSTVWXYZ");
            return a;
        }

        QChar luhnMod32(const QString& input)
        {
            int factor = 2;
            int sum = 0;
            for (int i = input.size() - 1; i >= 0; --i) {
                int addend = alphabet().indexOf(input.at(i)) * factor;
                factor = factor == 2 ? 1 : 2;
                addend = addend / 32 + addend % 32;
                sum += addend;
            }
            return alphabet().at((32 - sum % 32) % 32);
        }
    }

    bool LicenseValidation::isValidKeyFormat(const QString& key)
    {
        if (key.size() != 29)
            return false;
        for (int g = 0; g < 5; ++g) {
            const QString group = key.mid(g * 6, 5);
            if (g < 4 && key.at(g * 6 + 5) != QLatin1Char('-'))
                return false;
            for (const QChar c : group)
                if (!alphabet().contains(c))
                    return false;
            if (luhnMod32(group.left(4)) != group.at(4))
                return false;
        }
        return true;
    }

    QString LicenseValidation::formatKey(const QString& raw)
    {
        QString compact;
        for (QChar c : raw.toUpper()) {
            if (c == QLatin1Char('I') || c == QLatin1Char('L')) c = QLatin1Char('1');
            else if (c == QLatin1Char('O'))                      c = QLatin1Char('0');
            else if (c == QLatin1Char('U'))                      c = QLatin1Char('V');
            if (c.isLetterOrNumber())
                compact.append(c);
        }
        if (compact.size() != 25)
            return compact;
        QStringList groups;
        for (int i = 0; i < 25; i += 5)
            groups << compact.mid(i, 5);
        return groups.join(QLatin1Char('-'));
    }

    QString LicenseValidation::maskKey(const QString& key)
    {
        if (key.size() < 5)
            return key;
        return key.left(5) + QStringLiteral("-*****-*****-*****-*****");
    }
}
