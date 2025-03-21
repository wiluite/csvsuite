///
/// \file   suite/test/csvStack_test.cpp
/// \author wiluite
/// \brief  Tests for the csvStack utility.

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
        std::string groups = "empty";
        std::string group_name;
        bool filenames {false};
        bool check_integrity = {true};
    };

    struct csvStack_args : tf::common_args, tf::output_args, csvstack_specific_args {};

    "glob"_test = [] {
        struct Args : csvStack_args {
        } args;

        args.files = std::vector<std::string>{"examples/dummy*.csv"};

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == R"(a,b,c,d
1,2,3,
1,2,3,
1,2,3,
1,4,5,
0,2,3,
1,2,3,
1,2,3,4
)" or
        cout_buffer.str() == R"(a,b,c,d
1,2,3,
1,2,3,
1,2,3,4
0,2,3,
1,2,3,
1,4,5,
1,2,3,
)");
    };

    "skip lines"_test = [] {
        struct Args : csvStack_args {
        } args;

        args.files = std::vector<std::string>{"examples/test_skip_lines.csv", "examples/test_skip_lines.csv"};
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
        struct Args : csvStack_args {
        } args;

        std::ifstream ifs("examples/test_skip_lines.csv");
        stdin_subst new_cin(ifs);
        args.files = std::vector<std::string>{"examples/test_skip_lines.csv", "_"};
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
        struct Args : csvStack_args {
        } args;

        args.files = std::vector<std::string>{"examples/dummy.csv"};

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "multiple file stack col"_test = [] {
        struct Args : csvStack_args {
        } args;

        args.files = std::vector<std::string>{"examples/dummy.csv", "examples/dummy_col_shuffled.csv"};
        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            )

//      a,b,c
//      1,2,3
//      1,2,3

            expect("a,b,c\n1,2,3\n1,2,3\n" == cout_buffer.str());
        }

        args.files = std::vector<std::string>{"examples/dummy_col_shuffled.csv", "examples/dummy.csv"};
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
        struct Args : csvStack_args {
        } args;

        args.files = std::vector<std::string>{"examples/dummy.csv", "examples/dummy_col_shuffled_ragged.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        );

//      a,b,c,d
//      1,2,3,
//      1,2,3,4

        expect("a,b,c,d\n1,2,3,\n1,2,3,4\n" == cout_buffer.str());
    };

    "multiple file stack col ragged stdin"_test = [] {
        struct Args : csvStack_args {
        } args;

        {
            std::ifstream ifs("examples/dummy.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"_", "examples/dummy_col_shuffled_ragged.csv"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            );

//      a,b,c,d
//      1,2,3,
//      1,2,3,4

            expect("a,b,c,d\n1,2,3,\n1,2,3,4\n" == cout_buffer.str());
        }
        {
            std::ifstream ifs("examples/dummy.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"examples/dummy_col_shuffled_ragged.csv", "_"};

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
        struct Args : csvStack_args {
            Args() {
                group_name= "foo";
                groups = "asd,sdf";
            }
        } args;

        args.files = std::vector<std::string>{"examples/dummy.csv", "examples/dummy2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      foo,a,b,c
//      asd,1,2,3
//      sdf,1,2,3

        expect("foo,a,b,c\nasd,1,2,3\nsdf,1,2,3\n" == cout_buffer.str());
    };

    "filenames grouping"_test = [] {
        struct Args : csvStack_args {
            Args() {
                group_name= "path";
                filenames = true;
            }
        } args;

        args.files = std::vector<std::string>{"examples/dummy.csv", "examples/dummy2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      path,a,b,c
//      dummy.csv,1,2,3
//      dummy2.csv,1,2,3

        expect("path,a,b,c\nexamples/dummy.csv,1,2,3\nexamples/dummy2.csv,1,2,3\n" == cout_buffer.str());
    };

    "no header row basic"_test = [] {
        struct Args : csvStack_args {
            Args() {
                no_header = true;
            }
        } args;

        args.files = std::vector<std::string>{"examples/no_header_row.csv", "examples/no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      a,b,c
//      1,2,3
//      4,5,6

        expect("a,b,c\n1,2,3\n4,5,6\n" == cout_buffer.str());
    };

    "no header row basic stdin"_test = [] {
        struct Args : csvStack_args {
            Args() {
                no_header = true;
            }
        } args;

        {
            std::ifstream ifs("examples/no_header_row.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"_", "examples/no_header_row2.csv"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvstack::stack<notrimming_reader_type>(args)
            )

//      a,b,c
//      1,2,3
//      4,5,6

            expect("a,b,c\n1,2,3\n4,5,6\n" == cout_buffer.str());
        }
        {
            std::ifstream ifs("examples/no_header_row.csv");
            stdin_subst new_cin(ifs);
            args.files = std::vector<std::string>{"examples/no_header_row2.csv", "_"};

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
        struct Args : csvStack_args {
            Args() {
                no_header = true;
                groups = "foo,bar";
                group_name = "hey";
            }
        } args;

        args.files = std::vector<std::string>{"examples/dummy.csv", "examples/dummy3.csv"};
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
        struct Args : csvStack_args {
            Args() {
                no_header = true;
                filenames = true;
            }
        } args;

        args.files = std::vector<std::string>{"examples/no_header_row.csv", "examples/no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      group,a,b,c
//      no_header_row.csv,1,2,3
//      no_header_row2.csv,4,5,6

        expect("group,a,b,c\nexamples/no_header_row.csv,1,2,3\nexamples/no_header_row2.csv,4,5,6\n" == cout_buffer.str());
    };

    "grouped filenames and named column"_test = [] {
        struct Args : csvStack_args {
            Args() {
                no_header = true;
                filenames = true;
                group_name = "hello";
            }
        } args;

        args.files = std::vector<std::string>{"examples/no_header_row.csv", "examples/no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

//      hello,a,b,c
//      no_header_row.csv,1,2,3
//      no_header_row2.csv,4,5,6

        expect("hello,a,b,c\nexamples/no_header_row.csv,1,2,3\nexamples/no_header_row2.csv,4,5,6\n" == cout_buffer.str());
    };

    "linenumbers and groups"_test = [] {
        struct Args : csvStack_args {
            Args() {
                no_header = true;
                filenames = false;
                group_name = "hello";
                groups = "foo,bar";
                files = std::vector<std::string>{"examples/no_header_row.csv", "examples/no_header_row2.csv"};
                linenumbers = true; 
            }
        } args;
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvstack::stack<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == R"(line_number,hello,a,b,c
1,foo,1,2,3
2,bar,4,5,6
)"); 
    };

    "max field size"_test = [] {
        struct Args : csvStack_args {
            Args() { files = std::vector<std::string>{"examples/test_field_size_limit.csv"}; /*maxfieldsize = 100;*/ }
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
