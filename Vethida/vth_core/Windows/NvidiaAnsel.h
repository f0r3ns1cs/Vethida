#pragma once

namespace vth::win::nvidia
{
    // Uses the driver's NvCameraEnable tool, for every driver version in the
    // driver store.
    void toggleAnsel(bool disable);
}
