/// \file   suite/test/csvClean_dry_run_test.cpp
/// \author wiluite
/// \brief  One of the tests for the csvClean utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvClean.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_reader_macros.h"
#include "test_max_field_size_macros.h"

int main() {

    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "dry_run"_test = [] () mutable {
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "examples/bad.csv"; }
            bool dry_run {true};
        } args;

        notrimming_reader_type r (args.file);

        std::stringstream cerr_buffer;
        {
            redirect(cerr)
            redirect_cerr cr (cerr_buffer.rdbuf());
            csvclean::clean(r, args);
        }

        expect(nothrow([&] {
              expect(!std::filesystem::exists(std::filesystem::path{"bad_err.csv"}));
              expect(!std::filesystem::exists(std::filesystem::path{"bad_out.csv"}));
              std::string temp;
              std::getline(cerr_buffer, temp);
              expect(temp.substr(0,6) == "Line 1");
              std::getline(cerr_buffer, temp);
              expect(temp.substr(0,6) == "Line 2");
        }));
    };

    "max field size"_test = [&] () mutable {
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "examples/test_field_size_limit.csv";}
            bool dry_run {false};
        } args;

        using namespace z_test;
        Z_CHECK(csvclean::clean, test_reader_r1, skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK(csvclean::clean, test_reader_r2, skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

        Z_CHECK(csvclean::clean, test_reader_r3, skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK(csvclean::clean, test_reader_r4, skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

        Z_CHECK(csvclean::clean, test_reader_r5, skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        Z_CHECK(csvclean::clean, test_reader_r6, skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        std::filesystem::remove(std::filesystem::path{"test_field_size_limit_out.csv"});
    };


}
