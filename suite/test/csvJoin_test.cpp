///
/// \file   test/csvJoin_test.cpp
/// \author wiluite
/// \brief  Tests for the csvJoin utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvJoin.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_max_field_size_macros.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call) std::stringstream cout_buffer;                        \
                                             {                                                     \
                                                 redirect(cout)                                    \
                                                 redirect_cout cr(cout_buffer.rdbuf());            \
                                                 call;                                             \
                                             }

int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    struct csvjoin_specific_args {
        std::vector<std::string> files;
        bool right_join {false};
        bool left_join {false};
        bool outer_join {false};
        bool check_integrity = {true};
        bool honest_outer_join = {false};
    };

    "runs"_test = [] {
        namespace tf = csvsuite::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::output_args, csvjoin_specific_args {
            Args() {
                columns.clear();
                date_fmt = "%d/%m/%Y";
                datetime_fmt = "%d/%m/%Y %H:%M:%S";
            }
        } args;

        "sequential"_test = [&] {
            {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a_2,b_2,c_2
1,b,c,1,b,c
2,b,c,1,b,c
3,b,c,4,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(4 == new_reader.rows());
            }
            {
                // TEST FOR UBSAN (multiple (more than 2) sources)
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a_2,b_2,c_2,a_3,b_3,c_3
1,b,c,1,b,c,1,b,c
2,b,c,1,b,c,1,b,c
3,b,c,4,b,c,4,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(4 == new_reader.rows());
            }
            "max field size in this mode"_test = [&] {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"test_field_size_limit.csv", "test_field_size_limit.csv"};
                using namespace z_test;
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "inner"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b_2,c_2
1,b,c,b,c
1,b,c,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(3 == new_reader.rows());
            }
            {
                // TEST FOR UBSAN (multiple (more than 2) sources)
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b_2,c_2,b_3,c_3
1,b,c,b,c,b,c
1,b,c,b,c,b,c
1,b,c,b,c,b,c
1,b,c,b,c,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(5 == new_reader.rows());
            }
            "max field size in this mode"_test = [&] {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"test_field_size_limit.csv", "test_field_size_limit.csv"};
                args_copy.columns = "1";
                using namespace z_test;
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "left"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.left_join = true;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b_2,c_2
1,b,c,b,c
1,b,c,b,c
2,b,c,,
3,b,c,,
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(5 == new_reader.rows());
            }
            {
                // TEST FOR UBSAN (multiple (more than 2) sources)
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.left_join = true;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b_2,c_2,b_3,c_3
1,b,c,b,c,b,c
1,b,c,b,c,b,c
1,b,c,b,c,b,c
1,b,c,b,c,b,c
2,b,c,,,,
3,b,c,,,,
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(7 == new_reader.rows());
            }
            "max field size in this mode"_test = [&] {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"test_field_size_limit.csv", "test_field_size_limit.csv"};
                args_copy.columns = "1";
                args_copy.left_join = true;
                using namespace z_test;
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "right"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.right_join = true;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b_2,c_2
1,b,c,b,c
1,b,c,b,c
4,b,c,,
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(4 == new_reader.rows());
            }
            {
                // TEST FOR UBSAN (multiple (more than 2) sources)
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.right_join = true;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b_2,c_2,b_3,c_3
1,b,c,b,c,b,c
1,b,c,b,c,b,c
1,b,c,b,c,b,c
1,b,c,b,c,b,c
4,b,c,b,c,,
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(6 == new_reader.rows());
            }
            "max field size in this mode"_test = [&] {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"test_field_size_limit.csv", "test_field_size_limit.csv"};
                args_copy.columns = "1";
                args_copy.right_join = true;
                using namespace z_test;

#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")

            };
        };

        "right indices"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1,4";
            args_copy.right_join = true;
            args_copy.files = std::vector<std::string>{"join_a.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
            expect(cout_buffer.str() == R"(a,b,c,d,e,f,b_2,c_2
,,,,,,,
)");
            notrimming_reader_type new_reader (cout_buffer.str());
            expect(2 == new_reader.rows());

            "max field size in this mode"_test = [&] {
                args_copy.files = std::vector<std::string>{"test_field_size_limit.csv", "test_field_size_limit.csv"};
                args_copy.columns = "1,3";
                using namespace z_test;
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), 1, skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };

        };

        "outer"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.outer_join = true;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a_2,b_2,c_2
1,b,c,1,b,c
1,b,c,1,b,c
2,b,c,,,
3,b,c,,,
,,,4,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(6 == new_reader.rows());
            }
            {
                // TEST FOR UBSAN (multiple (more than 2) sources)
                // TODO: fixme till NY.
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.outer_join = true;
                args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv", "join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a_2,b_2,c_2,a_3,b_3,c_3
1,b,c,1,b,c,1,b,c
1,b,c,1,b,c,1,b,c
1,b,c,1,b,c,1,b,c
1,b,c,1,b,c,1,b,c
2,b,c,,,,,,
3,b,c,,,,,,
,,,4,b,c,,,
,,,,,,4,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(9 == new_reader.rows());
            }
            "max field size in this mode"_test = [&] {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"test_field_size_limit.csv", "test_field_size_limit.csv"};
                args_copy.columns = "1";
                args_copy.outer_join = true;
                using namespace z_test;
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#if 0
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.\n")
#endif
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "single"_test = [&] {
            auto args_copy = args;
            args_copy.no_inference = true;
            args_copy.files = std::vector<std::string>{"dummy.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c
//          1,2,3

            expect("a,b,c\n1,2,3\n" == cout_buffer.str());
        };

        "no blanks"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"blanks.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2
//          ,,,,,,,,,,,

            expect("a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2\n,,,,,,,,,,,\n" == cout_buffer.str());
        };

        "blanks"_test = [&] {
            auto args_copy = args;
            args_copy.blanks = true;
            args_copy.files = std::vector<std::string>{"blanks.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2
//          ,NA,N/A,NONE,NULL,.,,NA,N/A,NONE,NULL,.

            expect("a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2\n,NA,N/A,NONE,NULL,.,,NA,N/A,NONE,NULL,.\n" == cout_buffer.str());
        };

        "no header row"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1";
            args_copy.no_header = true;
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_no_header_row.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(3 == new_reader.rows());
        };

        "sniff limit"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"join_a.csv", "sniff_limit.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          -- FROM PYTHON CSVKIT --
