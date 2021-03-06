#pragma once

struct db;

struct db *db_open (const char *path);
struct db *db_current ();
void       db_close(struct db  *db);

int        db_start_q(struct db *db);
void       db_stop_q(struct db *db);
void       db_q_put(struct db *db, int fw, int group, int index, int timeout, int battery, char enter[5]);
void       db_tm_begin(struct db *db);
void       db_tm_commit(struct db *db);

int        db_last_event(struct db *db);
int        db_get_events(struct db *db, int last_event, char *events, int len, int *writed);
void       db_put_event(struct db *db, int type, int dev);
