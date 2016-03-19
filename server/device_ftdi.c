#include <stdio.h>
#include "device_ftdi.h"
#include "firmware.h"


static  DWORD __stdcall
event_ftdi_thread(LPVOID p)
{
    struct _DeviceFtdi *ev = (struct _DeviceFtdi *)p;
    DWORD          available;
    DWORD          closed;
    FT_STATUS      status;
    int            rx_buf_size = sizeof(ev->dev.rx) / sizeof(ev->dev.rx[0]) - 1;

    ResetEvent(ev->dev.thread_event_close);
    while (ev->h) {
        status = FT_GetQueueStatus(ev->h, &available);
        if (FT_SUCCESS(status)) {
            if (available > 0) {
                DWORD readed = 0;
                DWORD current = 0;

                EnterCriticalSection(&ev->dev.rx_sync);
                while (FT_SUCCESS(FT_Read(ev->h, ev->dev.rx + ev->dev.rx_index, available, &current))) {
                    readed += current;
                    ev->dev.rx_index += current;
                    if (readed >= available)
                        break;
                }
                SetEvent(ev->dev.rx_event);
                LeaveCriticalSection(&ev->dev.rx_sync);
            }
        }
        {
            EnterCriticalSection(&ev->dev.tx_sync);
            if (ev->dev.tx_index > 0) {
                DWORD written = 0;

                ev->dev.tx_len = 0;
                status = FT_Write(ev->h, ev->dev.tx, ev->dev.tx_index, &written);
                if (FT_SUCCESS(status)) {
                    if (written < ev->dev.tx_index) {
                        /**TODO: Записано не все, сохраняем хвостик */
                        memcpy(ev->dev.tx, ev->dev.tx + written, ev->dev.tx_index - written);
                    }
                    ev->dev.tx_index -= written;
                    ev->dev.tx_len += written;
                    SetEvent(ev->dev.tx_event);
                }
            }
            LeaveCriticalSection(&ev->dev.tx_sync);
        }
        closed = WaitForSingleObject(ev->dev.thread_event_close, 100);
        if (closed == WAIT_OBJECT_0)
            break;
    }
    fprintf(stdout, "FTDI close thread\n");
    return 0;
}

void
device_ftdi_init(struct _DeviceFtdi *ev)
{
    ev->h = NULL;
    device_init(&ev->dev);
}

static __inline void
device_ftdi_close(struct _DeviceFtdi *ev)
{
    EnterCriticalSection(&ev->dev.rx_sync);
    EnterCriticalSection(&ev->dev.tx_sync);
    __try {
        if (ev->h) {
            FT_Close(ev->h);
            ev->h = NULL;
            fprintf(stdout, "FTDI device closed\n");
        }
        device_close(&ev->dev);
    } __finally {
        LeaveCriticalSection(&ev->dev.tx_sync);
        LeaveCriticalSection(&ev->dev.rx_sync);
    }
}

void
device_ftdi_exit(struct _DeviceFtdi *ev)
{
    device_ftdi_close(ev);
    device_destroy(&ev->dev);
}

void
device_ftdi_loop(struct _DeviceFtdi *ev)
{
    DWORD devices;
    FT_STATUS status;

    status = FT_CreateDeviceInfoList(&devices);
    if (FT_SUCCESS(status) && devices == 1) {
        FT_DEVICE_LIST_INFO_NODE list[1];

        status = FT_GetDeviceInfoList(list, &devices);
        if (FT_SUCCESS(status)) {
            enum _Firmware fw = fw_parse(list[0].Description);
            if ((list[0].Flags & FT_FLAGS_OPENED) != FT_FLAGS_OPENED && fw != NONE) {
                EnterCriticalSection(&ev->dev.rx_sync);
                device_ftdi_close(ev);
                status = FT_Open(0, &ev->h);
                if (FT_SUCCESS(status)) {
                    if (FT_SetBaudRate(ev->h, FT_BAUD_57600) == FT_OK &&
                        FT_SetFlowControl(ev->h, FT_FLOW_NONE, 11, 13) == FT_OK &&
                        FT_SetLatencyTimer(ev->h, 200) == FT_OK &&
                        FT_SetTimeouts(ev->h, 200, 200) == FT_OK &&
                        FT_Purge(ev->h, FT_PURGE_RX | FT_PURGE_TX) == FT_OK) {
                        ev->dev.thread = CreateThread(NULL, 0, event_ftdi_thread, (LPVOID)ev, 0, &ev->dev.thread_id);
                        if (ev->dev.thread == INVALID_HANDLE_VALUE) {
                            fprintf(stderr, "Can't create FTDI rx and tx thread\n");
                            device_ftdi_close(ev);
                        } else {
                            ev->dev.fw = fw;
                            fprintf(stdout, "FTDI device opened\n");
                        }
                    } else {
                        fprintf(stderr, "Can't configuring FTDI device\n");
                        device_ftdi_close(ev);
                    }
                } else {
                    fprintf(stderr, "Can't open FTDI device\n");
                    ev->h = NULL;
                }
                LeaveCriticalSection(&ev->dev.rx_sync);
            }
        }
    } else if (ev->h) {
        device_ftdi_close(ev);
    }
}
