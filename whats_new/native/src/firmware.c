#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "firmware.h"

enum _Firmware
fw_parse(const char *fw)
{
    if (fw == NULL)
        return NONE;
    if (strncmp(fw, "I-VOTE", 6))
        return NONE;
    if (!strncmp(fw, "I-VOTE ver 4.2", 14))
        return FW42;
    if (!strncmp(fw, "I-VOTE ver 6.0", 14))
        return FW60;
    return NONE;
}

enum _Firmware
fw_parse_w(const wchar_t *fw)
{
    if (fw == NULL)
        return NONE;
    if (wcsncmp(fw, L"I-VOTE", 6))
        return NONE;
    if (!wcsncmp(fw, L"I-VOTE ver 4.2", 14))
        return FW42;
    if (!wcsncmp(fw, L"I-VOTE ver 6.0", 14))
        return FW60;
    return NONE;
}

uint8_t __inline
from_hex(uint8_t ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return ch - 'a' + 10;
}

uint8_t __inline
byte_from_hex(uint8_t hex[2])
{
    return (from_hex(hex[0]) * 16) + from_hex(hex[1]);
}

int
fw_crc_check(uint8_t *data, uint32_t *next)
{
    uint8_t ps = 0;
    uint8_t sum = 0;
    uint8_t that = 0;
    uint8_t it;
    uint8_t crc[2];

    ps = data[1];
    sum = ps;
    for (it = 2; it < ps; ++it) {
        sum += data[it];
    }
    crc[0] = data[ps];
    crc[1] = data[ps + 1];
    (*next) = ps + 2;
    that = byte_from_hex(crc);
    return sum == byte_from_hex(crc);
}

static const uint8_t hex_chars[] = "0123456789ABCDEF";

void 
fw_crc_create(uint8_t* data, int len, uint8_t crc[2])
{
    uint8_t sum = 0;
    int i;


    for (i = 0; i < len; ++i) {
        sum += data[i];
    }
    crc[0] = hex_chars[(sum & 0xf0) >> 4];
    crc[1] = hex_chars[(sum & 0x0f) >> 0];
}

int
fw_is_complete(uint8_t *data, int len)
{
    return (len >= 3) && (data[1] <= len - 2);
}

