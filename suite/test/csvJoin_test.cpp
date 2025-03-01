///
/// \file   suite/test/csvJoin_test.cpp
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

using namespace ::csvsuite::cli::compare;

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

        "glob"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"examples/dummy_col*.csv"};
            args_copy.no_inference = true;
            args_copy.columns = "1";
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
            expect(cout_buffer.str() == R"(b,c,a,c2,a2,d
2,3,1,3,1,4
)");
        };

        "sequential"_test = [&] {
            {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a2,b2,c2
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
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a2,b2,c2,a2_2,b2_2,c2_2
1,b,c,1,b,c,1,b,c
2,b,c,1,b,c,1,b,c
3,b,c,4,b,c,4,b,c
)");
                notrimming_reader_type new_reader (cout_buffer.str());
                expect(4 == new_reader.rows());
            }
            "max field size in this mode"_test = [&] {
                auto args_copy = args;
                args_copy.files = std::vector<std::string>{"examples/test_field_size_limit.csv", "examples/test_field_size_limit.csv"};
                using namespace z_test;
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "inner"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b2,c2
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
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b2,c2,b2_2,c2_2
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
                args_copy.files = std::vector<std::string>{"examples/test_field_size_limit.csv", "examples/test_field_size_limit.csv"};
                args_copy.columns = "1";
                using namespace z_test;

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
            // fixed UB: runtime error: reference binding to null pointer of type 'const value_type' (aka 'const std::vector<std::string>')
            "indices absent in rest files"_test = [&] {
                auto args_copy = args;
                args_copy.columns = "3,1,1";
                args_copy.files = std::vector<std::string>{"a,b,c\n1,2,3\n4,5,", "a,b\n2,b", "a,b\n3,c\n"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,c,b2,b2_2
)");
            };
        };

        "left"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.left_join = true;
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b2,c2
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
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b2,c2,b2_2,c2_2
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
                args_copy.files = std::vector<std::string>{"examples/test_field_size_limit.csv", "examples/test_field_size_limit.csv"};
                args_copy.columns = "1";
                args_copy.left_join = true;
                using namespace z_test;

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "right"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.right_join = true;
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b2,c2
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
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,b2,c2,b2_2,c2_2
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
                args_copy.files = std::vector<std::string>{"examples/test_field_size_limit.csv", "examples/test_field_size_limit.csv"};
                args_copy.columns = "1";
                args_copy.right_join = true;
                using namespace z_test;

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")

            };
        };

        "right indices"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1,4";
            args_copy.right_join = true;
            args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
            expect(cout_buffer.str() == R"(a,b,c,d,e,f,b2,c2
,,,,,,,
)");
            notrimming_reader_type new_reader (cout_buffer.str());
            expect(2 == new_reader.rows());

            "max field size in this mode"_test = [&] {
                args_copy.files = std::vector<std::string>{"examples/test_field_size_limit.csv", "examples/test_field_size_limit.csv"};
                args_copy.columns = "1,3";
                using namespace z_test;

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.");
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };

            "indices absent in rest files"_test = [&] {
                args_copy.columns = "3,1,1";
                assert(args_copy.right_join);
                args_copy.files = std::vector<std::string>{"a,b,c\n1,2,3\n4,5,", "a,b\n2,b", "a,b\n3,c\n"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,b2,a2,b2_2
3,c,,1,2
)");
            };
        };

        "outer"_test = [&] {
            {
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.outer_join = true;
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a2,b2,c2
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
                auto args_copy = args;
                args_copy.columns = "a";
                args_copy.outer_join = true;
                args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_b.csv", "examples/join_b.csv"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))
                expect(cout_buffer.str() == R"(a,b,c,a2,b2,c2,a2_2,b2_2,c2_2
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
                args_copy.files = std::vector<std::string>{"examples/test_field_size_limit.csv", "examples/test_field_size_limit.csv"};
                args_copy.columns = "1";
                args_copy.outer_join = true;
                using namespace z_test;

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 12, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::has_header, 13, "FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.")
                Z_CHECK0(csvjoin::join_wrapper(args_copy), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
            };
        };

        "single"_test = [&] {
            auto args_copy = args;
            args_copy.no_inference = true;
            args_copy.files = std::vector<std::string>{"examples/dummy.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c
//          1,2,3

            expect("a,b,c\n1,2,3\n" == cout_buffer.str());
        };

        "no blanks"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"examples/blanks.csv", "examples/blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c,d,e,f,a2,b2,c2,d2,e2,f2
//          ,,,,,,,,,,,

            expect("a,b,c,d,e,f,a2,b2,c2,d2,e2,f2\n,,,,,,,,,,,\n" == cout_buffer.str());
        };

        "blanks"_test = [&] {
            auto args_copy = args;
            args_copy.blanks = true;
            args_copy.files = std::vector<std::string>{"examples/blanks.csv", "examples/blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c,d,e,f,a2,b2,c2,d2,e2,f2
//          ,NA,N/A,NONE,NULL,.,,NA,N/A,NONE,NULL,.

            expect("a,b,c,d,e,f,a2,b2,c2,d2,e2,f2\n,NA,N/A,NONE,NULL,.,,NA,N/A,NONE,NULL,.\n" == cout_buffer.str());
        };

        "no header row"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1";
            args_copy.no_header = true;
            args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/join_no_header_row.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(3 == new_reader.rows());
        };

        "sniff limit"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"examples/join_a.csv", "examples/sniff_limit.csv"};
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
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
            expect(cout_buffer.str() == "h1,h2,3\r\nabc,abc,\r\n,def,ghi\r\n" || cout_buffer.str() == "h1,h2,h3\nabc,abc,\n,def,ghi\n");
        };

        "COMPLEX OUTER JOINS"_test = [&] {
            using namespace csvjoin::detail;

            auto args_copy = args;
            args_copy.outer_join = true;
            args_copy.no_header = true;
            {
                args_copy.columns = "1,1,1";
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
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
                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
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
                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
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
                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
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

                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
2024-12-14,False,4,N/A,2024-12-14,True,7,N/A,0:00:02,,,,,
2024-12-15,True,5,N/A,2024-12-15,False,6,N/A,0:00:01,,,,,
,,,,,,,,,2024-12-15,F,8,N/A,0:00:03
,,,,,,,,,2024-12-16,,9,N/A,0:00:04
)");
            }
            {
                // blanks and honest print after honest last typifying:
                args_copy.honest_outer_join = true;
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))

                // now we have the same except for type-aware printing!
                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
