#include "crc32.hpp"

static uint32_t table[256];
static bool table_init = false;

static void init_table() {
    for (uint32_t i=0; i<256; i++) {
        uint32_t c = i;
        for (int j=0;j<8;j++)
            c = c & 1 ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        table[i] = c;
    }
    table_init = true;
}

uint32_t crc32(const std::string& data) {
    if (!table_init) init_table();
    uint32_t c = 0xFFFFFFFF;
    for (unsigned char ch : data)
        c = table[(c ^ ch) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFF;
}
