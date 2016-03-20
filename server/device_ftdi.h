#pragma once
#include "device.h"

int  device_ftdi_init(struct _Device *dev);
void device_ftdi_exit(struct _Device *dev);