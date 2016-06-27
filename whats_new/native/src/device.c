#include <stdio.h>
#include "device.h"

void
device_init(struct _Device* dev)
{
    InitializeCriticalSectionAndSpinCount(&dev->rx_sync, 0x00000400);
    InitializeCriticalSectionAndSpinCount(&dev->tx_sync, 0x00000400);
    dev->events[DEVICE_STATUS_OPENED] = CreateEvent(NULL, FALSE, FALSE, NULL);
    dev->events[DEVICE_STATUS_CLOSED] = CreateEvent(NULL, FALSE, FALSE, NULL);
    dev->events[DEVICE_STATUS_READED] = CreateEvent(NULL, FALSE, FALSE, NULL);
    dev->events[DEVICE_STATUS_WRITED] = CreateEvent(NULL, FALSE, FALSE, NULL);
    dev->rx_index = 0;
    dev->tx_index = 0;
    dev->tx_len = 0;
    dev->thread = INVALID_HANDLE_VALUE;
}

void
device_close(struct _Device* dev)
{
    EnterCriticalSection(&dev->rx_sync);
    EnterCriticalSection(&dev->tx_sync);
    __try {
        if (dev->thread != NULL && dev->thread != INVALID_HANDLE_VALUE) {
            TerminateThread(dev->thread, 0);
        }
        dev->thread = INVALID_HANDLE_VALUE;
        dev->thread_id = 0;
        dev->fw = NONE;
    } __finally {
        LeaveCriticalSection(&dev->tx_sync);
        LeaveCriticalSection(&dev->rx_sync);
    }
}

int
device_write(struct _Device *dev, uint8_t *data, int len)
{
    int ret;

    EnterCriticalSection(&dev->tx_sync);
    if (dev->tx_index + len < sizeof(dev->tx) / sizeof(dev->tx[0])) {
        memcpy(dev->tx + dev->tx_index, data, len);
        dev->tx_index += len;
        ret = len;
    } else {
        ret = -1;
    }
    LeaveCriticalSection(&dev->tx_sync);
    return ret;
}

void
device_destroy(struct _Device* dev)
{
    DeleteCriticalSection(&dev->rx_sync);
    DeleteCriticalSection(&dev->tx_sync);
    CloseHandle(dev->events[DEVICE_STATUS_OPENED]);
    CloseHandle(dev->events[DEVICE_STATUS_CLOSED]);
    CloseHandle(dev->events[DEVICE_STATUS_READED]);
    CloseHandle(dev->events[DEVICE_STATUS_WRITED]);
}

int
device_receive(receive_callback cb, void *userdata, struct _Device* dev)
{
    int ret = 0;
    EnterCriticalSection(&dev->rx_sync);
    dev->rx[dev->rx_index] = 0;
    //fprintf(stdout, "Receive: %s\n", (char *)evf.rx);
    if (dev->fw == FW60) {
        device_fw_read60(cb, userdata, dev, &ret);
    } else if (dev->fw == FW45) {
        device_fw_read42(cb, userdata, dev, &ret);
    }
    LeaveCriticalSection(&dev->rx_sync);
    return ret;
}

