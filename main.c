#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "user.h"

#include "makeadmin_config.h"
#include "sqlite3.h"

struct sqlite3 *db = NULL;

void help(const char *program)
{
    fprintf(stderr, "usage: %s -d database -a animal  [-u username][-g group][-le]\n"
                    "makeadmin version %d.%d\n"
                    "\n"
                    "-d database       database file\n"
                    "-a animal         animal to modify/query\n"
                    "-u username       username to update\n"
                    "-g group          group to add\n"
                    "-l                list users in animal\n"
                    "-e                export users for authplain\n"
                    "\n", program,
                    MAKEADMIN_VERSION_MAJOR,
                    MAKEADMIN_VERSION_MINOR);
}

int print_users_from_animal(const char *animal);
int export_users_from_animal(const char *animal);
int add_user_to_group(const char *user, const char *group, const char *animal);


int main(int argc, char **argv)
{
    char *animal = NULL;
    char *user = NULL;
    char *database = NULL;
    const char *group = "admin";
    int ch;
    int result;
    enum {ACT_ADDUSER, ACT_LISTANIMAL, ACT_EXPORT, ACT_MAX} action = ACT_ADDUSER;

    while((ch = getopt(argc, argv, "d:u:a:g:el")) != -1) {
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
        case 'l':
            action = ACT_LISTANIMAL;
            break;
        case 'e':
            action = ACT_EXPORT;
            break;
        default:
            help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (database == NULL || (user == NULL && action == ACT_ADDUSER) || animal == NULL) {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    if (sqlite3_open(database, &db) != SQLITE_OK)
        goto oops;

    switch(action) {
    case ACT_ADDUSER:
        result = add_user_to_group(user, group, animal);
        break;
    case ACT_LISTANIMAL:
        result = print_users_from_animal(animal);
        break;
    case ACT_EXPORT:
        result = export_users_from_animal(animal);
        break;
    }

    sqlite3_close(db);

    return result;

oops:
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    if (NULL != db) {
        sqlite3_close(db);
    }
    return EXIT_FAILURE;
}

int print_users_from_animal(const char *animal)
{
    char **users;
    char *user;
    char **groups;
    char *group;
    int i;
    int j;

    users = users_in_animal(animal);
    if (users == NULL) {
        fprintf(stderr, "Could not read users for %s\n", animal);
        return EXIT_FAILURE;
    }

    printf("Users in %s\n----------------------------------------\n", animal);
    for(i=0; users[i]; ++i) {
        user = users[i];
        printf("%s: ", user);
        groups = user_get_groups(user, animal);
        for(j=0; groups[j]; ++j) {
            group = groups[j];
            if (j==0)
                printf("%s", group);
            else
                printf(", %s", group);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}

int add_user_to_group(const char *user, const char *group,  const char *animal)
{
    char **groups;
    int i;

    user_set_group(user, group, animal);
    if (user_is_in_group(user, group, animal) == 0) {
        fprintf(stderr, "Failed to add %s to %s in %s.\n", user, group, animal);
        return EXIT_FAILURE;
    }

    groups = user_get_groups(user, animal);
    printf("User %s has the following groups on %s:\n", user, animal);

    for(i=0; groups[i]; ++i) {
        printf("  %s\n", groups[i]);
    }

    return EXIT_SUCCESS;
}

int export_users_from_animal(const char *animal)
{
    char **users;
    char *user;
    char **groups;
    char *group;
    struct user_record *rec;
    int i;
    int j;

    users = users_in_animal(animal);

    users = users_in_animal(animal);
    if (NULL == users) {
        return EXIT_FAILURE;
    }

    puts("# users.auth.php\n"
         "# <?php exit()?>\n"
         "# Don't modify the lines above\n"
         "#\n"
         "# Userfile\n"
         "#\n"
         "# Format:\n"
         "#\n"
         "# login:passwordhash:Real Name:email:groups,comma,seperated\n");

    for(i=0; users[i]; ++i) {
        user = users[i];
        groups = user_get_groups(user, animal);
        if (NULL == groups) {
            continue;
        }
        rec = user_get_record(user);
        printf("%s:%s:%s:%s:", rec->login, rec->password, rec->name, rec->email);
        for(j=0; groups[j]; ++j) {
            group = groups[j];
            if (0==j)
                printf("%s", group);
            else
                printf(",%s", group);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}
