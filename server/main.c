#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <mongoose.h>
#include "event.h"
#include "db_plugin.h"
#include "device_ftdi.h"
#include "device_hid.h"

static sig_atomic_t is_running = 1;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static const struct mg_str s_get_method = MG_MK_STR("GET");
static const struct mg_str s_put_method = MG_MK_STR("PUT");
static const struct mg_str s_delele_method = MG_MK_STR("DELETE");

struct Observer {
    HANDLE                event;
    struct db            *db;
    int                   last_event;
    struct mg_connection *nc;
};

int group(unsigned char b[2]) {
    if (b[0] >= '0' && b[0] <= '9') {
        if (b[1] >= '0' && b[1] <= '9')
            return ((b[0] - '0') << 4 | b[1] - '0') & 255;
        return ((b[0] - '0') << 4 | (b[1] - 'A' + 10)) & 255;
    }
    if (b[1] >= '0' && b[1] <= '9')
        return ((b[0] - 'A' + 10) << 4 | b[1] - '0') & 255;
    return ((b[0] - 'A' + 10) << 4 | (b[1] - 'A' + 10)) & 255;
}

static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
    return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

static int is_equal(const struct mg_str *s1, const struct mg_str *s2) {
    return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    static const struct mg_str api_prefix = MG_MK_STR("/api/v1");
    struct http_message *hm = (struct http_message *) ev_data;
    struct websocket_message *wm = (struct websocket_message *) ev_data;
    struct mg_str key;

    switch (ev) {
    case MG_EV_HTTP_REQUEST:
        if (has_prefix(&hm->uri, &api_prefix)) {
            key.p = hm->uri.p + api_prefix.len;
            key.len = hm->uri.len - api_prefix.len;
            if (is_equal(&hm->method, &s_get_method)) {
                
            } else if (is_equal(&hm->method, &s_put_method)) {
                
            } else if (is_equal(&hm->method, &s_delele_method)) {
                
            } else {
                mg_printf(nc, "%s",
                    "HTTP/1.0 501 Not Implemented\r\n"
                    "Content-Length: 0\r\n\r\n");
            }
        } else {
            mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
        }
        break;
    case MG_EV_WEBSOCKET_FRAME:
        /* New websocket message. Tell everybody. */
//        broadcast(nc, (char *)wm->data, wm->size);
        break;
    default:
        break;
    }
}

static DWORD WINAPI OserverHandler(LPVOID pData) {
    struct Observer *observer = (struct Observer *)pData;
    int is_running = 1;

    while (is_running) {
        switch (WaitForSingleObject(observer->event, INFINITE)) {
        case WAIT_OBJECT_0:
        {
            fprintf(stdout, "Event\n");
            break;
        }
        case WAIT_FAILED:
            is_running = 0;
            break;
        }
    }
    return 0;
}

int
main(int argc, char *argv[]) {
    struct _Device        df;
    struct _Device        dh;
    struct db            *db;
    struct event         *event;
    struct mg_mgr         mgr;
    struct mg_connection *nc;

    mg_mgr_init(&mgr, NULL);
    nc = mg_bind(&mgr, s_http_port, ev_handler);
    mg_set_protocol_http_websocket(nc);
//    mg_enable_multithreading(nc);
    s_http_server_opts.document_root = "web";

    db = db_current();
    device_ftdi_init(&df);
    device_hid_init(&dh);
    event = event_start(&df, &dh, db, nc);

    while (is_running) {
        mg_mgr_poll(&mgr, 100);
    }

    event_stop(&event);
    db_close(db);
    device_hid_exit(&dh);
    device_ftdi_exit(&df);

    mg_mgr_free(&mgr);
    system("pause");
    return EXIT_SUCCESS;
}
