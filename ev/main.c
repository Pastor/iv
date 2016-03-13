#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ftd2xx.h>
#include <mongoose.h>

#define FW60_PACKET_SIZE  21

enum _Firmware
{
    FW42,
    FW60,
    NONE
};

enum _Firmware
fw_parse(const char *fw)
{
    if (fw == NULL)
        return NONE;
    if (strncmp(fw, "I-VOTE", 6))
        return NONE;
    if (!strncmp(fw, "I-VOTE ver 4.2", 14))
        return FW42;
    if (!strncmp(fw, "I-VOTE ver 6.0", 14))
        return FW60;
    return NONE;
}

static uint8_t __inline
from_hex(uint8_t ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return ch - 'a' + 10;
}


static uint8_t __inline
byte_from_hex(uint8_t hex[2])
{
    return (from_hex(hex[0]) * 16) + from_hex(hex[1]);
}

int
fw_crc_chack(uint8_t *data, uint32_t *next)
{
    uint8_t ps = 0;
    uint8_t sum = 0;
    uint8_t that = 0;
    uint8_t it;
    uint8_t crc[2];

    ps = data[1];
    sum = ps;
    for (it = 2; it < ps; ++it) {
        sum += data[it];
    }
    crc[0] = data[ps];
    crc[1] = data[ps + 1];
    (*next) = ps + 2;
    that = byte_from_hex(crc);
    //fprintf(stdout, "Sum    : %d = %d\n", sum, that);
    //fprintf(stdout, "Sum    : %c%c\n", crc[0], crc[1]);
    return sum == byte_from_hex(crc);
}

static int __inline
fw_is_complete(uint8_t *data, int len)
{
    return (len >= 3) && (data[1] <= len - 2);
}

struct _EventFtdi
{
    FT_HANDLE          dev;
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
    uint32_t           tx_buf_size;
    enum _Firmware     fw;
    int                loop;
};

static  DWORD __stdcall
event_ftdi_thread(LPVOID p)
{
    struct _EventFtdi *ev = (struct _EventFtdi *)p;
    DWORD          available;
    DWORD          closed;
    FT_STATUS      status;
    int            rx_buf_size = sizeof(ev->rx) / sizeof(ev->rx[0]) - 1;

    ResetEvent(ev->thread_event_close);
    while (ev->dev) {
        status = FT_GetQueueStatus(ev->dev, &available);
        if (FT_SUCCESS(status)) {
            if (available > 0) {
                DWORD readed = 0;
                DWORD current = 0;

                EnterCriticalSection(&ev->rx_sync);
                while (FT_SUCCESS(FT_Read(ev->dev, ev->rx + ev->rx_index, available, &current))) {
                    readed += current;
                    ev->rx_index += current;
                    if (readed >= available)
                        break;
                }
                SetEvent(ev->rx_event);
                LeaveCriticalSection(&ev->rx_sync);
            }
        }
        {
            EnterCriticalSection(&ev->tx_sync);
            if (ev->tx_index > 0) {
                DWORD written = 0;

                status = FT_Write(ev->dev, ev->tx, ev->tx_index, &written);
                if (FT_SUCCESS(status)) {
                    if (written < ev->tx_index) {
                        /**TODO: Записано не все, сохраняем хвостик */
                        memcpy(ev->tx, ev->tx + written, ev->tx_index - written);
                    }
                    ev->tx_index -= written;
                    SetEvent(ev->tx_event);
                }
            }
            LeaveCriticalSection(&ev->tx_sync);
        }
        closed = WaitForSingleObject(ev->thread_event_close, 100);
        if (closed == WAIT_OBJECT_0)
            break;
    }
    fprintf(stdout, "Close thread\n");
    return 0;
}

static void
event_ftdi_init(struct _EventFtdi *ev)
{
    InitializeCriticalSectionAndSpinCount(&ev->rx_sync, 0x00000400);
    InitializeCriticalSectionAndSpinCount(&ev->tx_sync, 0x00000400);
    ev->dev = NULL;
    ev->rx_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ev->rx_buf_size = sizeof(ev->rx) / sizeof(ev->rx[0]);
    ev->rx_index = 0;
    ev->tx_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ev->tx_buf_size = sizeof(ev->tx) / sizeof(ev->tx[0]);
    ev->tx_index = 0;
    ev->thread = INVALID_HANDLE_VALUE;
    ev->thread_event_close = CreateEvent(NULL, FALSE, FALSE, NULL);
}

static __inline void
event_ftdi_close_device(struct _EventFtdi *ev)
{
    EnterCriticalSection(&ev->rx_sync);
    EnterCriticalSection(&ev->tx_sync);
    __try {
        if (ev->dev) {
            FT_Close(ev->dev);
            ev->dev = NULL;
            fprintf(stdout, "Device closed\n");
        }
        SetEvent(ev->thread_event_close);
        Sleep(100);
        if (ev->thread != NULL && ev->thread != INVALID_HANDLE_VALUE) {
            TerminateThread(ev->thread, 0);
        }
        ev->thread = INVALID_HANDLE_VALUE;
        ev->thread_id = 0;
        ev->fw = NONE;        
    } __finally {
        LeaveCriticalSection(&ev->tx_sync);
        LeaveCriticalSection(&ev->rx_sync);        
    }
}

static void
event_ftdi_exit(struct _EventFtdi *ev)
{
    event_ftdi_close_device(ev);
    DeleteCriticalSection(&ev->rx_sync);
    DeleteCriticalSection(&ev->tx_sync);
    CloseHandle(ev->rx_event);
    CloseHandle(ev->tx_event);
    CloseHandle(ev->thread_event_close);
}

