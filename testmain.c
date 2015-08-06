#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "user.h"
#include "sqlite3.h"
#include "CuTest.h"

struct sqlite3 *db = 0;

void setup() {
    if (sqlite3_open("", &db) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "CREATE TABLE user (\n"
                     "  uid integer not null primary key,\n"
                     "  login text not null,\n"
                     "  pass text not null,\n"
                     "  fullname text not null default '',\n"
                     "  email text not null default '',\n"
                     "  unique(login),\n"
                     "  unique(email)\n"
                     ")", NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "CREATE TABLE groups (\n"
                   "  gid integer not null primary key,\n"
                   "  name text not null,\n"
                   "  unique(name)\n"
                   ")", NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "CREATE TABLE usergroup (\n"
                   "  uid integer not null,\n"
                   "  gid integer not null,\n"
                   "  animal text not null default '',\n"
                   "  primary key (uid, gid, animal)\n"
                   ")", NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO user (uid, login, pass, fullname, email)\n"
                     "VALUES (1, 'bob', '$1$2$abc', 'Bob Smith', 'bob@email.org')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO user (uid, login, pass, fullname, email)\n"
                     "VALUES (2, 'tom', '$1$3$def', 'Tom Jones', 'tom@smith.us')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO groups (gid, name)\n"
                     "VALUES (1, 'admin')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO groups (gid, name)\n"
                     "VALUES (2, 'user')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO usergroup (uid, gid, animal)\n"
                     "VALUES (1, 2, 'first')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO usergroup (uid, gid, animal)\n"
                     "VALUES (1, 1, 'first')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO usergroup (uid, gid, animal)\n"
                     "VALUES (2, 2, 'first')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    if (sqlite3_exec(db, "INSERT INTO usergroup (uid, gid, animal)\n"
                     "VALUES (1, 2, 'second')",
                     NULL, NULL, NULL) != SQLITE_OK) {
        goto setuperror;
    }

    return;
setuperror:
    fprintf(stderr, "setup: %s\n", sqlite3_errmsg(db));
    exit(1);
}

void teardown() {
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
    }
}

void test_user_get_uid_retrieves_targetted_user(CuTest *tc)
{
    int uid;

    setup();
    uid = user_get_uid("bob", "first");
    CuAssertIntEquals(tc, 1, uid);
    teardown();
}

void test_user_get_uid_does_not_retrieve_invalid_user(CuTest *tc)
{
    int uid;

    setup();
    uid = user_get_uid("noone", "first");
    CuAssertIntEquals(tc, -1, uid);
    teardown();
}

void test_user_get_group_gid_retrieves_desired_group(CuTest *tc)
{
    int gid;

    setup();
    gid = user_get_group_gid("admin", "first");
    CuAssertIntEquals(tc, 1, gid);
    teardown();
}

void test_user_get_group_gid_does_not_retrieve_invalid_group(CuTest *tc)
{
    int gid;

    setup();
    gid = user_get_group_gid("nosuchgroup", "first");
    CuAssertIntEquals(tc, -1, gid);
    teardown();
}

void test_user_is_in_group_true_when_user_in_group(CuTest *tc)
{
    int is_in_group;

    setup();
    is_in_group = user_is_in_group("bob", "user", "second");
    CuAssertIntEquals(tc, 1, is_in_group);
    teardown();
}

void test_user_is_in_group_false_when_user_not_in_group(CuTest *tc)
{
    int is_in_group;

    setup();
    is_in_group = user_is_in_group("tom", "user", "second");
    CuAssertIntEquals(tc, 0, is_in_group);
    teardown();
}

void test_user_group_add_succeeds_for_valid_user_group(CuTest *tc)
{
    int is_in_group;

    setup();
    user_set_group("bob", "admin", "second");
    is_in_group = user_is_in_group("bob", "admin", "second");
    CuAssertIntEquals(tc, 1, is_in_group);
    teardown();
}

void test_user_group_add_fails_for_invalid_user(CuTest *tc)
{
    int is_in_group;

    setup();
    user_set_group("frank", "admin", "second");
    is_in_group = user_is_in_group("frank", "admin", "second");
    CuAssertIntEquals(tc, 0, is_in_group);
    teardown();
}

void test_user_groups_retrieves_users_groups(CuTest *tc)
{
    char **groups;
    setup();

    groups = user_get_groups("bob", "first");
    CuAssertPtrNotNull(tc, groups);
    CuAssertStrEquals(tc, "admin", groups[0]);
    CuAssertStrEquals(tc, "user", groups[1]);

    /* This assertion ensures that groups come from a single
     * animal.
     */
    CuAssertPtrEquals(tc, NULL, groups[2]);

    teardown();
}

void test_users_in_animal_is_not_null(CuTest *tc)
{
    char **users;

    setup();
    users = users_in_animal("first");
    CuAssertPtrNotNull(tc, users);
    teardown();
}

void test_user_in_animal_returns_two_users_for_first_animal(CuTest *tc)
{
    char **users;

    setup();
    users = users_in_animal("first");
    CuAssertPtrNotNull(tc, users[0]);
    CuAssertPtrNotNull(tc, users[1]);
    CuAssertPtrEquals(tc, NULL, users[2]);
    teardown();
}


int main(int argc, char **argv)
{
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_user_get_uid_retrieves_targetted_user);
    SUITE_ADD_TEST(suite, test_user_get_uid_does_not_retrieve_invalid_user);
    SUITE_ADD_TEST(suite, test_user_get_group_gid_retrieves_desired_group);
    SUITE_ADD_TEST(suite, test_user_get_group_gid_does_not_retrieve_invalid_group);
    SUITE_ADD_TEST(suite, test_user_is_in_group_true_when_user_in_group);
    SUITE_ADD_TEST(suite, test_user_is_in_group_false_when_user_not_in_group);
    SUITE_ADD_TEST(suite, test_user_group_add_succeeds_for_valid_user_group);
    SUITE_ADD_TEST(suite, test_user_group_add_fails_for_invalid_user);
    SUITE_ADD_TEST(suite, test_user_groups_retrieves_users_groups);
    SUITE_ADD_TEST(suite, test_users_in_animal_is_not_null);
    SUITE_ADD_TEST(suite, test_user_in_animal_returns_two_users_for_first_animal);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    return suite->failCount;
}