void
device_fw_read60(receive_callback cb, void *userdata, struct _Device* dev, int *ret)
{
    struct _Result result[100];
    uint32_t next = 0;
    uint32_t it = 0;

    (*ret) = 0;
    for (it = 0; it < dev->rx_index; it += next) {
        next = 0;
        if (fw_is_complete(dev->rx + it, dev->rx_index - it)) {
            if (fw_crc_check(dev->rx + it, &next)) {
                uint8_t  *start = dev->rx + it;
                uint8_t   size = next;
                uint8_t   dev_group = byte_from_hex(&start[2]);
                uint8_t   dev_index = byte_from_hex(&start[4]);
                char      dev_battery_s[3] = { start[7], start[8], 0 };
                char      dev_timeout_s[5] = { start[9], start[10], start[11], start[12], 0 };
                char      dev_enter_s[5] = { start[14], start[15], start[16], start[17], 0 };
                uint8_t   dev_battery = (uint8_t)strtol(dev_battery_s, NULL, 10);
                uint16_t  dev_timeout = (uint16_t)strtol(dev_timeout_s, NULL, 10);

                fprintf(stdout, "G: %03d, I: %03d, B: %02d, T: %04d, E: %04s\n", dev_group, dev_index, dev_battery, dev_timeout, dev_enter_s);
                result[(*ret)].group = dev_group;
                result[(*ret)].index = dev_index;
                result[(*ret)].battery = dev_battery;
                result[(*ret)].timeout = dev_timeout;
                result[(*ret)].enter_s[0] = dev_enter_s[0];
                result[(*ret)].enter_s[1] = dev_enter_s[1];
                result[(*ret)].enter_s[2] = dev_enter_s[2];
                result[(*ret)].enter_s[3] = dev_enter_s[3];
                result[(*ret)].enter_s[4] = dev_enter_s[4];
                (*ret)++;
            } else {
                fprintf(stdout, "Damaged: %d, Size: %d\n", next, dev->rx_index);
            }
        } else if (dev->rx[0] != 2 || (dev->rx_index > 1 && dev->rx[1] == 0)) {
            dev->rx_index = 0;
            fprintf(stdout, "Illegal packet. Reset buffer\n");
            break;
        } else {
            //fprintf(stdout, "Incomplete packet\n");
            break;
        }
    }

    if ((*ret) > 0) {
        (*cb)(60, result, (*ret), userdata);
    }

    if (it + next < dev->rx_index) {
        memcpy(dev->rx, dev->rx + it + next, dev->rx_index - it - next);
        dev->rx_index = dev->rx_index - it - next;
    } else {
        dev->rx_index = 0;
    }
}

void device_fw_read42(receive_callback cb, void *userdata, struct _Device* dev, int *ret)
{
    struct _Result result[100];
    uint8_t  buf[sizeof(dev->rx)];
    uint32_t len = 0;

    EnterCriticalSection(&dev->rx_sync);
    if (dev->rx_index > 0) {
        len = dev->rx_index;
        memcpy(buf, dev->rx, dev->rx_index);
        dev->rx_index = 0;
    }
    LeaveCriticalSection(&dev->rx_sync);

    if (len > 0) {
        buf[len] = 0;
        if (buf[0] == 'Q') {
            uint8_t   dev_group = byte_from_hex(&buf[1]);
            if (len >= 510) {
                uint32_t  i = 0;
                uint8_t   dev_index;
                uint8_t   dev_battery = 0;
                uint16_t  dev_timeout = 0;
                char      dev_enter_s[5] = { ' ', ' ', 0, 0, 0 };

                (*ret) = 0;
                for (i = 3; i < len; ++i) {
                    if (buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ')
                        continue;
                    if (buf[i] == 'A' && buf[i + 1] == 'L' && buf[i + 2] == 'L')
                        break;
                    dev_index = byte_from_hex(&buf[i]);
                    i += 2;
                    dev_enter_s[2] = buf[i];
                    dev_enter_s[3] = buf[i + 1];
                    i += 2;
                    dev_battery = 0;
                    if (buf[i] != ' ') {
                        dev_battery = buf[i];
                        ++i;
                    }
                    /** Button = 7 - number */
                    if (isdigit((int)dev_enter_s[3])) {
                        int n = (7 - (dev_enter_s[3] - '0') & 255);
                        dev_enter_s[3] = '0' + n;
                    }

                    if (dev_enter_s[3] != 'E') {
                        fprintf(stdout, "G: %03d, I: %03d, B: %02d, T: %04d, E: %04s\n", dev_group, dev_index, dev_battery, dev_timeout, dev_enter_s);
                        result[(*ret)].group = dev_group;
                        result[(*ret)].index = dev_index;
                        result[(*ret)].battery = dev_battery;
                        result[(*ret)].timeout = dev_timeout;
                        result[(*ret)].enter_s[0] = dev_enter_s[0];
                        result[(*ret)].enter_s[1] = dev_enter_s[1];
                        result[(*ret)].enter_s[2] = dev_enter_s[2];
                        result[(*ret)].enter_s[3] = dev_enter_s[3];
                        result[(*ret)].enter_s[4] = dev_enter_s[4];
                        (*ret)++;
                    }
                }
                if ((*ret) > 0) {
                    (*cb)(45, result, (*ret), userdata);
                }
            } else {
                /** Skip */
            }
        } else {
            fprintf(stdout, "%s\n", (char *)buf);
        }
    }
}
