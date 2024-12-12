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
    };

    "runs"_test = [] {
        namespace tf = csvsuite::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::output_args, csvjoin_specific_args {
            Args() {
                columns.clear();
            }
        } args;

        "sequential"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(4 == new_reader.rows());

            "max field size in this mode"_test = [&] {
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
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(3 == new_reader.rows());

            "max field size in this mode"_test = [&] {
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
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.left_join = true;
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(5 == new_reader.rows());

            "max field size in this mode"_test = [&] {
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

        "right"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.right_join = true;
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(4 == new_reader.rows());

            "max field size in this mode"_test = [&] {
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

        "right indices"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1,4";
            args_copy.right_join = true;  
            args_copy.files = std::vector<std::string>{"join_a.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

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
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.outer_join = true;
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(6 == new_reader.rows());

            "max field size in this mode"_test = [&] {
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

        "csvjoin union strings"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"h1\nabc","h2\nabc\ndef","h3\n\nghi"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin::detail::typify::csvjoin_source_option::csvjoin_string_source))
            expect(cout_buffer.str() == "h1,h2,3\r\nabc,abc,\r\n,def,ghi\r\n" || cout_buffer.str() == "h1,h2,h3\nabc,abc,\n,def,ghi\n");
        };
    };

}
