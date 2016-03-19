#pragma once
#include <stdint.h>
#include <ftd2xx.h>
#include "device.h"

struct _DeviceFtdi
{
    struct _Device     dev;
    FT_HANDLE          h;
};


void device_ftdi_init(struct _DeviceFtdi *ev);
void device_ftdi_exit(struct _DeviceFtdi *ev);
void device_ftdi_loop(struct _DeviceFtdi *ev);