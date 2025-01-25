///
/// \file   suite/test/In2csv_test.cpp
/// \author wiluite
/// \brief  Tests for the In2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../In2csv.cpp"
#include "common_args.h"
#include "strm_redir.h"
#include "stdin_subst.h"

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
        bool non_flat = false;
        bool names = false;
        std::string sheet;
        std::string write_sheets;
        bool use_sheet_names = false;
        std::string encoding_xls = "UTF-8";
        std::string d_excel = "none";
        std::string dt_excel = "none";
        bool is1904 = false;
    };

    struct In2csv_args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {};

    "exceptions"_test = [] {
        struct Args : In2csv_args {
            Args() = default;
        } args;

        // Neither file name specified nor piping data is coming.
        expect(throws<in2csv::detail::empty_file_and_no_piping_now>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));
#if 0 //TODO: FIX it, it influences to many other tests failed results!
        {
            // now emulate pipe data pull
            stdin_redir sr("stdin_select");
            expect(throws<in2csv::detail::no_format_specified_on_piping>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));
        }
#endif
        args.file = "blah-blah";
        args.format.clear();
        expect(throws<in2csv::detail::no_schema_when_no_extension>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.file = "blah-blah.unknown";
        expect(throws<in2csv::detail::unable_to_automatically_determine_format>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.format = "dbf"; // file blah-blah.unknown not found
        expect(throws<in2csv::detail::file_not_found>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.format = "unknown";
        expect(throws<in2csv::detail::invalid_input_format>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.format = "fixed";
        args.schema = "examples/absent_schema.csv";
        expect(throws<in2csv::detail::schema_file_not_found>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.schema = "examples/foo2.csv";
        expect(throws<in2csv::detail::file_not_found>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.file = "examples/stdin_select"; // all is fine (format, schema and file) - no exception
        args.schema = "examples/testfixed_schema.csv";
        expect(nothrow([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.file = "";
        args.format = "dbf";
        expect(throws<in2csv::detail::dbf_cannot_be_converted_from_stdin>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

    };

    auto get_source = [](std::string const & pattern_filename) {
        std::ifstream ifs(pattern_filename);
        std::string result {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        return result;
    };

    bool locale_support = detect_locale_support();
    if (locale_support) {
        "locale"_test = [&] {
            struct Args : In2csv_args {
                Args() { file = "examples/test_locale.csv"; num_locale = "de_DE"; }
            } args;
            expect(nothrow([&] {
                CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
                expect(cout_buffer.str() == get_source("examples/test_locale_converted.csv"));
            }));
        };
    }

    "no blanks"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/blanks.csv"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/blanks_converted.csv"));
        }));
    };

    "blanks"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/blanks.csv"; blanks = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/blanks.csv"));
        }));
    };

    "null value"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "_"; format = "csv"; null_value = {R"(\N)"}; }
        } args;

        std::istringstream iss("a,b\nn/a,\\N");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
        expect(cout_buffer.str() == "a,b\n,\n");
    };

    "null value blanks"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "_"; format = "csv"; null_value = {R"(\N)"}; blanks = true;}
        } args;

        std::istringstream iss("a,b\nn/a,\\N\n");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
        expect(cout_buffer.str() == "a,b\nn/a,\n");
    };

    "no leading zeroes"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_no_leading_zeroes.csv"; no_leading_zeroes = true;}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/test_no_leading_zeroes.csv"));
        }));
    };

    "date format default"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_date_format.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/test_date_format.csv"));
        }));
    };

    "numeric date format default"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_numeric_date_format.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/test_numeric_date_format.csv"));
        }));
    };

    "date like number"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/date_like_number.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/date_like_number.csv"));
        }));
    };

    "convert csv"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/testfixed_converted.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testfixed_converted.csv"));
        }));
    };

    "convert csv with skip lines"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_skip_lines.csv"; skip_lines = 3; no_inference = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/dummy.csv"));
        }));
    };
    //TODO: fix me (memory error and so on...)
#if defined(__MINGW32__)
    "convert dbf"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/testdbf.dbf";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testdbf_converted.csv"));
        }));
    };
