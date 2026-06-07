#ifndef MRT_TEST_H
#define MRT_TEST_H

#include <stdio.h>
#include <stdlib.h>

#define MRT_TEST_ASSERT_TRUE(expr)                                                       \
    do {                                                                                 \
        if (!(expr)) {                                                                   \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); \
            exit(1);                                                                     \
        }                                                                                \
    } while (0)

#define MRT_TEST_ASSERT_EQ_U32(expected, actual)                                       \
    do {                                                                               \
        unsigned long mrt_expected_value = (unsigned long)(expected);                  \
        unsigned long mrt_actual_value = (unsigned long)(actual);                      \
        if (mrt_expected_value != mrt_actual_value) {                                  \
            fprintf(stderr, "%s:%d: expected %lu got %lu\n",                          \
                    __FILE__, __LINE__, mrt_expected_value, mrt_actual_value);         \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#endif
