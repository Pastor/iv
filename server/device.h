#pragma once
#include <stdint.h>
#include <Windows.h>
#include "firmware.h"

enum {
    DeviceUnknwon = 0x0000,
    DeviceOpened  = 0x0001,
    DeviceClosed  = 0x0002,
    DeviceReaded  = 0x0004,
    DeviceWritten = 0x0008
};

struct db;

struct _Device
{
    CRITICAL_SECTION   rx_sync;
    CRITICAL_SECTION   tx_sync;
    HANDLE             thread;
    DWORD              thread_id;
    HANDLE             events[4];
    uint8_t            rx[1024 * FW60_PACKET_SIZE];
    uint32_t           rx_index;
    uint8_t            tx[256];
    uint32_t           tx_index;
    uint32_t           tx_len;
    enum _Firmware     fw;
    int                loop;
};

#define DEVICE_STATUS_OPENED  0
#define DEVICE_STATUS_CLOSED  1
#define DEVICE_STATUS_READED  2
#define DEVICE_STATUS_WRITED  3

#define DEVICE_SET_READED(dev)    SetEvent((dev)->events[DEVICE_STATUS_READED])
#define DEVICE_SET_WRITTEN(dev)   SetEvent((dev)->events[DEVICE_STATUS_WRITED])
#define DEVICE_SET_OPENED(dev)    SetEvent((dev)->events[DEVICE_STATUS_OPENED])
#define DEVICE_SET_CLOSED(dev)    SetEvent((dev)->events[DEVICE_STATUS_CLOSED])
#define DEVICE_RESET_STATUS(dev)  

void device_init(struct _Device *dev);
void device_close(struct _Device *dev);
int  device_write(struct _Device *dev, uint8_t *data, int len);
void device_destroy(struct _Device *dev);
int  device_receive(struct db *db, struct _Device *dev);

void device_fw_read60(struct db *db, struct _Device * dev, int *ret);
void device_fw_read42(struct db *db, struct _Device * dev, int *ret);

