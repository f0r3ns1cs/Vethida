#pragma once

#if defined(VETHIDA_DEV_BUILD) && defined(NDEBUG)
#  error "VETHIDA_DEV_BUILD must never be defined in a release configuration."
#endif
