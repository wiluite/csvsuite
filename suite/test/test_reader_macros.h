///
/// \file   test/test_reader_macros.h
/// \author wiluite
/// \brief  Different formal readers specially for tests.

#pragma once

#define TEST_TRIM_POLICY_READER(NAME)                                             \
    struct NAME {                                                                 \
    public:                                                                       \
        inline static void trim(std::string &) {}                                 \
        inline static auto ret_trim(std::string s) { return s; }                  \
        inline static auto ret_trim(csv_co::unquoted_cell_string s) { return s; } \
    };                                                                            \
    using test_reader_##NAME = csv_co::reader<NAME>;

    TEST_TRIM_POLICY_READER(r1)   // test_reader_r1
    TEST_TRIM_POLICY_READER(r2)   // test_reader_r2
    TEST_TRIM_POLICY_READER(r3)   // test_reader_r3
    TEST_TRIM_POLICY_READER(r4)   // test_reader_r4
    TEST_TRIM_POLICY_READER(r5)   // test_reader_r5
    TEST_TRIM_POLICY_READER(r6)   // test_reader_r6
    TEST_TRIM_POLICY_READER(r7)   // test_reader_r7
    TEST_TRIM_POLICY_READER(r8)   // test_reader_r8
    TEST_TRIM_POLICY_READER(r9)   // test_reader_r9
    TEST_TRIM_POLICY_READER(r10)  // test_reader_r10
    TEST_TRIM_POLICY_READER(r11)  // test_reader_r11
    TEST_TRIM_POLICY_READER(r12)  // test_reader_r12
    TEST_TRIM_POLICY_READER(r13)  // test_reader_r13
    TEST_TRIM_POLICY_READER(r14)  // test_reader_r14
    TEST_TRIM_POLICY_READER(r15)  // test_reader_r15
