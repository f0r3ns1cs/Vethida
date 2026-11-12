#pragma once

#if defined(VETHIDA_DEV_BUILD) && defined(NDEBUG)
#  error "VETHIDA_DEV_BUILD must never be defined in a release configuration."
#endif

// VETHIDA_LICENSING: without it every plan is unlocked and nothing talks to a
// licensing server. Build with /p:VethidaLicensing=true (common.props) to get license
// keys, the Account page and tier gating back.
