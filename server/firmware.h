#pragma once
#include <stdint.h>

#define FW60_PACKET_SIZE  21

enum _Firmware
{
    FW42,
    FW60,
    NONE
};

enum _Firmware fw_parse(const char *fw);
enum _Firmware fw_parse_w(const wchar_t *fw);
int fw_crc_check(uint8_t *data, uint32_t *next);
void fw_crc_create(uint8_t *data, int len, uint8_t crc[2]);
int fw_is_complete(uint8_t *data, int len);
uint8_t byte_from_hex(uint8_t hex[2]);
uint8_t from_hex(uint8_t ch);
