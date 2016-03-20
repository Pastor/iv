#include <stdio.h>
#include <ftd2xx.h>
#include "device_ftdi.h"
#include "firmware.h"


static  DWORD __stdcall
ftdi_thread(LPVOID p)
{
    struct _Device          *dev = (struct _Device *)p;
    DWORD                    devices = 0;
    FT_STATUS                status;
    FT_DEVICE_LIST_INFO_NODE list[1];
    FT_HANDLE                h;

    while (devices <= 0) {
        status = FT_CreateDeviceInfoList(&devices);
        if (FT_SUCCESS(status) && devices > 0) {
            status = FT_GetDeviceInfoList(list, &devices);
            if (FT_SUCCESS(status)) {
                dev->fw = fw_parse(list[0].Description);
                if ((list[0].Flags & FT_FLAGS_OPENED) != FT_FLAGS_OPENED && dev->fw != NONE) {
                    status = FT_Open(0, &h);
                    if (FT_SUCCESS(status)) {
                        if (FT_SetBaudRate(h, FT_BAUD_57600) == FT_OK &&
                            FT_SetFlowControl(h, FT_FLOW_NONE, 11, 13) == FT_OK &&
                            FT_SetLatencyTimer(h, 200) == FT_OK &&
                            FT_SetTimeouts(h, 200, 200) == FT_OK &&
                            FT_Purge(h, FT_PURGE_RX | FT_PURGE_TX) == FT_OK) {
                            DWORD  available = 0;


                            DEVICE_SET_OPENED(dev);
                            while (dev->fw != NONE) {
                                status = FT_GetQueueStatus(h, &available);
                                if (FT_SUCCESS(status)) {
                                    if (available > 0) {
                                        DWORD readed = 0;
                                        DWORD current = 0;

                                        EnterCriticalSection(&dev->rx_sync);
                                        while (FT_SUCCESS(FT_Read(h, dev->rx + dev->rx_index, available, &current))) {
                                            readed += current;
                                            dev->rx_index += current;
                                            if (readed >= available)
                                                break;
                                        }
                                        LeaveCriticalSection(&dev->rx_sync);
                                    } else {
                                        if (dev->rx_index > 0) {
                                            EnterCriticalSection(&dev->rx_sync);
                                            DEVICE_SET_READED(dev);
                                            LeaveCriticalSection(&dev->rx_sync);
                                        }
                                        EnterCriticalSection(&dev->tx_sync);
                                        if (dev->tx_index > 0) {
                                            DWORD written = 0;
                                            
                                            status = FT_Write(h, dev->tx, dev->tx_index, &written);
                                            if (FT_SUCCESS(status)) {
                                                if (written < dev->tx_index) {
                                                    /**TODO: Записано не все, сохраняем хвостик */
                                                    memcpy(dev->tx, dev->tx + written, dev->tx_index - written);
                                                }
                                                dev->tx_index -= written;
                                                dev->tx_len += written;
                                                DEVICE_SET_WRITTEN(dev);
                                            }
                                        }
                                        LeaveCriticalSection(&dev->tx_sync);
                                        Sleep(100);
                                    }
                                } else {
                                    dev->fw = NONE;
                                }
                            }
                        }
                        FT_Close(h);
                    }
                }
            }
        }
        Sleep(100);
        devices = 0;
    }
    return 0;
}

int
device_ftdi_init(struct _Device *dev)
{
    device_init(dev);
    dev->thread = CreateThread(NULL, 0, ftdi_thread, (LPVOID)dev, 0, &dev->thread_id);
    return dev->thread != INVALID_HANDLE_VALUE;
}

void
device_ftdi_exit(struct _Device *dev)
{
    DWORD                    devices;
    FT_DEVICE_LIST_INFO_NODE list[1];
    FT_HANDLE                h;
    
    if (FT_CreateDeviceInfoList(&devices) == FT_OK && 
        devices > 0 && 
        FT_GetDeviceInfoList(list, &devices) == FT_OK && 
        fw_parse(list[0].Description) != NONE &&
        (list[0].Flags & FT_FLAGS_OPENED) == FT_FLAGS_OPENED &&
        FT_Open(0, &h) == FT_OK) {
        
        FT_Close(h);
    }
    dev->loop = 0;
    device_destroy(dev);
}
