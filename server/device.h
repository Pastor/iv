#pragma once
#include <stdint.h>
#include <Windows.h>
#include "firmware.h"


struct _Device
{
    CRITICAL_SECTION   rx_sync;
    CRITICAL_SECTION   tx_sync;
    HANDLE             thread;
    HANDLE             thread_event_close;
    DWORD              thread_id;
    HANDLE             rx_event;
    uint8_t            rx[1024 * FW60_PACKET_SIZE];
    uint32_t           rx_index;
    uint32_t           rx_buf_size;
    HANDLE             tx_event;
    uint8_t            tx[256];
    uint32_t           tx_index;
    uint32_t           tx_len;
    uint32_t           tx_buf_size;
    enum _Firmware     fw;
    int                loop;
};

void device_init(struct _Device *dev);
void device_close(struct _Device *dev);
int  device_write(struct _Device *dev, uint8_t *data, int len);
void device_destroy(struct _Device *dev);
void device_receive(struct _Device *dev);

void device_fw_read60(struct _Device * dev);
void device_fw_read42(struct _Device * dev);