#endif

    "convert json"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/testjson.json";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testjson_converted.csv"));
        }));
    };

    "convert geojson"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_geojson.json"; format = "geojson"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/test_geojson.csv"));
        }));
    };
#if 0
    "convert ndjson"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/testjson_multiline.json"; format = "ndjson"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testjson_multiline_converted.csv"));
        }));
    };
#endif

    "convert nested json"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/testjson_nested.json"; non_flat = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testjson_nested_converted.csv"));
        }));
    };

    // TODO: FOR THIS ONE AND FOR THE NEXT ONE add tests with different data shifts absent at Python kit.
    "convert xls"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test.xls"; d_excel = "2"; dt_excel = "6"; num_locale = "C"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testxls_converted.csv"));
        }));
    };

    "convert xls with sheet"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xls"; sheet = "data"; d_excel = "2"; dt_excel = "6"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testxls_converted.csv"));
        }));
    };

    //TODO : do this test working with 'ʤ' represented in 1251, 1250, 1252 active pages in windows. Check this in Linux as well.
    "convert xls with unicode sheet"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xls"; sheet = "ʤ"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1.0,2.0,3.0\n");
        }));
    };

    "convert xls with skip lines"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_skip_lines.xls"; d_excel = "2"; dt_excel = "6"; skip_lines = 3; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testxls_converted.csv"));
        }));
    };
#if 1 // works with "store to misaligned address 0x6160000048ff for type ...." in zippy.hpp, fno-sanitize=alignment is the key
    "convert xlsx"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test.xlsx"; d_excel = "2"; dt_excel = "6"; is1904 = true;}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testxlsx_converted.csv"));
        }));
    };

    "convert xlsx with sheet"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xlsx"; sheet = "data"; d_excel = "2"; dt_excel = "6"; is1904 = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            //TODO: fixme: do a byte-by-byte comparison, and allow the only 1 byte to be different
#if 0
            expect(cout_buffer.str().size() == get_source("examples/testxlsx_converted.csv").size());
#endif
            expect(cout_buffer.str().size() == get_source("examples/testxlsx_converted.csv").size());
        }));
    };
#endif

    "convert xlsx with unicode sheet"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xlsx"; sheet = "ʤ"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\nTrue,2,3\n");
        }));
    };

    "convert xlsx with skip lines"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/test_skip_lines.xlsx"; d_excel = "2"; dt_excel = "6"; skip_lines = 3; is1904 = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/testxlsx_converted.csv"));
        }));
    };

    "names xls"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xls"; names = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "not this one\ndata\nʤ\n");
        }));
    };

    "names xlsx"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xlsx"; names = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "not this one\ndata\nʤ\n");
        }));
    };

    "csv no header"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/no_header_row.csv"; no_header = true; no_inference = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("examples/dummy.csv"));
        }));
    };

    "csv no inference"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/dummy.csv"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1,2,3\n");
        }));
    };

    "xls no inference"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/dummy.xls"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1.0,2.0,3.0\n");
        }));
    };

    "xlsx no inference"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/dummy.xlsx"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1,2,3\n");
        }));
    };

    "geojson no inference"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "_"; format = "geojson"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            std::istringstream iss(R"({"a": 1, "b": 2, "type": "FeatureCollection", "features": [{"geometry": {}, "properties": {"a": 1, "b": 2, "c": 3}}]})");
            stdin_subst new_cin(iss);

            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "id,a,b,c,geojson,type,longitude,latitude\n,1,2,3,\"{}\",,,\n");
        }));
    };

    "json no inference"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "_"; format = "json"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            std::istringstream iss(R"([{"a": 1, "b": 2, "c": 3}])");
            stdin_subst new_cin(iss);

            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1,2,3\n");
        }));
    };
#if 0
    "ndjson no inference"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "_"; format = "ndjson"; no_inference = true; }
        } args;
        expect(nothrow([&] {
            std::istringstream iss(R"({"a": 1, "b": 2, "c": 3})");
            stdin_subst new_cin(iss);

            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == "a,b,c\n1,2,3\n");
        }));
    };