static void
event_ftdi_loop(struct _EventFtdi *ev)
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
                EnterCriticalSection(&ev->rx_sync);
                event_ftdi_close_device(ev);
                status = FT_Open(0, &ev->dev);
                if (FT_SUCCESS(status)) {
                    if (FT_SetBaudRate(ev->dev, FT_BAUD_57600) == FT_OK &&
                        FT_SetFlowControl(ev->dev, FT_FLOW_NONE, 11, 13) == FT_OK &&
                        FT_SetLatencyTimer(ev->dev, 200) == FT_OK &&
                        FT_SetTimeouts(ev->dev, 200, 200) == FT_OK &&
                        FT_Purge(ev->dev, FT_PURGE_RX | FT_PURGE_TX) == FT_OK) {
                        ev->thread = CreateThread(NULL, 0, event_ftdi_thread, (LPVOID)ev, 0, &ev->thread_id);
                        if (ev->thread == INVALID_HANDLE_VALUE) {
                            fprintf(stderr, "Can't create rx and tx thread\n");
                            event_ftdi_close_device(ev);
                        } else {
                            ev->fw = fw;
                            fprintf(stdout, "Device opened\n");
                        }
                    } else {
                        fprintf(stderr, "Can't configuring FTDI device\n");
                        event_ftdi_close_device(ev);
                    }
                } else {
                    fprintf(stderr, "Can't open FTDI device\n");
                    ev->dev = NULL;
                }
                LeaveCriticalSection(&ev->rx_sync);
            }
        }
    } else if (ev->dev) {
        event_ftdi_close_device(ev);
    }
}

static int
event_ftdi_write(struct _EventFtdi *ev, uint8_t *data, int len)
{
    int ret;

    EnterCriticalSection(&ev->rx_sync);
    if ( ev->tx_index + len < ev->tx_buf_size ) {
        memcpy(ev->tx + ev->tx_index, data, len);
        ev->tx_index += len;
        ret = len;
    } else {
        ret = -1;
    }
    LeaveCriticalSection(&ev->tx_sync);
    return ret;
}


int
main(int argc, char *argv[])
{
    struct _EventFtdi evf;
    DWORD             rx_wait;
    HANDLE            h[4];

    event_ftdi_init(&evf);
    h[0] = evf.rx_event;
    h[1] = evf.tx_event;
    h[2] = INVALID_HANDLE_VALUE;
    h[3] = INVALID_HANDLE_VALUE;
    evf.loop = 1;
    while (evf.loop) {
        event_ftdi_loop(&evf);
        rx_wait = WaitForMultipleObjects(2, h, FALSE, 100);
        switch (rx_wait) {
        case WAIT_OBJECT_0 + 0: /** Приняли с FTDI */
        {
            EnterCriticalSection(&evf.rx_sync);
            evf.rx[evf.rx_index] = 0;
            //fprintf(stdout, "Receive: %s\n", (char *)evf.rx);
            if (evf.fw == FW60) {
                uint32_t next = 0;
                uint32_t it   = 0;

                for (it = 0; it < evf.rx_index; it += next) {
                    next = 0;
                    if (fw_is_complete(evf.rx + it, evf.rx_index - it)) {
                        if (fw_crc_chack(evf.rx + it, &next)) {
                            uint8_t  *start = evf.rx + it;
                            uint8_t   size  = next;
                            uint8_t   dev_group   = byte_from_hex(&start[2]);
                            uint8_t   dev_index   = byte_from_hex(&start[4]);
                            char      dev_battery_s[3] = {start[7], start[8], 0 };
                            char      dev_timeout_s[5] = {start[9], start[10], start[11], start[12], 0};
                            char      dev_enter_s[5] = { start[14], start[15], start[16], start[17], 0 };
                            uint8_t   dev_battery = (uint8_t)strtol(dev_battery_s, NULL, 10);
                            uint16_t  dev_timeout = (uint16_t)strtol(dev_timeout_s, NULL, 10);

                            fprintf(stdout, "G: %03d, I: %03d, B: %02d, T: %04d, E: %04s\n", dev_group, dev_index, dev_battery, dev_timeout, dev_enter_s);
                        } else {
                            fprintf(stdout, "Damaged: %d, Size: %d\n", next, evf.rx_index);
                        }
                    } else if (evf.rx[0] != 2 || (evf.rx_index > 1 && evf.rx[1] == 0)) {
                        evf.rx_index = 0;
                        fprintf(stdout, "Illegal packet. Reset buffer\n");
                        break;
                    } else {
                        //fprintf(stdout, "Incomplete packet\n");
                        break;
                    }
                }
                if (it + next < evf.rx_index) {
                    memcpy(evf.rx, evf.rx + it + next, evf.rx_index - it - next);
                    evf.rx_index = evf.rx_index - it - next;
                } else {
                    evf.rx_index = 0;
                }
            }
            LeaveCriticalSection(&evf.rx_sync);
            break;
        }
        case WAIT_OBJECT_0 + 1: /** Приняли в FTDI */
        {
            EnterCriticalSection(&evf.tx_sync);
            fprintf(stdout, "Written: %d\n", evf.tx_index);
            LeaveCriticalSection(&evf.tx_sync);
            break;
        }
        case WAIT_OBJECT_0 + 2: /** Приняли с HID */
        {
            break;
        }
        case WAIT_OBJECT_0 + 3: /** Записали в HID */
        {
            break;
        }
        default:
            break;
        }
    }
    event_ftdi_exit(&evf);
    system("pause");
    return EXIT_SUCCESS;
}
