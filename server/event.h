#pragma once

struct _Device;
struct db;
struct event;

enum Event {
    DEVICE_CONNECT           = 0x00000002,
    DEVICE_DISCONNECT        = 0x00000004,
    DEVICE_RECVEIVE          = 0x00000008,
    DEVICE_SENT              = 0x00000010,
    DEVICE_HID               = 0x00000020,
    DEVICE_FTDI              = 0x00000040,
    FIRMWARE_42              = 0x00000080,
    FIRMWARE_60              = 0x00000100,
    CLOSE                    = 0x00000000,
    START                    = 0x00000001
};

struct event * event_start(struct _Device *df, struct _Device *dh, struct db *db, struct mg_connection *nc);
void event_stop(struct event **event);
