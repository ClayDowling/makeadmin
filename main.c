#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "makeadmin_config.h"
#include "sqlite3.h"

void help(const char *program)
{
    fprintf(stderr, "usage: %s -d database -u username -a animal\n"
                    "makeadmin version %d.%d\n", program,
                    MAKEADMIN_VERSION_MAJOR,
                    MAKEADMIN_VERSION_MINOR);
}

int main(int argc, char **argv)
{
    struct sqlite3 *db;
    struct sqlite3_stmt *user_query;
    struct sqlite3_stmt *group_query;
    struct sqlite3_stmt *ug_update;
    struct sqlite3_stmt *validate_query;
    int uid = 0;
    int gid = 0;
    char *animal = NULL;
    char *user = NULL;
    char *database = NULL;
    const char *group = NULL;
    int ch;

    while((ch = getopt(argc, argv, "d:u:a:")) != -1) {
        switch(ch) {
        case 'd':
            database = optarg;
            break;
        case 'u':
            user = optarg;
            break;
        case 'a':
            animal = optarg;
            break;
        default:
            help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (database == NULL || user == NULL || animal == NULL) {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    if (sqlite3_open(database, &db) != SQLITE_OK)
        goto oops;

    if (sqlite3_prepare_v2(db, "SELECT uid FROM user WHERE login=?", -1, &user_query, NULL) != SQLITE_OK)
        goto oops;
    if (sqlite3_prepare_v2(db, "SELECT gid FROM groups WHERE name='admin'", -1, &group_query, NULL) != SQLITE_OK)
        goto oops;
    if (sqlite3_prepare_v2(db, "INSERT INTO usergroup (uid, gid, animal) VALUES (?, ?, ?)", -1, &ug_update, NULL) != SQLITE_OK)
        goto oops;
    if (sqlite3_prepare_v2(db, "SELECT groups.name FROM groups\n"
                           "JOIN usergroup ON groups.gid = usergroup.gid\n"
                           "WHERE usergroup.uid = ? AND usergroup.animal = ?", -1, &validate_query, NULL) != SQLITE_OK)
        goto oops;

    sqlite3_bind_text(user_query, 1, user, -1, SQLITE_STATIC);

    if (sqlite3_step(user_query) != SQLITE_ROW) {
        fprintf(stderr, "User \"%s\" not found.\n", user);
        goto oops;
    }
    uid = sqlite3_column_int(user_query, 1);
    sqlite3_finalize(user_query);

    if (sqlite3_step(group_query) != SQLITE_ROW) {
        fprintf(stderr, "Group \"admin\" not found.\n");
        goto oops;
    }
    gid = sqlite3_column_int(group_query, 1);
    sqlite3_finalize(group_query);

    if (sqlite3_bind_int(ug_update, 1, uid) != SQLITE_OK) goto oops;
    if (sqlite3_bind_int(ug_update, 2, gid) != SQLITE_OK) goto oops;
    if (sqlite3_bind_text(ug_update, 3, animal, -1, SQLITE_STATIC) != SQLITE_OK) goto oops;
    if (sqlite3_step(ug_update) != SQLITE_DONE)
        goto oops;
    sqlite3_finalize(ug_update);

    sqlite3_bind_int(validate_query, 1, uid);
    sqlite3_bind_text(validate_query, 2, animal, -1, SQLITE_STATIC);
    printf("User %s has the following groups on %s:\n", user, animal);
    while(sqlite3_step(validate_query) != SQLITE_DONE) {
        group = sqlite3_column_text(validate_query, 1);
        printf("  %s\n", group);
    }
    sqlite3_finalize(validate_query);

    sqlite3_close(db);

    return EXIT_SUCCESS;

oops:
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    if (NULL != db) {
        sqlite3_close(db);
    }
    return EXIT_FAILURE;
}
