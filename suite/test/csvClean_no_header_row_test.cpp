/// \file   test/csvClean_no_header_row_test.cpp
/// \author wiluite
/// \brief  One of the tests for the csvClean utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvClean.cpp"
#include "csvClean_test_funcs.h"
#include "common_args.h"

int main() {

    using namespace boost::ut;

#if defined (WIN32)
    cfg<override> ={.colors={.none="", .pass="", .fail=""}};
#endif

    "no header row"_test = [] () mutable {
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "examples/no_header_row.csv"; }
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        csvclean::clean(r, args);
        expect(nothrow([&](){
            csvsuite::test_facilities::assertCleaned ("no_header_row", {"1,2,3"}, {});
        }));
    };

}
