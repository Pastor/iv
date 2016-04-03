#include <stdio.h>
#include <stdlib.h>
#include <mongoose.h>
#include "device.h"
#include "event.h"
#include "db_plugin.h"

struct event {
    HANDLE          event_thread_handle;
    DWORD           event_thread_id;
    HANDLE          observer_thread_handle;
    DWORD           observer_thread_id;
    struct _Device *df;
    struct _Device *dh;
    struct db *     db;
    HANDLE          event;
    struct mg_connection *nc;
};

static int is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

static void broadcast(struct mg_connection *nc, const char *msg, size_t len) {
    struct mg_connection *c;
    char buf[500];

    snprintf(buf, sizeof(buf), "%.*s", (int)len, msg);
    for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
        if (is_websocket(c))
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
}

static DWORD WINAPI
EventLoop(LPVOID pData) {
    struct event *ev = (struct event *)pData;
    DWORD   rx_wait;
    HANDLE  h[8];
    int i = 0;

    h[0] = ev->df->events[DEVICE_STATUS_OPENED];
    h[1] = ev->df->events[DEVICE_STATUS_CLOSED];
    h[2] = ev->df->events[DEVICE_STATUS_READED];
    h[3] = ev->df->events[DEVICE_STATUS_WRITED];

    h[4] = ev->dh->events[DEVICE_STATUS_OPENED];
    h[5] = ev->dh->events[DEVICE_STATUS_CLOSED];
    h[6] = ev->dh->events[DEVICE_STATUS_READED];
    h[7] = ev->dh->events[DEVICE_STATUS_WRITED];
    ev->df->loop = 1;
    ev->dh->loop = 1;
    db_start_q(ev->db);
    while (ev->df->loop && ev->dh->loop) {
        rx_wait = WaitForMultipleObjects(8, h, FALSE, 100);
        switch (rx_wait) {
        case WAIT_OBJECT_0 + DEVICE_STATUS_OPENED:
        {
            fprintf(stdout, "FTDI connected\n");
            db_put_event(ev->db, DEVICE_CONNECT | DEVICE_FTDI, 0);
            SetEvent(ev->event);
            break;
        }

        case WAIT_OBJECT_0 + DEVICE_STATUS_CLOSED:
        {
            fprintf(stdout, "FTDI disconnected\n");
            db_put_event(ev->db, DEVICE_DISCONNECT | DEVICE_FTDI, 0);
            SetEvent(ev->event);
            break;
        }

        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_OPENED:
        {
            fprintf(stdout, "HID  connected\n");
            db_put_event(ev->db, DEVICE_CONNECT | DEVICE_HID, 0);
            SetEvent(ev->event);
            break;
        }

        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_CLOSED:
        {
            fprintf(stdout, "HID  disconnected\n");
            db_put_event(ev->db, DEVICE_DISCONNECT | DEVICE_HID, 0);
            SetEvent(ev->event);
            break;
        }


        case WAIT_OBJECT_0 + DEVICE_STATUS_READED: /** Приняли с FTDI */
        {
            if (device_receive(ev->db, ev->df)) {
                SetEvent(ev->event);
            }
            break;
        }

        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_READED: /** Приняли с HID */
        {
            if (device_receive(ev->db, ev->dh)) {
                SetEvent(ev->event);
            }
            i = 1;
            break;
        }
        default:
            break;
        }
        if (i > 0) {
            uint8_t d[] = { 2, 6, 'Q', '2', 'D', '\r', 0, 0 };

            i = 0;
            fw_crc_create(&d[2], 4, &d[6]);
            device_write(ev->dh, d, sizeof(d));
        }
    }

    return 0;
}

static DWORD WINAPI OserverHandler(LPVOID pData) {
    HANDLE event = ((struct event *)pData)->event;
    struct mg_connection *nc = ((struct event *)pData)->nc;
    struct db *db = ((struct event *)pData)->db;
    int is_running = 1;
    char events[4096];
    int len;
    int last_event = db_last_event(db);

    while (is_running) {
        switch (WaitForSingleObject(event, INFINITE)) {
        case WAIT_OBJECT_0:
        {
            fprintf(stdout, "Event\n");
            last_event = db_get_events(db, last_event, events, sizeof(events), &len);
            if (last_event > 0 && len > 2) {
                broadcast(nc, events, len);
            }
            break;
        }
        case WAIT_FAILED:
            is_running = 0;
            break;
        }
    }
    return 0;
}

struct event *
    event_start(struct _Device* df, struct _Device* dh, struct db* db, struct mg_connection *nc) {
    struct event *event = (struct event *)calloc(1, sizeof(struct event));
    event->nc = nc;
    event->db = db;
    event->df = df;
    event->dh = dh;
    event->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    event->event_thread_handle = CreateThread(NULL, 0, EventLoop, event, 0, &event->event_thread_id);
    event->observer_thread_handle = CreateThread(NULL, 0, OserverHandler, event, 0, &event->observer_thread_id);
    return event;
}

void
event_stop(struct event** event) {
    if (event != NULL && (*event) != NULL) {
        CloseHandle((*event)->event);
        TerminateThread((*event)->event_thread_handle, -1);
        free((*event));
        (*event) = NULL;
    }
}

void*
event_handle(struct event* event) {
    return event->event;
}
