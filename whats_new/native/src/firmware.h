#pragma once
#include <stdint.h>

#define FW60_PACKET_SIZE  21

enum _Firmware
{
    FW45,
    FW60,
    NONE
};

enum _Firmware fw_parse(const char *fw);
enum _Firmware fw_parse_w(const wchar_t *fw);
int fw_crc_check(uint8_t *data, uint32_t *next);
void fw_crc_create(uint8_t *data, int len, uint8_t crc[2]);
int fw_is_complete(uint8_t *data, int len);

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
