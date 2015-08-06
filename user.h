#ifndef USER_H
#define USER_H

#include "sqlite3.h"

extern struct sqlite3 *db;

struct user_record {
    int uid;
    char *login;
    char *password;
    char *name;
    char *email;
};

int user_get_uid(const char *login);
int user_get_group_gid(const char *group, const char *animal);
void user_set_group(const char *login, const char *group, const char *animal);
int user_is_in_group(const char *login, const char *group, const char *animal);
char**  user_get_groups(const char *login, const char *animal);
char** users_in_animal(const char *animal);

struct user_record* user_get_record(const char *login);


#endif // USER_H
