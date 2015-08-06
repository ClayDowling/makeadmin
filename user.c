#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "user.h"

int user_get_uid(const char *login)
{
    struct sqlite3_stmt *user_query;
    int uid = -1;

    if (sqlite3_prepare_v2(db, "SELECT uid FROM user WHERE login=?", -1, &user_query, NULL) != SQLITE_OK) {
        goto uidoops;
    }

    if (sqlite3_bind_text(user_query, 1, login, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto uidoops;
    }

    if (sqlite3_step(user_query) == SQLITE_ROW) {
        uid = sqlite3_column_int(user_query, 0);
    }

    sqlite3_finalize(user_query);

    return uid;
uidoops:
    fprintf(stderr, "%s: %s\n", __func__, sqlite3_errmsg(db));
    return -1;
}

int user_get_group_gid(const char *group, const char *animal)
{
    struct sqlite3_stmt *group_query;
    int gid = -1;

    if (sqlite3_prepare_v2(db, "SELECT gid FROM groups WHERE name=?", -1, &group_query, NULL) != SQLITE_OK)
        goto gidoops;

    if (sqlite3_bind_text(group_query, 1, group, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto gidoops;
    }

    if (sqlite3_step(group_query) == SQLITE_ROW) {
        gid = sqlite3_column_int(group_query, 0);
    }

    sqlite3_finalize(group_query);

    return gid;
gidoops:
    fprintf(stderr, "%s: %s\n", __func__, sqlite3_errmsg(db));
    return -1;
}

void user_set_group(const char *login, const char *group, const char *animal)
{
    struct sqlite3_stmt *ug_update;
    int uid;
    int gid;

    uid = user_get_uid(login);
    if (uid < 1) {
        fprintf(stderr, "%s: login %s not found.\n", __func__, login);
        return;
    }

    gid = user_get_group_gid(group, animal);
    if (gid < 1) {
        fprintf(stderr, "%s: group %s not found.\n", __func__, login);
        return;
    }

    if (sqlite3_prepare_v2(db, "INSERT INTO usergroup (uid, gid, animal) VALUES (?, ?, ?)", -1, &ug_update, NULL) != SQLITE_OK)
        goto ugoops;

    if (sqlite3_bind_int(ug_update, 1, uid) != SQLITE_OK) {
        goto ugoops;
    }

    if (sqlite3_bind_int(ug_update, 2, gid) != SQLITE_OK) {
        goto ugoops;
    }

    if (sqlite3_bind_text(ug_update, 3, animal, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto ugoops;
    }

    if (sqlite3_step(ug_update) != SQLITE_DONE) {
        goto ugoops;
    }
    sqlite3_finalize(ug_update);
    return;
ugoops:
    fprintf(stderr, "%s: %s\n", __func__, sqlite3_errmsg(db));
}

int user_is_in_group(const char *user, const char *group, const char *animal)
{
    struct sqlite3_stmt *query;
    int hasgroup = 0;
    int uid;
    int gid;

    uid = user_get_uid(user);
    gid = user_get_group_gid(group, animal);

    if (sqlite3_prepare_v2(db, "SELECT * FROM usergroup WHERE uid=? AND gid=? AND animal=?", -1, &query, NULL) != SQLITE_OK)
        goto ingroupoops;

    if(sqlite3_bind_int(query, 1, uid) != SQLITE_OK) goto ingroupoops;
    if(sqlite3_bind_int(query, 2, gid) != SQLITE_OK) goto ingroupoops;
    if(sqlite3_bind_text(query, 3, animal, -1, SQLITE_STATIC) != SQLITE_OK) goto ingroupoops;

    if (sqlite3_step(query)  == SQLITE_ROW) {
        hasgroup = 1;
    }

    sqlite3_finalize(query);
    return hasgroup;

ingroupoops:
    fprintf(stderr, "%s: %s\n", __func__, sqlite3_errmsg(db));
    return 0;
}

char** user_get_groups(const char *login, const char *animal)
{
    char **groups = NULL;
    struct sqlite3_stmt *query;
    int uid;
    int count;
    int i;

    uid = user_get_uid(login);

    if (sqlite3_prepare_v2(db, "SELECT COUNT(groups.name) FROM groups\n"
                           "JOIN usergroup ON usergroup.gid = groups.gid\n"
                           "WHERE usergroup.uid=? AND usergroup.animal=?", -1, &query, NULL) != SQLITE_OK) {
        goto groupsoops;
    }
    if (sqlite3_bind_int(query, 1, uid) != SQLITE_OK) {
        goto groupsoops;
    }
    if (sqlite3_bind_text(query, 2, animal, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto groupsoops;
    }
    if (sqlite3_step(query) != SQLITE_ROW) {
        goto groupsoops;
    }
    count = sqlite3_column_int(query, 0);
    groups = (char**)calloc(count + 1, sizeof(char*));

    sqlite3_reset(query);
    if (sqlite3_prepare_v2(db, "SELECT groups.name FROM groups\n"
                           "JOIN usergroup ON usergroup.gid = groups.gid\n"
                           "WHERE usergroup.uid=? AND usergroup.animal=?", -1, &query, NULL) != SQLITE_OK) {
        goto groupsoops;
    }
    if (sqlite3_bind_int(query, 1, uid) != SQLITE_OK) {
        goto groupsoops;
    }
    if (sqlite3_bind_text(query, 2, animal, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto groupsoops;
    }

    i = 0;
    while(sqlite3_step(query) == SQLITE_ROW) {
        groups[i] = strdup(sqlite3_column_text(query, 0));
        ++i;
    }
    sqlite3_finalize(query);

    return groups;

groupsoops:
    fprintf(stderr, "%s: %s\n", __func__, sqlite3_errmsg(db));
    return NULL;
}

char** users_in_animal(const char *animal)
{
    char **users = NULL;
    sqlite3_stmt *query = NULL;
    int count;
    int i;

    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM (\n"
                               "SELECT DISTINCT login FROM user\n"
                               "JOIN usergroup ug ON ug.uid = user.uid \n"
                               "JOIN groups ON groups.gid = ug.gid\n"
                               "WHERE ug.animal=?\n"
                               "ORDER BY user.login)", -1, &query, NULL) != SQLITE_OK) {
        goto inanimal_oops;
    }
    if (sqlite3_bind_text(query, 1, animal, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto inanimal_oops;
    }
    if (sqlite3_step(query) != SQLITE_ROW) {
        goto inanimal_oops;
    }
    count = sqlite3_column_int(query, 0);
    users = (char**)calloc(count + 1, sizeof(char*));
    sqlite3_reset(query);

    if (sqlite3_prepare_v2(db, "SELECT DISTINCT login FROM user\n"
                               "JOIN usergroup ug ON ug.uid = user.uid \n"
                               "JOIN groups ON groups.gid = ug.gid\n"
                               "WHERE ug.animal=?\n"
                               "ORDER BY user.login", -1, &query, NULL) != SQLITE_OK) {
        goto inanimal_oops;
    }
    if (sqlite3_bind_text(query, 1, animal, -1, SQLITE_STATIC) != SQLITE_OK) {
        goto inanimal_oops;
    }

    i = 0;
    while(sqlite3_step(query) == SQLITE_ROW) {
        users[i] = strdup(sqlite3_column_text(query, 0));
        ++i;
    }
    sqlite3_finalize(query);

    return users;
inanimal_oops:
    fprintf(stderr, "%s: %s\n\n", __func__, sqlite3_errmsg(db));
    return NULL;
}

struct user_record* user_get_record(const char *login)
{
    struct user_record *rec = NULL;
    sqlite3_stmt *query = NULL;
    int uid;

    rec = (struct user_record*)calloc(1, sizeof(struct user_record));
    uid = user_get_uid(login);
    if (uid <= 0) {
        fprintf(stderr, "User %s not found.\n", login);
        return NULL;
    }
    if (sqlite3_prepare_v2(db, "SELECT uid, login, pass, fullname, email FROM user WHERE uid=?",
                           -1, &query, NULL) != SQLITE_OK) {
        goto getrecordoops;
    }
    if (sqlite3_bind_int(query, 1, uid) != SQLITE_OK) {
        goto getrecordoops;
    }
    if (sqlite3_step(query) != SQLITE_ROW) {
        goto getrecordoops;
    }

    rec->uid = sqlite3_column_int(query, 0);
    rec->login = strdup(sqlite3_column_text(query, 1));
    rec->password = strdup(sqlite3_column_text(query, 2));
    rec->name = strdup(sqlite3_column_text(query, 3));
    rec->email = strdup(sqlite3_column_text(query, 4));
    sqlite3_finalize(query);

    return rec;

getrecordoops:
    fprintf(stderr, "%s: %s\n\n", __func__, sqlite3_errmsg(db));
    return NULL;
}

