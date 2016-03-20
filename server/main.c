#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <mongoose.h>
#include "db_plugin.h"
#include "device_ftdi.h"
#include "device_hid.h"

int
main(int argc, char *argv[])
{
    struct _Device    df;
    struct _Device    dh;
    struct db        *db;
    DWORD             rx_wait;
    HANDLE            h[8];
    int               i = 0;

    db = db_current();
    device_ftdi_init(&df);
    device_hid_init(&dh);
    h[0] = df.events[DEVICE_STATUS_OPENED];
    h[1] = df.events[DEVICE_STATUS_CLOSED];
    h[2] = df.events[DEVICE_STATUS_READED];
    h[3] = df.events[DEVICE_STATUS_WRITED];

    h[4] = dh.events[DEVICE_STATUS_OPENED];
    h[5] = dh.events[DEVICE_STATUS_CLOSED];
    h[6] = dh.events[DEVICE_STATUS_READED];
    h[7] = dh.events[DEVICE_STATUS_WRITED];
    df.loop = 1;
    dh.loop = 1;
    db_start_q(db);
    while (df.loop && dh.loop) {
        rx_wait = WaitForMultipleObjects(8, h, FALSE, 100);
        switch (rx_wait) {
        case WAIT_OBJECT_0 + DEVICE_STATUS_OPENED:
        {
            fprintf(stdout, "FTDI connected\n");
            break;
        }
        case WAIT_OBJECT_0 + DEVICE_STATUS_CLOSED:
        {
            fprintf(stdout, "FTDI disconnected\n");
            break;
        }
        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_OPENED:
        {
            fprintf(stdout, "HID  connected\n");
            break;
        }
        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_CLOSED:
        {
            fprintf(stdout, "HID  disconnected\n");
            break;
        }


        case WAIT_OBJECT_0 + DEVICE_STATUS_READED: /** Приняли с FTDI */
        {
            device_receive(db, &df);
            break;
        }
        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_READED: /** Приняли с HID */
        {
            device_receive(db, &dh);
            i = 1;
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
    db_close(db);
    device_hid_exit(&dh);
    device_ftdi_exit(&df);    
    system("pause");
    return EXIT_SUCCESS;
}
