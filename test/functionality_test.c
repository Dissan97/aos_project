#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib/include/ref_syscall.h"

int all_state[] = {ON, OFF, REC_ON, REC_OFF};
int bad_state[] = {
    0,  ON | OFF, REC_OFF | REC_ON, ON | OFF | REC_ON | REC_OFF, 0x40,
    -1, -1000};
int current_state;
// before test

struct sys_function_pointer {
    int (*change_password_func)(const char *old_password,
                                const char *new_password);
    int (*change_state_func)(const char *password, int state);
    int (*change_path_func)(const char *password, const char *the_path,
                            int add_or_remove);
};

int get_state_from_sys()
{
    char buff[16];
    int dummy;
    int read_bytes;
    FILE *current_state_pseudo_file;

    current_state_pseudo_file = fopen(
        "/sys/module/the_reference_monitor/parameters/current_state", "r");
    if (current_state_pseudo_file == NULL) {
        perror("cannot ope sys");
        return -1;
    }

    read_bytes =
        fread(buff, sizeof(char), sizeof(buff), current_state_pseudo_file);
    if (read_bytes < 0) {
        perror("some problem occured");
        return -1;
    }
    buff[read_bytes - 1] = 0;
    fclose(current_state_pseudo_file);
    return atoi(buff);
}

void test_change_state(char *good_password, char *bad_password,
                       struct sys_function_pointer syscall_function_pointer)
{
    int i = 0;
    int read_state;
    seteuid(1000);
    // good argment but bad user
    int ret;
    assert((ret = syscall_function_pointer.change_state_func(good_password,
                                                             0x1)) < 0);
    perror("good password bad euid expected EPERM");
    assert(errno == EPERM);
    seteuid(0);
    // bad password good state
    assert(syscall_function_pointer.change_state_func(bad_password, 0x1) < 0);
    perror("bad password bad good state expected EACCESS");
    assert(errno == EACCES);
    // bad password bad state
    for (i = 0; bad_state[i] != -1000; i++) {
        printf("testing bad_state=%d\n", bad_state[i]);
        fflush(stdout);
        assert(syscall_function_pointer.change_state_func(bad_password,
                                                          bad_state[i]) < 0);
        perror("bad password bad bad_state expected EINVAL");
        assert(errno == EINVAL);
        perror("good password bad_state expected EINVAL");
        assert(syscall_function_pointer.change_state_func(good_password,
                                                          bad_state[i]) < 0);
        assert(errno == EINVAL);
    }

    printf("trying current state =%d\n", current_state);
    assert(syscall_function_pointer.change_state_func(good_password, current_state) < 0);
    perror("good password same state expected ECANCELED");
    assert(errno == ECANCELED);

    // checking the state transition
    for (i = 0; i < 4; i++) {
        if (current_state != all_state[i]) {
            assert(syscall_function_pointer.change_state_func(
                       good_password, all_state[i]) == 0);
            read_state = get_state_from_sys();
            assert(read_state == all_state[i]);
            printf("{Current state=%d; Expected state=%d}\n", read_state,
                   all_state[i]);
            fflush(stdout);
            current_state = all_state[i];
        }
    }
}

void test_change_path(char *good_password, char *bad_password,
                      const char *test_exists_path, const char *not_exists_path,
                      struct sys_function_pointer syscall_function_pointer)
{
    int ret;
    int expected;
    
    
    const char *forbitten_paths[8] = {"/proc", "/sys", "/run",
                                      "/var",  "/tmp", NULL};
    seteuid(1000);

    ret = change_path(good_password, test_exists_path, ADD_PATH);
    perror("change path invalid euid expected EPERM");
    assert(ret < 0 && errno == EPERM);

    seteuid(0);
    printf("good_password %s\n", good_password);
    fflush(stdout);
    ret = syscall_function_pointer.change_path_func(good_password, NULL,
                                                    ADD_PATH);
    perror("change path invalid path=NULL expected EINVAL");

