/// \file   test/csvclean_bad_skip_lines_test.cpp
/// \author wiluite
/// \brief  One of the tests for the csvclean utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvclean.cpp"
#include "csvclean_test_funcs.h"
#include "common_args.h"

int main() {

    using namespace boost::ut;

#if defined (WIN32)
    cfg<override> ={.colors={.none="", .pass="", .fail=""}};
#endif
    "bad skip lines"_test = [&] () mutable {
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "bad_skip_lines.csv"; skip_lines = 3;}
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        csvclean::clean(r, args);
        expect(nothrow([&](){
            csvsuite::test_facilities::assertCleaned ("bad_skip_lines",
                                                      {"column_a,column_b,""column_c","0,mixed types.... uh oh,17"},
                                                      {"line_number,msg,column_a,column_b,column_c",
                                                     R"(1,"Expected 3 columns, found 4 columns",1,27,,I'm too long!)",
                                                     R"(2,"Expected 3 columns, found 2 columns",,I'm too short!)"}
            );
        }));
    };


}
