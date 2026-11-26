#pragma once
#include <mbedtls/ctr_drbg.h>
inline void bootloader_random_enable(){assert(!fake_nonce::active);fake_nonce::active=true;++fake_nonce::enables;}
inline void bootloader_random_disable(){assert(fake_nonce::active);fake_nonce::active=false;++fake_nonce::disables;}
