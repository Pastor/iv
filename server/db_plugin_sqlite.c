#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <Windows.h>
#include "db_plugin.h"

struct db {
    void            *db;
    int              session;
    char             q[20];
    CRITICAL_SECTION sync;
};

static const char * const db_table_events = "CREATE TABLE IF NOT EXISTS `events` ("
	" `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
    " `type` INTEGER NOT NULL,"
    " `created_at` TEXT NOT NULL DEFAULT 'datetime(''now'')'"
    ")";
static const char * const db_table_sessions = "CREATE TABLE IF NOT EXISTS `sessions` ("
	" `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
    " `created_at` TEXT NOT NULL DEFAULT 'datetime(''now'')',"
    " `completed_at` TEXT DEFAULT NULL"
    ")";
static const char * const db_table_devices = "CREATE TABLE IF NOT EXISTS `devices_%s` ("
	" `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
    " `timeout`	INTEGER NOT NULL DEFAULT 0,"
    " `battery`	INTEGER NOT NULL DEFAULT 0,"
    " `enter`	TEXT NOT NULL DEFAULT 'EEEE'"
    ")";


static const char * const db_pragma[] = {
    "PRAGMA cache_size = 4000;",
    "PRAGMA encoding = \"UTF-8\";",
    "PRAGMA journal_mode = \"MEMORY\";",
    "PRAGMA threads = 10;",
};

struct db *
db_open(const char *path) {
  sqlite3 *db = NULL;
  if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) == SQLITE_OK) {
      int i;

      sqlite3_exec(db, db_table_events, 0, 0, 0);
      sqlite3_exec(db, db_table_sessions, 0, 0, 0);

      for (i = 0; i < sizeof(db_pragma) / sizeof(db_pragma[0]); ++i) {
          sqlite3_exec(db, db_pragma[i], 0, 0, 0);
      }
  } else {
      return NULL;
  }
  struct db *d = (struct db *)calloc(1, sizeof(struct db));
  d->db = db;
  InitializeCriticalSectionAndSpinCount(&d->sync, 0x00000400);
  db_stop_q(d);
  return d;
}

struct db* 
db_current()
{
    char   buf[20];
    time_t timer;
    struct tm *cur;

    time(&timer);
    cur = localtime(&timer);
    strftime(buf, sizeof(buf), "%d.%m.%Y.db3", cur);
    return db_open(buf);
}

int
db_start_q(struct db *db)
{
    char   buf[256];
    time_t timer;
    struct tm *cur;
    int    ret = SQLITE_BUSY;

    time(&timer);
    cur = localtime(&timer);

    EnterCriticalSection(&db->sync);
    strftime(db->q, sizeof(db->q), "%H%M%S", cur);
    sprintf(buf, db_table_devices, db->q);

    ret = sqlite3_exec(db->db, buf, 0, 0, 0);
    LeaveCriticalSection(&db->sync);
    return ret;
}

void
db_stop_q(struct db *db)
{
    char   buf[256];

    EnterCriticalSection(&db->sync);
    sprintf(db->q, "default");
    sprintf(buf, db_table_devices, db->q);
    sqlite3_exec(db->db, buf, 0, 0, 0);
    LeaveCriticalSection(&db->sync);
}

void 
db_q_put(struct db* db, int fw, int group, int index, int timeout, int battery, char enter[5])
{
    int           id = 0;
    sqlite3_stmt *stmt = NULL;
    char          buf[256];

    /** 4 BYTE: RESERVED{2}FIRMWARE{2}GROUP{2}ID{2} */
    fw = fw & 255;
    group = group & 255;
    index = index & 255;

    id += fw << 16;
    id += group << 8;
    id += index;

    EnterCriticalSection(&db->sync);
    sprintf(buf, "INSERT OR REPLACE INTO devices_%s VALUES(?, ?, ?, ?);", db->q);
    if (sqlite3_prepare_v2(db->db, buf, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_int(stmt, 2, timeout);
        sqlite3_bind_int(stmt, 3, battery);
        sqlite3_bind_text(stmt, 4, enter, 4, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    LeaveCriticalSection(&db->sync);
}

void 
db_tm_begin(struct db* db)
{
    EnterCriticalSection(&db->sync);
    sqlite3_exec(db->db, "BEGIN", 0, 0, 0);
}

void 
db_tm_commit(struct db* db)
{
    sqlite3_exec(db->db, "COMMIT", 0, 0, 0);
    LeaveCriticalSection(&db->sync);
}

void 
db_close(struct db *db) {
  if (db != NULL && db->db != NULL) {
    sqlite3_close(db->db);
    db->db = NULL;
    DeleteCriticalSection(&db->sync);
    free(db);
  }
}

/**
static void op_set(struct mg_connection *nc, const struct http_message *hm,
                   const struct mg_str *key, void *db) {
  sqlite3_stmt *stmt = NULL;
  char value[200];
  const struct mg_str *body = hm->query_string.len > 0 ?
    &hm->query_string : &hm->body;

  mg_get_http_var(body, "value", value, sizeof(value));
  if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO kv VALUES (?, ?);",
      -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key->p, key->len, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, strlen(value), SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
}

static void op_get(struct mg_connection *nc, const struct http_message *hm,
                   const struct mg_str *key, void *db) {
  sqlite3_stmt *stmt = NULL;
  const char *data = NULL;
  int result;
  (void) hm;

  if (sqlite3_prepare_v2(db, "SELECT val FROM kv WHERE key = ?;",
      -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key->p, key->len, SQLITE_STATIC);
    result = sqlite3_step(stmt);
    data = (char *) sqlite3_column_text(stmt, 0);
    if ((result == SQLITE_OK || result == SQLITE_ROW) && data != NULL) {
      mg_printf(nc, "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n\r\n%s",
                (int) strlen(data), data);
    } else {
      mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n\r\n");
    }
    sqlite3_finalize(stmt);
  } else {
    mg_printf(nc, "%s", "HTTP/1.1 500 Server Error\r\n"
              "Content-Length: 0\r\n\r\n");
  }
}

static void op_del(struct mg_connection *nc, const struct http_message *hm,
                   const struct mg_str *key, void *db) {
  sqlite3_stmt *stmt = NULL;
  int result;
  (void) hm;

  if (sqlite3_prepare_v2(db, "DELETE FROM kv WHERE key = ?;",
      -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key->p, key->len, SQLITE_STATIC);
    result = sqlite3_step(stmt);
    if (result == SQLITE_OK || result == SQLITE_ROW) {
      mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
    } else {
      mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n\r\n");
    }
    sqlite3_finalize(stmt);
  } else {
    mg_printf(nc, "%s", "HTTP/1.1 500 Server Error\r\n"
              "Content-Length: 0\r\n\r\n");
  }
}

void db_op(struct mg_connection *nc, const struct http_message *hm,
           const struct mg_str *key, void *db, int op) {
  switch (op) {
    case API_OP_GET: op_get(nc, hm, key, db); break;
    case API_OP_SET: op_set(nc, hm, key, db); break;
    case API_OP_DEL: op_del(nc, hm, key, db); break;
    default:
      mg_printf(nc, "%s", "HTTP/1.0 501 Not Implemented\r\n"
                "Content-Length: 0\r\n\r\n");
      break;
  }
}
*/
