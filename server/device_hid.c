#include <stdio.h>
#include "device_hid.h"
#include "hid.h"
#include "firmware.h"

wchar_t *rawhid_product(int num);

static  DWORD __stdcall
hid_thread(LPVOID p)
{
    struct _Device *dev = (struct _Device *)p;
    int            r = -1;
    uint8_t        buf[64];

    while (r <= 0) {
        r = rawhid_open(1, 0x10C4, 0x8468, 0xFF00, 0x0001);
        if (r > 0) {
            int  num = 0;
            dev->fw = fw_parse_w( rawhid_product(0) );
            /** Connect device */
            while (dev->fw != NONE) {
                memset(buf, 0, sizeof(buf));
                num = rawhid_recv(0, buf, 64, 220);
                if (num < 0) {
                    printf("\nerror reading, device went offline\n");
                    /** Disconnect device*/
                    r = -1;
                    dev->fw = NONE;
                } else if (num > 0 && buf[0] > 0) {
                    EnterCriticalSection(&dev->rx_sync);
                    memcpy(dev->rx + dev->rx_index, buf + 1, buf[0]);
                    dev->rx_index += buf[0];
                    LeaveCriticalSection(&dev->rx_sync);
                } else {
                    if (dev->rx_index > 0) {
                        EnterCriticalSection(&dev->rx_sync);
                        SetEvent(dev->rx_event);
                        LeaveCriticalSection(&dev->rx_sync);
                    }
                    EnterCriticalSection(&dev->tx_sync);
                    if (dev->tx_index > 0) {
                        /** tx_index <= 64 */
                        dev->tx_len = rawhid_send(0, dev->tx, 64, 100);
                        if (dev->tx_len > 0) {
                            dev->tx_index = 0;
                            SetEvent(dev->tx_event);
                        } else {
                            /**FIXME: Error */
                        }
                    }
                    LeaveCriticalSection(&dev->tx_sync);
                    Sleep(100);
                }
            }
            rawhid_close(0);
        } else {
            /**Skip tx if device closed */
            EnterCriticalSection(&dev->tx_sync);
            dev->tx_index = 0;
            LeaveCriticalSection(&dev->tx_sync);
        }
        Sleep(100);
    }
    return 0;
}

int
device_hid_init(struct _Device *dev)
{
    device_init(dev);
    dev->thread = CreateThread(NULL, 0, hid_thread, (LPVOID)dev, 0, &dev->thread_id);
    return dev->thread != INVALID_HANDLE_VALUE;
}

void
device_hid_exit(struct _Device *dev)
{
    rawhid_close(0);
    device_destroy(dev);
}
