#include <stddef.h>
#include <stdint.h>

uint32_t sp_crc32_fallback(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t) data[i];
        for (int b = 0; b < 8; b++) {
            uint32_t mask = -(int32_t) (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

uint32_t sp_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
#if defined(__x86_64__) || defined(__i386__)
    uint8_t* p = (uint8_t*) data;
#if defined(__SSE4_2__)
    for (size_t i = 0; i < len; i++) {
        unsigned int value = p[i];
        asm volatile("crc32b %1, %0" : "+r"(crc) : "r"(value));
    }
    return crc ^ 0xFFFFFFFFu;
#endif
#endif
    return sp_crc32_fallback(data, len);
}
