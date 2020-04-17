#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

inline uint16_t __bswap_16(uint16_t val) {
    return (val << 8) | (val >> 8 );
}

inline uint32_t __bswap_32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

inline uint64_t __bswap_64(uint64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

#ifdef __cplusplus
}
#endif
