#pragma once

#include "../Models/LicenseModels.h"

namespace vth::services
{
    // DPAPI-encrypted persistence for LicenseInfo via SecureStorage.
    class LicenseStore
    {
    public:
        static vth::models::LicenseInfo load();
        static void save(const vth::models::LicenseInfo& info);
        static void deleteLicense();
    };
}
