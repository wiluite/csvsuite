///
/// \file   suite/test/csvLook_test.cpp
/// \author wiluite
/// \brief  Tests for the csvLook utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvLook.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_runner_macros.h"
#include "test_max_field_size_macros.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT std::stringstream cout_buffer; \
{                                                                     \
    redirect(cout)                                                    \
    redirect_cout cr(cout_buffer.rdbuf());                            \
    csvlook::look(r, args);                                           \
}

int main() {
    using namespace boost::ut;
    namespace tf = csvsuite::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    struct csvlook_specific_args {
        mutable unsigned max_columns {max_unsigned_limit};
        unsigned max_column_width {max_unsigned_limit};
        unsigned long max_rows {max_size_t_limit};
        unsigned max_precision {3u};
        bool no_number_ellipsis {false};
        std::string glob_locale {"C"};
        mutable unsigned precision_locally{};
    };

    "runs"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_utf8.csv"; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | foo | bar | baz |
//      | --- | --- | --- |
//      |   1 |   2 | 3   |
//      |   4 |   5 | ʤ   |

        expect(cout_buffer.str() == "| foo | bar | baz | \n| --- | --- | --- | \n|   1 |   2 | 3   | \n|   4 |   5 | ʤ   | \n");
    };

    "simple"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/dummy3.csv"; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      |    a | b | c |
//      | ---- | - | - |
//      | True | 2 | 3 |
//      | True | 4 | 5 |

        expect(cout_buffer.str() == "|    a | b | c | \n| ---- | - | - | \n| True | 2 | 3 | \n| True | 4 | 5 | \n");
    };

    "encoding"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_latin1.csv"; encoding = "latin1"; }
        } args;

        std::stringstream cout_buffer;
        redirect(cout)
        redirect_cout cr(cout_buffer.rdbuf());
        test_reader_configurator_and_runner(args, csvlook::look)

//      | a | b | c |
//      | - | - | - |
//      | 1 | 2 | 3 |
//      | 4 | 5 | © |
        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| 1 | 2 | 3 | \n| 4 | 5 | © | \n");
    };

    "no blanks"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/blanks.csv"; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | a | b | c | d | e | f |
//      | - | - | - | - | - | - |
//      |   |   |   |   |   |   |

        std::string result = "| a | b | c | d | e | f | \n| - | - | - | - | - | - | \n|   |   |   |   |   |   | \n";
        expect(cout_buffer.str() == result);
    };

    "blanks"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/blanks.csv"; blanks = true;}
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | a | b  | c   | d    | e    | f |
//      | - | -- | --- | ---- | ---- | - |
//      |   | NA | N/A | NONE | NULL | . |

        expect(cout_buffer.str() == "| a | b  | c   | d    | e    | f | \n| - | -- | --- | ---- | ---- | - | \n|   | NA | N/A | NONE | NULL | . | \n");
    };

    "no header row"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/no_header_row3.csv"; no_header = true; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | a | b | c |
//      | - | - | - |
//      | 1 | 2 | 3 |
//      | 4 | 5 | 6 |

        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| 1 | 2 | 3 | \n| 4 | 5 | 6 | \n");
    };

    // TODO: add just unicode test
    "unicode bom"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_utf8_bom.csv"; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | foo | bar | baz |
//      | --- | --- | --- |
//      |   1 |   2 | 3   |
//      |   4 |   5 | ʤ   |

        expect(cout_buffer.str() == "| foo | bar | baz | \n| --- | --- | --- | \n|   1 |   2 | 3   | \n|   4 |   5 | ʤ   | \n");
    };

    "linenumbers"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/dummy3.csv"; linenumbers = true;}
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | line_numbers |    a | b | c |
//      | ------------ | ---- | - | - |
//      |            1 | True | 2 | 3 |
//      |            2 | True | 4 | 5 |

        expect(cout_buffer.str() == "| line_numbers |    a | b | c | \n| ------------ | ---- | - | - | \n|            1 | True | 2 | 3 | \n|            2 | True | 4 | 5 | \n");
    };

    "no_inference"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/dummy3.csv"; no_inference = true; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | a | b | c |
//      | - | - | - |
//      | 1 | 2 | 3 |
//      | 1 | 4 | 5 |

        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| 1 | 2 | 3 | \n| 1 | 4 | 5 | \n");
    };

    "max rows"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/dummy.csv"; max_rows = 0; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | a | b | c |
//      | - | - | - |
//      | . | . | . |

        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| . | . | . | \n");
    };

    "max columns"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/dummy.csv"; max_columns = 1; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      |    a | ... |
//      | ---- | --- |
//      | True | ... |

        expect(cout_buffer.str() == "|    a | ... |\n| ---- | --- |\n| True | ... |\n");
    };

    "max columns width"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/dummy4.csv"; max_column_width = 4; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      |    a | b | c |
//      | ---- | - | - |
//      | F... | 2 | 3 |

        expect(cout_buffer.str() == "|    a | b | c | \n| ---- | - | - | \n| F... | 2 | 3 | \n");
    };

    "max precision"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_precision.csv"; max_precision = 0; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      |  a |
//      | -- |
//      | 1… |

        expect(cout_buffer.str() == "|  a | \n| -- | \n| 1… | \n");
    };

    "no number ellipsis"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_precision.csv"; no_number_ellipsis = true;}
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      |     a |
//      | ----- |
//      | 1.235 |

        expect(cout_buffer.str() == "|     a | \n| ----- | \n| 1.235 | \n");
    };

    "max precision no number ellipsis"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_precision.csv"; max_precision = 0; no_number_ellipsis = true; }
        } args;

        notrimming_reader_type r (args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      | a |
//      | - |
//      | 1 |

        expect(cout_buffer.str() == "| a | \n| - | \n| 1 | \n");
    };

    "max field size"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, csvlook_specific_args {
            Args() { file = "examples/test_field_size_limit.csv"; maxfieldsize = 100; }
        } args;

        csv_co::reader<> r (args.file);
        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT}));

        using namespace z_test;
        Z_CHECK(csvlook::look, notrimming_reader_type, skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK(csvlook::look, notrimming_reader_type, skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

        Z_CHECK(csvlook::look, notrimming_reader_type, skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK(csvlook::look, notrimming_reader_type, skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

        Z_CHECK(csvlook::look, notrimming_reader_type, skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        Z_CHECK(csvlook::look, notrimming_reader_type, skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
    };
}
