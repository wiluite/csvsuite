///
/// \file   test/test_reader_macros.h
/// \author wiluite
/// \brief  Different formal readers specially for tests.

#pragma once

#define TEST_TRIM_POLICY_READER(POLICY_NAME)                                      \
    struct POLICY_NAME {                                                          \
    public:                                                                       \
        inline static void trim(std::string &) {}                                 \
        inline static auto ret_trim(std::string s) { return s; }                  \
        inline static auto ret_trim(csv_co::unquoted_cell_string s) { return s; } \
    };                                                                            \
    using test_reader_##POLICY_NAME = csv_co::reader<POLICY_NAME>;

    TEST_TRIM_POLICY_READER(r1)   // test_reader_r1
    TEST_TRIM_POLICY_READER(r2)   // test_reader_r2
    TEST_TRIM_POLICY_READER(r3)   // test_reader_r3
    TEST_TRIM_POLICY_READER(r4)   // test_reader_r4
    TEST_TRIM_POLICY_READER(r5)   // test_reader_r5
    TEST_TRIM_POLICY_READER(r6)   // test_reader_r6
