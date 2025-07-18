#ifndef TINYTEST_H
#define TINYTEST_H

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
typedef void (*TestFunc)(void);

typedef struct TestCase {
    const char *name;
    TestFunc func;
    struct TestCase *next;
} TestCase;

static TestCase *test_list = NULL;

static void tinytest_register(const char *name, TestFunc func) {
    TestCase *tc = malloc(sizeof(TestCase));
    tc->name = name;
    tc->func = func;
    tc->next = test_list;
    test_list = tc;
}

#define TEST(name)                                                             \
    static void name(void);                                                    \
    __attribute__((constructor)) static void register_##name(void) {           \
        tinytest_register(#name, name);                                        \
    }                                                                          \
    static void name(void)

static void tinytest_run_all(void) {
    int passed = 0, failed = 0;
    for (TestCase *tc = test_list; tc != NULL; tc = tc->next) {
        printf("Running %s... ", tc->name);
        fflush(stdout);
        if (setjmp(*((jmp_buf *)malloc(sizeof(jmp_buf))))) {
            printf("FAILED\n");
            failed++;
        } else {
            tc->func();
            printf("PASSED\n");
            passed++;
        }
    }
    printf("\n%d passed, %d failed\n", passed, failed);
}

#define ASSERT_EQ(actual, expected)                                            \
    do {                                                                       \
        typeof(actual) _a = (actual);                                          \
        typeof(expected) _b = (expected);                                      \
        if (_a != _b) {                                                        \
            printf("\nAssertion failed: %s == %s (%s:%d)\n", #actual,          \
                   #expected, __FILE__, __LINE__);                             \
            printf("  Actual:   0x%llx\n", (unsigned long long)_a);            \
            printf("  Expected: 0x%llx\n", (unsigned long long)_b);            \
            longjmp(*((jmp_buf *)malloc(sizeof(jmp_buf))), 1);                 \
        }                                                                      \
    } while (0)

#endif // TINYTEST_H
