#ifndef USER_H
#define USER_H

#include "sqlite3.h"

extern struct sqlite3 *db;

int user_get_uid(const char *login, const char *animal);
int user_get_group_gid(const char *group, const char *animal);
void user_set_group(const char *login, const char *group, const char *animal);
int user_is_in_group(const char *login, const char *group, const char *animal);
char** user_get_groups(const char *login, const char *animal);

#endif // USER_H