    assert(ret < 0 && errno == EINVAL);
    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, (ADD_PATH + 1000));
    perror("change path invalid op=ADD_PATH+1000 expected EINVAL");
    assert(ret < 0 && errno == EINVAL);

    ret = syscall_function_pointer.change_path_func(
        bad_password, test_exists_path, (ADD_PATH & REMOVE_PATH));
    perror("change path invalid op=ADD_PATH&REMOVE_PATH expected EINVAL");
    assert(ret < 0 && errno == EINVAL);

    ret = syscall_function_pointer.change_path_func(
        bad_password, test_exists_path, (ADD_PATH & REMOVE_PATH));
    perror("change path invalid op=password expected EINVAL");
    assert(ret < 0 && errno == EINVAL);

    // changing configuration to rec_off
    ret = syscall_function_pointer.change_state_func(good_password, REC_OFF);
    if (ret) {
        perror("cannot call sys_change_state");
        return;
    }
    current_state = get_state_from_sys();
    if (current_state < 0) {
        return;
    }

    for (int i = 0; forbitten_paths[i] != NULL; i++) {
        ret = syscall_function_pointer.change_path_func(
            good_password, forbitten_paths[i], ADD_PATH);
        printf("passing %s not allowed ", forbitten_paths[i]);
        fflush(stdout);
        perror("expected EINVAL");
        assert(ret < 0 && errno == EINVAL);
    }

    ret = syscall_function_pointer.change_path_func(
        bad_password, test_exists_path, ADD_PATH);
    perror("change path invalid passoword expected EACCESS");
    assert(ret < 0 && errno == EACCES);

    current_state = get_state_from_sys();

    if (((current_state & REC_OFF) ||
         (current_state & REC_ON))) { // reconfiguration mode is on
        ret = syscall_function_pointer.change_state_func(good_password, OFF);
        if (ret) {
            perror("cannot change state to off");
            exit(EXIT_FAILURE);
        }
    }

    // test -ECANCELED

    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, ADD_PATH);
    perror("change path invalid op=ADD_PATH and state != REC_ON && state "
           "!= REC_ON REC_OFF expected ECANCELED");
    assert(ret < 0 && errno == ECANCELED);

    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, REMOVE_PATH);
    perror("change path invalid op=REMOVE_PATH and state != REC_ON && "
           "state != REC_ON REC_OFF expected ECANCELED");
    assert(ret < 0 && errno == ECANCELED);

    // test good

    current_state = get_state_from_sys();
    if (!(current_state & REC_OFF & REC_ON)) { // reconfiguration mode is off
        ret =
            syscall_function_pointer.change_state_func(good_password, REC_OFF);
        if (ret) {
            perror("cannot change state to rec_off");
            exit(EXIT_FAILURE);
        }
    }

    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, ADD_PATH);
    printf("change path op=ADD_PATH and state = REC_OFF\n");
    fflush(stdout);
    assert(ret == 0);

    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, ADD_PATH);
    printf("change path op=REMOVE_PATH and path already in list: ");
    fflush(stdout);
    perror("expected EEXIST");
    assert(ret < 0 && errno == EEXIST);

    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, REMOVE_PATH);
    printf("change path op=REMOVE_PATH and state = REC_OFF\n");
    fflush(stdout);
    assert(ret == 0);

    // no testing removing not existing path
    ret = syscall_function_pointer.change_path_func(
        good_password, test_exists_path, REMOVE_PATH);
    printf("change path op=REMOVE_PATH and path not in list: ");
    fflush(stdout);
    perror("expected ENOENT");
    assert(ret < 0 && errno == ENOENT);
}

int main(int argc, char **argv)
{

    char *good_password;
    char *bad_password;
    char *good_path;
    char *bad_path;
    int before_state;
    size_t len;
    struct sys_function_pointer syscall_function_pointer;

    if (argc < 6) {
        printf("Launch program refMonitorPwd badPwd exists_path not_exists_path current_reference_monitor_state\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    printf("launcing %s\n", argv[0]);
    printf("good_password=%s\n", argv[1]);
    printf("before_password=%s\n", argv[2]);
    printf("good_path=%s\n", argv[3]);
    printf("bad_path=%s\n", argv[4]);
    printf("before_state=%s\n", argv[5]);

    good_password = argv[1];
    bad_password = argv[2];
    good_path = argv[3];
    bad_path = argv[4];
    before_state = strtol(argv[5], NULL, 10);
    
    if (before_state == 0){
        printf("cannot use before_state=%d\n", before_state);
        exit(EXIT_FAILURE);
    }
    current_state = before_state;
    printf("before_state=%d\n", current_state);
    printf("running change state tests\n");
    fflush(stdout);
    fflush(stdout);
    syscall_function_pointer.change_password_func = change_password;
    syscall_function_pointer.change_state_func = change_state;
    syscall_function_pointer.change_path_func = change_path;
    printf("TESTING_CLASSIC SYSTEM CALL\n");
    fflush(stdout);

    test_change_state(good_password, bad_password, syscall_function_pointer);
    printf("change state tests passed\n");
    fflush(stdout);
    printf("current_state=%d - restoring before_state=%d\n", current_state,
           before_state);

    change_state(good_password, before_state);

    current_state = before_state;
    printf("running change path tests\n");
    fflush(stdout);
    
    test_change_path(good_password, bad_password, good_path, bad_path, syscall_function_pointer);
    
    syscall_function_pointer.change_password_func = path_change_password;
    syscall_function_pointer.change_state_func = path_change_state;
    syscall_function_pointer.change_path_func = path_change_path;

    printf("TESTING_PATH_BASED SYSTEM CALL\n");
    fflush(stdout);
    change_state(good_password, before_state);
    current_state = before_state;
    test_change_state(good_password, bad_password, syscall_function_pointer);
    printf("change state tests passed\n");
    fflush(stdout);
    printf("current_state=%d - restoring before_state=%d\n", current_state,
           before_state);
    change_state(good_password, before_state);
    current_state = before_state;
    printf("running change path tests\n");
    test_change_path(good_password, bad_password, good_path, bad_path, syscall_function_pointer);

    printf("all tests passed\n");
    fflush(stdout);
}