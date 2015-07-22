#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "user.h"

#include "makeadmin_config.h"
#include "sqlite3.h"

struct sqlite3 *db = NULL;

void help(const char *program)
{
    fprintf(stderr, "usage: %s -d database -u username -a animal\n"
                    "makeadmin version %d.%d\n", program,
                    MAKEADMIN_VERSION_MAJOR,
                    MAKEADMIN_VERSION_MINOR);
}

int main(int argc, char **argv)
{
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

    user_set_group(user, "admin", animal);
    if (user_is_in_group(user, "admin", animal) == 0) {
        fprintf(stderr, "Failed to add %s to admin in %s.\n", user, animal);
    }

    if (sqlite3_prepare_v2(db, "SELECT groups.name FROM groups\n"
                           "JOIN usergroup ON groups.gid = usergroup.gid\n"
                           "WHERE usergroup.uid = ? AND usergroup.animal = ?", -1, &validate_query, NULL) != SQLITE_OK)
        goto oops;

    uid = user_get_uid(user, animal);
    sqlite3_bind_int(validate_query, 1, uid);
    sqlite3_bind_text(validate_query, 2, animal, -1, SQLITE_STATIC);
    printf("User %s has the following groups on %s:\n", user, animal);
    while(sqlite3_step(validate_query) == SQLITE_ROW) {
        group = sqlite3_column_text(validate_query, 0);
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
