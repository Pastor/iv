#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <mongoose.h>
#include "device_ftdi.h"
#include "device_hid.h"

int
main(int argc, char *argv[])
{
    struct _DeviceFtdi df;
    struct _Device     dh;
    DWORD             rx_wait;
    HANDLE            h[4];
    int               i = 0;

    device_ftdi_init(&df);
    device_hid_init(&dh);
    h[0] = df.dev.rx_event;
    h[1] = df.dev.tx_event;
    h[2] = dh.rx_event;
    h[3] = dh.tx_event;
    df.dev.loop = 1;
    dh.loop = 1;
    while (df.dev.loop && dh.loop) {
        device_ftdi_loop(&df);
        rx_wait = WaitForMultipleObjects(4, h, FALSE, 100);
        switch (rx_wait) {
        case WAIT_OBJECT_0 + 0: /** Приняли с FTDI */
        {
            device_receive(&df.dev);
            break;
        }
        case WAIT_OBJECT_0 + 1: /** Приняли в FTDI */
        {
            EnterCriticalSection(&df.dev.tx_sync);
            //fprintf(stdout, "FTDI written: %d\n", df.dev.tx_len);
            LeaveCriticalSection(&df.dev.tx_sync);
            break;
        }
        case WAIT_OBJECT_0 + 2: /** Приняли с HID */
        {
            device_receive(&dh);
            i = 1;
            break;
        }
        case WAIT_OBJECT_0 + 3: /** Записали в HID */
        {
            EnterCriticalSection(&dh.tx_sync);
            //fprintf(stdout, "HID  written: %d\n", dh.dev.tx_len);
            LeaveCriticalSection(&dh.tx_sync);
            break;
        }
        default:
            break;
        }
        if (i > 0) {
            uint8_t d[] = {2, 6, 'Q', '2', 'D', '\r', 0, 0};
            
            i = 0;
            fw_crc_create(&d[2], 4, &d[6]);
            device_write(&dh, d, sizeof(d));
        }
    }
    device_hid_exit(&dh);
    device_ftdi_exit(&df);    
    system("pause");
    return EXIT_SUCCESS;
}