#endif
    "convert xls with write sheets"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xls"; sheet = "data"; d_excel = "2"; dt_excel = "6"; write_sheets = "ʤ,1"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(get_source("sheets_0.csv") == get_source("examples/testxls_unicode_converted.csv"));
            expect(get_source("sheets_1.csv") ==
R"(text,date,integer,boolean,float,datetime,empty_column,h
Chicago Reader,24472.0,40.0,True,1.0,24472.17638888889,,
Chicago Sun-Times,16071.0,63.0,True,1.27,16071.62306712963,,Extra data beyond headers will be trimmed
Chicago Tribune,5844.0,164.0,False,41800000.01,5844.0,,
This row has blanks,,,,,,,
Unicode! Σ,,,,,,,
)");
            expect(!std::filesystem::exists("sheets_2.csv"));

            std::remove("sheets_0.csv");
            std::remove("sheets_1.csv");
        }));
    };

    "convert xlsx with write sheets"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xlsx"; no_inference = true; sheet = "data"; d_excel = "2"; dt_excel = "6"; write_sheets = "ʤ,1"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(get_source("sheets_0.csv") == get_source("examples/testxlsx_unicode_converted.csv"));
            expect(get_source("sheets_1.csv") ==
R"(text,date,integer,boolean,float,datetime,empty_column,h
Chicago Reader,24472,40,True,1,24472.17638888889,,
Chicago Sun-Times,16071,63,True,1.27,16071.62306712963,,Extra data beyond headers will be trimmed
Chicago Tribune,5844,164,False,41800000.01,5844,,
This row has blanks,,,,,,,
Unicode! Σ,,,,,,,
)");
            expect(!std::filesystem::exists("sheets_2.csv"));

            std::remove("sheets_0.csv");
            std::remove("sheets_1.csv");
        }));
    };

    "convert xls with write sheets with names"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xls"; sheet = "data"; d_excel = "2"; dt_excel = "6"; write_sheets = "ʤ,1"; use_sheet_names = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(get_source("sheets_ʤ.csv") == get_source("examples/testxls_unicode_converted.csv"));
            expect(get_source("sheets_data.csv") ==
R"(text,date,integer,boolean,float,datetime,empty_column,h
Chicago Reader,24472.0,40.0,True,1.0,24472.17638888889,,
Chicago Sun-Times,16071.0,63.0,True,1.27,16071.62306712963,,Extra data beyond headers will be trimmed
Chicago Tribune,5844.0,164.0,False,41800000.01,5844.0,,
This row has blanks,,,,,,,
Unicode! Σ,,,,,,,
)");
            expect(!std::filesystem::exists("sheets_0.csv"));
            expect(!std::filesystem::exists("sheets_1.csv"));
            expect(!std::filesystem::exists("sheets_2.csv"));

            std::remove("sheets_ʤ.csv");
            std::remove("sheets_data.csv");
        }));
    };

    "convert xlsx with write sheets with names"_test = [&] {
        struct Args : In2csv_args {
            Args() { file = "examples/sheets.xlsx"; no_inference = true; sheet = "data"; d_excel = "2"; dt_excel = "6"; write_sheets = "ʤ,1"; use_sheet_names = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(get_source("sheets_ʤ.csv") == get_source("examples/testxlsx_unicode_converted.csv"));
            expect(get_source("sheets_data.csv") ==
R"(text,date,integer,boolean,float,datetime,empty_column,h
Chicago Reader,24472,40,True,1,24472.17638888889,,
Chicago Sun-Times,16071,63,True,1.27,16071.62306712963,,Extra data beyond headers will be trimmed
Chicago Tribune,5844,164,False,41800000.01,5844,,
This row has blanks,,,,,,,
Unicode! Σ,,,,,,,
)");
            expect(!std::filesystem::exists("sheets_0.csv"));
            expect(!std::filesystem::exists("sheets_1.csv"));
            expect(!std::filesystem::exists("sheets_2.csv"));

            std::remove("sheets_ʤ.csv");
            std::remove("sheets_data.csv");
        }));
    };

}
