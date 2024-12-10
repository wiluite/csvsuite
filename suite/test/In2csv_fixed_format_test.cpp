///
/// \file   test/in2csv_fixed_format_test.cpp
/// \author wiluite
/// \brief  Additional tests for the in2csv utility (fixed format tests).

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../In2csv.cpp"
#include "common_args.h"
#include "strm_redir.h"
#include "stdin_subst.h"
#include "test_reader_macros.h"
#include "../include/in2csv/in2csv_fixed.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call)    \
    std::stringstream cout_buffer;              \
    {                                           \
        redirect(cout)                          \
        redirect_cout cr(cout_buffer.rdbuf());  \
        call;                                   \
    }

int main() {
    using namespace boost::ut;
    namespace tf = csvsuite::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    struct in2csv_specific_args {
        std::string format;
        std::string schema;
        std::string key;
        bool names = false;
        std::string sheet;
        std::string write_sheets;
        bool use_sheet_names = false;
        std::string encoding_xls = "UTF-8";
        std::string d_excel = "none";
        std::string dt_excel = "none";
        bool is1904 = false;
    };

    struct in2csv_args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {};

    auto assert_converted = [](std::string const & source, std::string const & pattern_filename) {
        std::ifstream ifs(pattern_filename);
        std::string result {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        expect(result == source);
    };

    "fixed"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "testfixed"; schema = "testfixed_schema.csv"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "testfixed_converted.csv");
        }));
    };

    "fixed skip lines"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "testfixed_skip_lines"; schema = "testfixed_schema.csv"; skip_lines = 3; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "testfixed_converted.csv");
        }));
    };

    "fixed too many skip lines"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "testfixed_skip_lines"; schema = "testfixed_schema.csv"; skip_lines = 30; }
        } args;
        try {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "testfixed_converted.csv");
        } catch(std::exception const & ex) {
            expect(std::string(ex.what()) == "ValueError: Too many lines to skip.");
        }
    };

    "fixed no inference"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "_"; format = "fixed"; schema = "testfixed_schema_no_inference.csv"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            std::istringstream iss("     1   2 3");
            stdin_subst new_cin(iss);

            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1,2,3\n");
        }));
    };

    "schema decoder bad cases"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "_"; format = "fixed"; schema = "testfixed_schema_no_inference.csv"; no_inference = true; }
        } args;

        try {
            test_reader_r1 r1("foo column\n 0");
            in2csv::detail::fixed::schema_decoder dec(r1);
        } catch (std::exception const & e) {
            expect(std::string(e.what()) == R"(ValueError: A column named "column" must exist in the schema file.)");
        }
        try {
            test_reader_r1 r1("column\n 0");
            in2csv::detail::fixed::schema_decoder dec(r1);
        } catch (std::exception const & e) {
            expect(std::string(e.what()) == R"(ValueError: A column named "start" must exist in the schema file.)");
        }
        try {
            test_reader_r1 r1("start,column\n 1, name");
            in2csv::detail::fixed::schema_decoder dec(r1);
        } catch (std::exception const & e) {
            expect(std::string(e.what()) == R"(ValueError: A column named "length" must exist in the schema file.)");
        }
        try {
            test_reader_r1 r1("length,start,column, another information\n bad_len, 0, name, ###");
            in2csv::detail::fixed::schema_decoder dec(r1);
        } catch (std::exception const & e) {
            expect(std::string(e.what()) == R"(A value of unsupported type ' bad_len' for length.)");
        }
        try {
            test_reader_r1 r1(" another information,length,start,column\n ###, 1,bad_start, name");
            in2csv::detail::fixed::schema_decoder dec(r1);
        } catch (std::exception const & e) {
            expect(std::string(e.what()) == R"(A value of unsupported type 'bad_start' for start.)");
        }
    };

    "zero-based schema decoder"_test = [&] {
        expect(nothrow([&] {
            test_reader_r1 r1(R"(length,start,column,comment
 1, 0,column_name,This is a comment
 5, 1,column_name2,This is another comment
 14,9,column_name3,yet another comment  )");
            in2csv::detail::fixed::schema_decoder dec(r1);
            expect(dec.names().size() == 3);
            expect(dec.names()[0] == "column_name");
            expect(dec.names()[1] == "column_name2");
            expect(dec.names()[2] == "column_name3");
            expect(dec.starts()[0] == 0);
            expect(dec.starts()[1] == 1);
            expect(dec.starts()[2] == 9);
            expect(dec.lengths()[0] == 1);
            expect(dec.lengths()[1] == 5);
            expect(dec.lengths()[2] == 14);
        }));
    };

    "one-based schema decoder"_test = [&] {
        expect(nothrow([&] {
            test_reader_r1 r1(R"(length,start,column
 5, 1,LABEL
 15, 6,LABEL2
)");
            in2csv::detail::fixed::schema_decoder dec(r1);
            expect(dec.names().size() == 2);
            expect(dec.names()[0] == "LABEL");
            expect(dec.names()[1] == "LABEL2");
            expect(dec.starts()[0] == 0);
            expect(dec.starts()[1] == 5);
            expect(dec.lengths()[0] == 5);
            expect(dec.lengths()[1] == 15);
        }));
    };

}

namespace in2csv::detail::xlsx { void impl::convert() {} }
namespace in2csv::detail::xls { void impl::convert() {} }
namespace in2csv::detail::ndjson { void impl::convert() {} }
namespace in2csv::detail::json { void impl::convert() {} }
namespace in2csv::detail::csv { void impl::convert() {} }
namespace in2csv::detail::dbf { void impl::convert() {} }
namespace in2csv::detail::geojson { void impl::convert() {} }
