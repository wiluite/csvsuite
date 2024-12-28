/// \file   test/csvClean_changes_character_encoding_test.cpp
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
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "changes character encoding"_test = [] () mutable {
        struct Args : csvsuite::test_facilities::single_file_arg, csvsuite::test_facilities::common_args {
            Args() { file = "examples/test_latin1.csv"; maxfieldsize = max_unsigned_limit;} // TODO: Why does it requires max_unsigned_limit for test to pass?
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        // TODO: It should be done inside assertCleaned. -e latin1 is needed simply to parse and also to use transformation
        csvclean::clean(r, args);
        expect(nothrow([&](){
            using namespace csvsuite::cli;
            std::u8string u8str = u8"4,5,©";
            csvsuite::test_facilities::assertCleaned ("test_latin1", {"a,b,c", "1,2,3", encoding::iconv("latin1", "utf-8", std::string(u8str.begin(), u8str.end()))}, {});
        }));
    };

}