//          ['a', 'b', 'c', 'a;b;c'],
//          ['1', 'b', 'c', '1;2;3'],
//          ['2', 'b', 'c', ''],
//          ['3', 'b', 'c', ''],

            expect("a,b,c,a;b;c\n1,b,c,1;2;3\n2,b,c,\n3,b,c,\n" == cout_buffer.str());
        };

        "both left and right"_test = [&] {
            auto args_copy = args;
            args_copy.right_join = args_copy.left_join = true;
            args_copy.columns = "1";
            expect(throws([&] { csvjoin::join_wrapper(args_copy); }));
        };

        "no join column names"_test = [&] {
            auto args_copy = args;
            args_copy.right_join = true;
            expect(throws([&] { csvjoin::join_wrapper(args_copy); }));
        };

        "join column names must match the number of files"_test = [&] {
            auto args_copy = args;
            args_copy.right_join = true;
            args_copy.columns = "1,1";
            args_copy.files = std::vector<std::string>{"file","file2","file3"};
            expect(throws([&] { csvjoin::join_wrapper(args_copy); }));
        };

        "csvjoin sequential/union strings"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"h1\nabc","h2\nabc\ndef","h3\n\nghi"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::typify::csvjoin_source_option::csvjoin_string_source))
            expect(cout_buffer.str() == "h1,h2,3\r\nabc,abc,\r\n,def,ghi\r\n" || cout_buffer.str() == "h1,h2,h3\nabc,abc,\n,def,ghi\n");
        };

        "COMPLEX OUTER JOINS"_test = [&] {
            using namespace csvjoin::detail::typify;

            auto args_copy = args;
            args_copy.outer_join = true;
            args_copy.no_header = true;
            {
                args_copy.columns = "1,1,1";
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,c,d,a_2,b_2,c_2,d_2,e,a_3,b_3,c_3,d_3,e_2
2024-12-14,False,4,,2024-12-14,True,7,,0:00:02,,,,,
2024-12-15,True,5,,2024-12-15,False,6,,0:00:01,2024-12-15,False,8,,0:00:03
,,,,,,,,,2024-12-16,,9,,0:00:04
)");
            }
            {
                args_copy.columns = "1,1,1";
                args_copy.no_inference = true;
                args_copy.blanks = true;
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,c,d,a_2,b_2,c_2,d_2,e,a_3,b_3,c_3,d_3,e_2
14/12/2024,F,4,N/A,14/12/2024,T,7,N/A,2s,,,,,
15/12/2024,T,5,N/A,15/12/2024,F,6,N/A,1s,15/12/2024,F,8,N/A,3s
,,,,,,,,,16/12/2024,,9,N/A,4s
)");
            }
            {
                args_copy.columns = "1,1,1";
                args_copy.no_inference = true;
                args_copy.blanks = false;
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
//                std::cerr << cout_buffer.str();
                expect(cout_buffer.str() == R"(a,b,c,d,a_2,b_2,c_2,d_2,e,a_3,b_3,c_3,d_3,e_2
14/12/2024,F,4,,14/12/2024,T,7,,2s,,,,,
15/12/2024,T,5,,15/12/2024,F,6,,1s,15/12/2024,F,8,,3s
,,,,,,,,,16/12/2024,,9,,4s
)");
            }
            {
                args_copy.columns = "1,1,1";
                args_copy.no_inference = false;
                args_copy.blanks = true;
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,c,d,a_2,b_2,c_2,d_2,e,a_3,b_3,c_3,d_3,e_2
2024-12-14,False,4,N/A,2024-12-14,True,7,N/A,0:00:02,,,,,
2024-12-15,True,5,N/A,2024-12-15,False,6,N/A,0:00:01,2024-12-15,F,8,N/A,0:00:03
,,,,,,,,,2024-12-16,,9,N/A,0:00:04
)");
            }
            {
                //still blanks
                args_copy.columns = "1,1,2";
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
                //std::cerr << cout_buffer.str();
                expect(cout_buffer.str() == R"(a,b,c,d,a_2,b_2,c_2,d_2,e,a_3,b_3,c_3,d_3,e_2
2024-12-14,False,4,N/A,2024-12-14,True,7,N/A,0:00:02,,,,,
2024-12-15,True,5,N/A,2024-12-15,False,6,N/A,0:00:01,,,,,
,,,,,,,,,2024-12-15,F,8,N/A,0:00:03
,,,,,,,,,2024-12-16,,9,N/A,0:00:04
)");
            }

        };
    };

}
