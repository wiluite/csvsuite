/// \file   test/csvClean_mac_newlines_test.cpp
/// \author wiluite
/// \brief  One of the tests for the csvClean utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"
#include "../csvClean.cpp"
#include "common_args.h"

int main() {

    using namespace boost::ut;

#if defined (WIN32)
    cfg<override> ={.colors={.none="", .pass="", .fail=""}};
#endif

    "mac newlines"_test = [&] () {
        // We now handle with a Mac's line-breaking file with a Mac's line-breaking multiline field.
        // Output file is a native system line-breaking one, still having 3 rows and 1 multiline field
        // with native line-breaking.
#if !defined (__APPLE__)
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "mac_newlines.csv"; }
            bool dry_run {false};
        } args;

        using namespace csv_co;
        using mac_reader_type = csv_co::reader<csv_co::trim_policy::no_trimming, double_quotes, comma_delimiter, line_break<'\r'>>;
        mac_reader_type r (args.file);
        csvclean::clean(r, args);

        expect(nothrow([&] {
            notrimming_reader_type out {std::filesystem::path{"mac_newlines_out.csv"}};
            std::string out_str;
            auto rows = 0u;
            out.run_rows([&](auto & row){
                rows++;
                for (auto & elem : row) {
                    out_str += elem.operator csv_co::cell_string();
                }
            });
            expect(rows == 3);
            expect(out_str == "abc123\"Once upon\r\na time\"56" || out_str == "abc123\"Once upon\na time\"56");
        }));
        expect(nothrow([&](){std::filesystem::remove(std::filesystem::path{"mac_newlines_out.csv"});}));
#endif
    };

}
