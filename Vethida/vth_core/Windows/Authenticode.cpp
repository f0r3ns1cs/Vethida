#include "Authenticode.h"

#include <QDir>

#include <windows.h>
#include <softpub.h>
#include <wintrust.h>

#include <iterator>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

namespace vth::win::authenticode
{
    QString signer(const QString& file) {
        const std::wstring path = QDir::toNativeSeparators(file).toStdWString();
        WINTRUST_FILE_INFO fi{};
        fi.cbStruct      = sizeof(fi);
        fi.pcwszFilePath = path.c_str();
        WINTRUST_DATA td{};
        td.cbStruct            = sizeof(td);
        td.dwUIChoice          = WTD_UI_NONE;
        td.fdwRevocationChecks = WTD_REVOKE_NONE;
        td.dwUnionChoice       = WTD_CHOICE_FILE;
        td.dwStateAction       = WTD_STATEACTION_VERIFY;
        td.pFile               = &fi;
        GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
        const LONG status = WinVerifyTrust(nullptr, &policy, &td);

        QString name;
        if (status == ERROR_SUCCESS) {
            if (auto* pd = WTHelperProvDataFromStateData(td.hWVTStateData)) {
                if (auto* sgnr = WTHelperGetProvSignerFromChain(pd, 0, FALSE, 0)) {
                    if (sgnr->pChainContext && sgnr->pChainContext->cChain > 0
                        && sgnr->pChainContext->rgpChain[0]->cElement > 0) {
                        auto* ctx = sgnr->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;
                        wchar_t buf[256]{};
                        if (CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr,
                                               buf, static_cast<DWORD>(std::size(buf))))
                            name = QString::fromWCharArray(buf);
                    }
                }
            }
        }
        td.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(nullptr, &policy, &td);
        return name;
    }

    bool signedBy(const QString& file, const QString& expected) {
        const QString name = signer(file);
        return !name.isEmpty() && !expected.isEmpty()
            && name.contains(expected, Qt::CaseInsensitive);
    }
}
