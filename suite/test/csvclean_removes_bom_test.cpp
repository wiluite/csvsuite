///
/// \file   test/csvclean_removes_bom_test.cpp
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
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "removes bom"_test = [] () mutable {
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "test_utf8_bom.csv"; maxfieldsize = max_unsigned_limit; }
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        csvclean::clean(r, args);
        expect(nothrow([&](){
            using namespace csvsuite::cli;
            csvsuite::test_facilities::assertCleaned ("test_utf8_bom", {"foo,bar,baz", "1,2,3", "4,5,ʤ"}, {});
        }));
    };

}