14/12/2024,F,4,N/A,14/12/2024,T,7,N/A,2s,,,,,
15/12/2024,T,5,N/A,15/12/2024,F,6,N/A,1s,,,,,
,,,,,,,,,15/12/2024,F,8,N/A,3s
,,,,,,,,,16/12/2024,,9,N/A,4s
)");
            }
            {
                // NO blanks and honest print after honest last typifying:
                args_copy.blanks = false;
                args_copy.files = {"14/12/2024,F,4,N/A\n15/12/2024,T,5,N/A", "15/12/2024,F,6,N/A,1s\n14/12/2024,T,7,N/A,2s", "15/12/2024,F,8,N/A,3s\n16/12/2024,,9,N/A,4s"};         
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))

                // now we have again have the type-aware printing!
                expect(cout_buffer.str() == R"(a,b,c,d,a2,b2,c2,d2,e,a2_2,b2_2,c2_2,d2_2,e2
2024-12-14,False,4,,2024-12-14,True,7,,0:00:02,,,,,
2024-12-15,True,5,,2024-12-15,False,6,,0:00:01,,,,,
,,,,,,,,,2024-12-15,False,8,,0:00:03
,,,,,,,,,2024-12-16,,9,,0:00:04
)");
            }

            "indices absent in rest files"_test = [&] {
                auto args_copy = args;
                args_copy.columns = "3,1,1";
                args_copy.outer_join = true;
                args_copy.files = std::vector<std::string>{"a,b,c\n1,2,3\n", "a,b\n2,b\n", "a,b\n2,c\n"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,c,a2,b2,a2_2,b2_2
True,2,3,,,,
,,,2,b,,
,,,,,2,c
)");
            };

            "empty values in columns of different types"_test = [&] {
                auto args_copy = args;
                args_copy.columns = "1,1";
                args_copy.outer_join = true;
                args_copy.files = std::vector<std::string>{"a,b\n1.0,2\n ,4", "a,b\n,char2\nchar3,char4"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,a2,b2
1.0,2,,
,4,,char2
,,char3,char4
)");
            };
            "empty values in columns of different types, no inference"_test = [&] {
                auto args_copy = args;
                args_copy.columns = "1,1";
                args_copy.outer_join = true;
                args_copy.no_inference = true;
                args_copy.files = std::vector<std::string>{"a,b\n1.0,2\n ,4", "a,b\n,char2\nchar3,char4"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,a2,b2
1.0,2,,
,4,,char2
,,char3,char4
)");
            };

            "empty values in columns of different types, blanks"_test = [&] {
                auto args_copy = args;
                args_copy.columns = "1,1";
                args_copy.outer_join = true;
                args_copy.blanks = true;
                args_copy.files = std::vector<std::string>{"a,b\n1.0,2\n ,4", "a,b\n,char2\nchar3,char4"};
                CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
                expect(cout_buffer.str() == R"(a,b,a2,b2
1.0,2,,
 ,4,,
,,,char2
,,char3,char4
)");
            };

        };

        "std::equal_range"_test = [&] {
            using namespace ::csvsuite::cli::compare;
            auto args_copy = args;
            args_copy.columns = "1";
            using reader_type = skipinitspace_reader_type;
            reader_type reader("h1,h2\n7,aa\n10.01234,m\n11.1,a\n10.01234,b\n10.012,c\n10.01234,d\n,mm\n");
            using element_t = typed_span_t<reader_type>;
            auto const types_blanks = std::get<1>(typify(reader, args, typify_option::typify_without_precisions));

            // Filling in data to sort.
            // It is sufficient to have csv_co::quoted cell_spans in it, because comparison is quite sophisticated and takes it into account
            compromise_table_MxN table(reader, args);
            auto only_key_idx = 0u;
            assert(table[1][0].str() == "10.01234");
            assert(table[6][0].str() == "");

            // typed_span with 10.01234 value
            auto const key_to_search = table[1][only_key_idx];
            auto const empty_key_to_search = table[6][only_key_idx];

            // first sort what to search by a key in key_to_search
            auto compare_fun = obtain_compare_functionality<element_t>(only_key_idx, types_blanks, args);
            std::stable_sort(table.begin(), table.end(), sort_comparator(compare_fun, std::less<>()));

            // search "10.01234" by std::equal_range algo.
            auto p = std::equal_range(table.begin(), table.end(), key_to_search, equal_range_comparator<reader_type>(compare_fun));
            std::string accumulator;
            for (auto i = p.first; i != p.second; ++i) {
                accumulator += (*i)[1];
                accumulator += ' ';
            }
            expect (accumulator == "m b d ");

            accumulator.clear();
            p = std::equal_range(table.begin(), table.end(), empty_key_to_search, equal_range_comparator<reader_type>(compare_fun));
            for (auto i = p.first; i != p.second; ++i) {
                accumulator += (*i)[1];
                accumulator += ' ';
            }
            expect (accumulator == "mm ");
        };

        "correctly compose new/joined header"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1";
            args_copy.outer_join = true;
            args_copy.files = std::vector<std::string>{"a,\"b,b\"\n1,2", "a,\"b,b\"\n2,b", "\"a\",\"b,b\"\n2,c"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::csvjoin_source_option::csvjoin_string_source))
            expect(cout_buffer.str() == R"(a,"b,b",a2,"b,b2",a2_2,"b,b2_2"
True,2,,,,
,,2,b,,
,,,,2,c
)");
            };

    };
}
