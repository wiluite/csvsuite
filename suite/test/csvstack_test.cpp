///
/// \file   test/csvstack_test.cpp
/// \author wiluite
/// \brief  Tests for the csvstack utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvStack.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_reader_macros.h"
#include "test_max_field_size_macros.h"
#include "stdin_subst.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call) std::stringstream cout_buffer;                        \
                                             {                                                     \
                                                 redirect(cout)                                    \
                                                 redirect_cout cr(cout_buffer.rdbuf());            \
                                                 call;                                             \
                                             }

int main() {
    using namespace boost::ut;
    namespace tf = csvsuite::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    struct csvstack_specific_args {
        std::vector<std::string> files;
        std::string groups;
        std::string group_name;
        bool filenames {false};
        bool check_integrity = {true};
    };

    "skip lines"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
        } args;

        args.files = std::vector<std::string>{"test_skip_lines.csv", "test_skip_lines.csv"};
        args.skip_lines = 3;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      a,b,c
//      1,2,3
//      1,2,3

        expect("a,b,c\n1,2,3\n1,2,3\n" == cout_buffer.str());
    };

    "skip lines stdin"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
        } args;

        std::ifstream ifs("test_skip_lines.csv");
        stdin_subst new_cin(ifs);
        args.files = std::vector<std::string>{"test_skip_lines.csv", "_"};
        args.skip_lines = 3;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      a,b,c
//      1,2,3
//      1,2,3

        expect("a,b,c\n1,2,3\n1,2,3\n" == cout_buffer.str());
    };

    "single file stack"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
        } args;

        args.files = std::vector<std::string>{"dummy.csv"};

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "multiple file stack col"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy_col_shuffled.csv"};
        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            )

//      a,b,c
//      1,2,3
//      1,2,3

            expect("a,b,c\n1,2,3\n1,2,3\n" == cout_buffer.str());
        }

        args.files = std::vector<std::string>{"dummy_col_shuffled.csv", "dummy.csv"};
        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            )

//      a,b,c
//      1,2,3
//      1,2,3

            expect("b,c,a\n2,3,1\n2,3,1\n" == cout_buffer.str());
        }
    };

    "multiple file stack col ragged"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy_col_shuffled_ragged.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        );

//      a,b,c,d
//      1,2,3,
//      1,2,3,4
        
        expect("a,b,c,d\n1,2,3,\n1,2,3,4\n" == cout_buffer.str());
    };

    "multiple file stack col ragged stdin"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
        } args;

        {
            std::ifstream ifs("dummy.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"_", "dummy_col_shuffled_ragged.csv"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            );

//      a,b,c,d
//      1,2,3,
//      1,2,3,4

            expect("a,b,c,d\n1,2,3,\n1,2,3,4\n" == cout_buffer.str());
        }
        {
            std::ifstream ifs("dummy.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"dummy_col_shuffled_ragged.csv", "_"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            );

//      b,c,a,d
//      2,3,1,4
//      2,3,1,

            expect("b,c,a,d\n2,3,1,4\n2,3,1,\n" == cout_buffer.str());
        }

    };


    "explicit grouping"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                group_name= "foo";
                groups = "asd,sdf";
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      foo,a,b,c
//      asd,1,2,3
//      sdf,1,2,3
        
        expect("foo,a,b,c\nasd,1,2,3\nsdf,1,2,3\n" == cout_buffer.str());
    };

    "filenames grouping"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                group_name= "path";
                filenames = true;
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      path,a,b,c
//      dummy.csv,1,2,3
//      dummy2.csv,1,2,3
        
        expect("path,a,b,c\ndummy.csv,1,2,3\ndummy2.csv,1,2,3\n" == cout_buffer.str());
    };

    "no header row basic"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                no_header = true;
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      a,b,c
//      1,2,3
//      4,5,6
        
        expect("a,b,c\n1,2,3\n4,5,6\n" == cout_buffer.str());
    };

    "no header row basic stdin"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                no_header = true;
            }
        } args;

        {
            std::ifstream ifs("no_header_row.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"_", "no_header_row2.csv"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            )

//      a,b,c
//      1,2,3
//      4,5,6

            expect("a,b,c\n1,2,3\n4,5,6\n" == cout_buffer.str());
        }
        {
            std::ifstream ifs("no_header_row.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"no_header_row2.csv", "_"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            )

//      a,b,c
//      4,5,6
//      1,2,3

            expect("a,b,c\n4,5,6\n1,2,3\n" == cout_buffer.str());
        }

    };

    "grouped manual and named column"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                no_header = true;
                groups = "foo,bar";
                group_name = "hey"; 
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy3.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      hey,a,b,c
//      foo,a,b,c
//      foo,1,2,3
//      bar,a,b,c
//      bar,1,2,3
//      bar,1,4,5
        
        expect("hey,a,b,c\nfoo,a,b,c\nfoo,1,2,3\nbar,a,b,c\nbar,1,2,3\nbar,1,4,5\n" == cout_buffer.str());
    };

    "grouped filenames"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                no_header = true;
                filenames = true;
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      group,a,b,c
//      no_header_row.csv,1,2,3
//      no_header_row2.csv,4,5,6
        
        expect("group,a,b,c\nno_header_row.csv,1,2,3\nno_header_row2.csv,4,5,6\n" == cout_buffer.str());
    };

    "grouped filenames and named column"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() {
                no_header = true;
                filenames = true;
                group_name = "hello";
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      hello,a,b,c
//      no_header_row.csv,1,2,3
//      no_header_row2.csv,4,5,6
        
        expect("hello,a,b,c\nno_header_row.csv,1,2,3\nno_header_row2.csv,4,5,6\n" == cout_buffer.str());
    };


    "max field size"_test = [] {
        struct Args : tf::common_args, tf::output_args, csvstack_specific_args {
            Args() { files = std::vector<std::string>{"test_field_size_limit.csv"}; /*maxfieldsize = 100;*/ }
        } args;

        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )
        }));

        using namespace z_test;

        Z_CHECK1(csvstack::stack<test_reader_r1>(args), skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK1(csvstack::stack<test_reader_r2>(args), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK1(csvstack::stack<test_reader_r3>(args), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK1(csvstack::stack<test_reader_r4>(args), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK1(csvstack::stack<test_reader_r5>(args), skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        Z_CHECK1(csvstack::stack<test_reader_r6>(args), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
    };
}
