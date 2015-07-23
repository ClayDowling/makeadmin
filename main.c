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
    int i = 0;
    char *animal = NULL;
    char *user = NULL;
    char *database = NULL;
    const char *group = "admin";
    char **groups;
    int ch;

    while((ch = getopt(argc, argv, "d:u:a:g:")) != -1) {
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
        case 'g':
            group = optarg;
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

    user_set_group(user, group, animal);
    if (user_is_in_group(user, group, animal) == 0) {
        fprintf(stderr, "Failed to add %s to %s in %s.\n", user, group, animal);
    }

    groups = user_get_groups(user, animal);
    sqlite3_close(db);
    printf("User %s has the following groups on %s:\n", user, animal);

    for(i=0; groups[i]; ++i) {
        printf("  %s\n", groups[i]);
    }

    return EXIT_SUCCESS;

oops:
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    if (NULL != db) {
        sqlite3_close(db);
    }
    return EXIT_FAILURE;
}
