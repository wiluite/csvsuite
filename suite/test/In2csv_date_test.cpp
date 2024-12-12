///
/// \file   test/In2csv_test_date.cpp
/// \author wiluite
/// \brief  Additional tests for the In2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../In2csv.cpp"
#include "common_args.h"
#include "strm_redir.h"

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

    "date format"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_date_format.csv"; date_fmt = R"(%d/%m/%Y)";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "test_date_format_converted.csv");
        }));
    };

    "numeric date format default"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_numeric_date_format.csv"; skip_init_space = true; date_fmt = R"(%Y%m%d)"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "test_date_format_converted.csv");
        }));
    };

}

namespace in2csv::detail::xlsx { void impl::convert() {} }
namespace in2csv::detail::xls { void impl::convert() {} }
namespace in2csv::detail::ndjson { void impl::convert() {} }
namespace in2csv::detail::json { void impl::convert() {} }
namespace in2csv::detail::fixed { void impl::convert() {} }
namespace in2csv::detail::dbf { void impl::convert() {} }
namespace in2csv::detail::geojson { void impl::convert() {} }
