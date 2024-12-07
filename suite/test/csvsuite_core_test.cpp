///
/// \file   test/csvsuite_core_test.cpp
/// \author wiluite
/// \brief  Core functionality tests.

//TODO: Add tests for oll of cli.h
#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"
#include <cli.h>
#include "test_reader_macros.h"
#include "common_args.h"

int main() {
    using namespace boost::ut;
    using namespace csv_co;
    using namespace ::csvsuite::cli;
    using namespace ::csvsuite::test_facilities;
    using namespace vince_csv;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    bool locale_support = detect_locale_support();

    if (locale_support) {
        "tune_format"_test = [] {
            std::ostringstream oss;
            oss.imbue(std::locale("de_DE.UTF-8"));

            "sscanf_s does not depend on global locale"_test = [&oss] {
                expect(throws([&]() { tune_format(oss, "dummy"); }));
                std::locale::global(std::locale("en_US.UTF-8"));
                expect(nothrow([&]() { tune_format(oss, "%.12f"); }));
                std::locale::global(std::locale("de_DE.UTF-8"));
                expect(nothrow([&]() { tune_format(oss, "%.2e"); }));
            };

            expect(nothrow([&]() { tune_format(oss, "%.2F"); }));

            expect(nothrow([&]() { tune_format(oss, "%.10f"); }));
            oss << spec_prec(1.012345);
            expect(oss.str() == "1,012345");
            oss.str({});
            oss << spec_prec(1.01234567891);
            expect(oss.str() == "1,0123456789");
            oss.str({});
            oss << spec_prec(1.01234567899);
            expect(oss.str() == "1,012345679");
            oss.str({});
            oss << spec_prec(101);
            expect(oss.str() == "101,");

            oss.str({});
            expect(nothrow([&] { tune_format(oss, "%.7e"); }));
            oss << spec_prec(10.12345);
            expect(oss.str() == "1,0123450e+01");

            oss.str({});
            expect(nothrow([&]() { tune_format(oss, "%.12g"); }));
            oss << spec_prec(1.012345);
            expect(oss.str() == "1,012345");
        };

        "tune_ostream"_test = [] {
            std::ostringstream oss;
            std::locale::global(std::locale("en_GB"));
            expect(nothrow([&] { tune_ostream<custom_boolean_and_groping_sep_facet>(oss, "%.4f"); }));
            oss << std::boolalpha << true << ' ' << false << ' ' << 123456.123456;
            expect("True False 123,456.1235" == oss.str());

            oss.str({});
            expect(nothrow([&] { tune_ostream<custom_boolean_and_no_groping_sep_facet>(oss, "%.4f"); }));
            oss << std::boolalpha << true << ' ' << false << ' ' << 123456.123456;
            expect("True False 123456.1235" == oss.str());
        };
    }

    "quick_check"_test = [] {

        struct Args {
            std::size_t skip_lines = 0;
            bool check_integrity {true};
        } args;

        expect(nothrow([&] { quick_check(reader<> ("a,b,c\n1,2,3"), args); }));
        expect(nothrow([&] { quick_check(reader<> ("a,b,c\n1,2,3\n"), args); }));

        expect(throws([&] { quick_check(reader<> ("a,b,c\n1,3"), args); }));
        try {quick_check(reader<> ("a,b,c\n1,3"), args);} catch(std::exception const & e) {
            expect(std::string(e.what()) == R"(The document has different numbers of columns : 2 3 at least at rows : 2 1...
Either use/reuse the -K option for alignment, or use the csvclean utility to fix it.)"
            or std::string(e.what()) == R"(The document has different numbers of columns : 3 2 at least at rows : 1 2...
Either use/reuse the -K option for alignment, or use the csvclean utility to fix it.)");
        }

        expect(throws([&] { quick_check(reader<> ("a,b,c\n1,2,3,4"), args); }));
        try {quick_check(reader<> ("a,b,c\n1,2,3,4"), args);} catch(std::exception const & e) {
            expect(std::string(e.what()) == R"(The document has different numbers of columns : 4 3 at least at rows : 2 1...
Either use/reuse the -K option for alignment, or use the csvclean utility to fix it.)"
            or std::string(e.what()) == R"(The document has different numbers of columns : 3 4 at least at rows : 1 2...
Either use/reuse the -K option for alignment, or use the csvclean utility to fix it.)");
        }

        expect(throws([&] { quick_check(reader<>("a,b,c\n1,2,3\n\n"), args); }));
        try {quick_check(reader<> ("a,b,c\n1,2,3\n\n"), args);} catch(std::runtime_error const & e) {
            expect(std::string(e.what()) == R"(The document has 1 column at 3 row...)");
        }
        // special case of the common case above also detected
        try {quick_check(reader<> ("a\n1,2\n"), args);} catch(std::runtime_error const & e) {
            expect(std::string(e.what()) == R"(The document has 1 column at 1 row...)");
        }

        // no wrong document if 1-column featured
        expect(nothrow([&] { quick_check(reader<>("a\n1\n\n\n\n\n\n"), args); }));

    };

    "cr_trimming"_test = [] {

        std::vector<cell_string> v;
        std::vector<cell_string> v2{"\rnine"};

        reader<::csvsuite::cli::trim_policy::crtrim> r("\rnine\r\n");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v2 == v);
    };

    "initial space trimming"_test = [] {

        std::vector<cell_string> v;
        std::vector<cell_string> v2{"nine"};

        reader<::csvsuite::cli::trim_policy::init_space_trim> r("   nine\r\n");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v2 == v);
    };

    static std::locale loc("C");

    "typed_span locale"_test = [&] {
        reader<>::typed_span<quoted>::imbue_num_locale(loc);
        expect(reader<>::typed_span<quoted>::num_locale().name() == "C");
        expect(throws([&] {
            reader<>::typed_span<unquoted>::num_locale().name();
        }));
    };

    "check typed_span type"_test = [&] {

        using reader_type = reader<>;
        reader<>::typed_span<unquoted>::imbue_num_locale(loc);
        // reader<>::typed_span<quoted>::imbue(loc); <-- this is already imbued in the test above...
        reader<csv_co::trim_policy::alltrim>::typed_span<unquoted>::imbue_num_locale(loc);
        reader<csv_co::trim_policy::alltrim>::typed_span<quoted>::imbue_num_locale(loc);
        reader<::csvsuite::cli::trim_policy::crtrim>::typed_span<quoted>::imbue_num_locale(loc);
        reader<::csvsuite::cli::trim_policy::crtrim>::typed_span<unquoted>::imbue_num_locale(loc);
        reader<::csvsuite::cli::trim_policy::init_space_trim>::typed_span<quoted>::imbue_num_locale(loc);
        reader<::csvsuite::cli::trim_policy::init_space_trim>::typed_span<unquoted>::imbue_num_locale(loc);

        cell_string cs;

        "check types"_test = [&cs] {
            cs = " \"123456789  \"         ";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_STRING);
                expect(not span.is_num());
                expect(span.is_str());
                expect(not span.is_float());
                expect(not span.is_int());
                expect(not span.is_nil());
                expect(not span.is_boolean());
                expect(throws([&] { span.num(); }));
            }
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_INT32);
                expect(span.is_num());
                expect(not span.is_str());
                expect(not span.is_float());
                expect(span.is_int());
                expect(not span.is_nil());
                expect(not span.is_boolean());
                expect(nothrow([&] { span.num(); }));
                expect(span.num() == 123456789);
                expect(span.precision() == 0);

                cs = " \"12345678e-1  \"         ";
                {
                    reader<>::typed_span<unquoted> float_span{reader<>::cell_span{cs}};
                    expect(float_span.is_float());
                    expect(float_span.num() >= 1234567.8 and float_span.num() <= 1234567.9);
                }
            }
            cs = " \"-123456789  \"         ";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_STRING);
                expect(not span.is_num());
                expect(span.is_str());
                expect(not span.is_float());
                expect(not span.is_int());
                expect(not span.is_nil());
                expect(not span.is_boolean());
                expect(throws([&] { span.num(); }));
            }
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_INT32);
                expect(span.is_num());
                expect(not span.is_str());
                expect(not span.is_float());
                expect(span.is_int());
                expect(not span.is_nil());
                expect(not span.is_boolean());
                expect(nothrow([&] { span.num(); }));
                expect(span.num() == -123456789);
                expect(span.precision() == 0);

                cs = " \"-12345678e-1  \"         ";
                {
                    reader<>::typed_span<unquoted> float_span{reader<>::cell_span{cs}};
                    expect(float_span.is_float());
                    expect(float_span.num() <= -1234567.8 and float_span.num() >= -1234567.9);
                }
            }
            cs = " \"+123456789  \"         ";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_STRING);
                expect(not span.is_num());
                expect(span.is_str());
                expect(not span.is_float());
                expect(not span.is_int());
                expect(not span.is_nil());
                expect(not span.is_boolean());
                expect(throws([&] { span.num(); }));
            }
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_INT32);
                expect(span.is_num());
                expect(not span.is_str());
                expect(not span.is_float());
                expect(span.is_int());
                expect(not span.is_nil());
                expect(not span.is_boolean());
                expect(nothrow([&] { span.num(); }));
                expect(span.num() == +123456789);
                expect(span.precision() == 0);

                cs = " \"+12345678e-1  \"         ";
                {
                    reader<>::typed_span<unquoted> float_span{reader<>::cell_span{cs}};
                    expect(float_span.is_float());
                    expect(float_span.num() >= 1234567.8 and float_span.num() < 1234567.9);
                    expect(float_span.precision() == 1);
                }
                cs = " \"+1e-  1  \"  ";  //strange string rep. Considered as number.
                {
                    reader<>::typed_span<unquoted> float_span{reader<>::cell_span{cs}};
                    expect(float_span.is_num());
                    expect(float_span.is_float());
                    expect(float_span.num() >= 0.1 and float_span.num() < 0.2);
                    expect(float_span.precision() == 1);
                }
            }

        };

        "nils and nulls tests"_test = [&cs] {
            cs = "  \"  \" ";
            expect(not reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_nil());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_nil());

            cs = " . "; // this NULL is a responsibility of vince_csv (fortunately)
            expect(reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_null());
            expect(reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_nil());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_null());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_nil());

            cs = " \" . \" ";
            expect(not reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_null());
            expect(not reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_nil());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_null());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_nil());

            using cr_trim_pol = ::csvsuite::cli::trim_policy::crtrim;
            using init_space_trim_pol = ::csvsuite::cli::trim_policy::init_space_trim;

            cs = " N/a ";
            expect(reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_null());
            expect(reader<cr_trim_pol>::typed_span<quoted>{reader<cr_trim_pol>::cell_span{cs}}.is_null());
            expect(reader<init_space_trim_pol>::typed_span<quoted>{reader<init_space_trim_pol>::cell_span{cs}}.is_null());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_null());
            expect(reader<cr_trim_pol>::typed_span<unquoted>{reader<cr_trim_pol>::cell_span{cs}}.is_null());
            expect(reader<init_space_trim_pol>::typed_span<unquoted>{reader<init_space_trim_pol>::cell_span{cs}}.is_null());

            cs = " \"N/a\" ";
            expect(not reader<>::typed_span<quoted>{reader<>::cell_span{cs}}.is_null());
            expect(not reader<cr_trim_pol>::typed_span<quoted>{reader<cr_trim_pol>::cell_span{cs}}.is_null());
            expect(not reader<init_space_trim_pol>::typed_span<quoted>{reader<init_space_trim_pol>::cell_span{cs}}.is_null());
            expect(reader<>::typed_span<unquoted>{reader<>::cell_span{cs}}.is_null());
            expect(reader<cr_trim_pol>::typed_span<unquoted>{reader<cr_trim_pol>::cell_span{cs}}.is_null());
            expect(reader<init_space_trim_pol>::typed_span<unquoted>{reader<init_space_trim_pol>::cell_span{cs}}.is_null());
        };

        "booleans tests"_test = [&cs] {
            cs = R"( 01 )";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 1 and span.is_num());
            }

            cs = R"( 0 )";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 0 and span.is_num());
            }

            cs = R"( " 01 " )";
            expect(not (reader<>::typed_span<quoted>{reader<>::cell_span{cs}}).is_boolean());
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 1 and span.is_num());
            }

            cs = R"( " 0 " )";
            expect(not (reader<>::typed_span<quoted>{reader<>::cell_span{cs}}).is_boolean());
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 0 and span.is_num());
            }

            cs = R"(  TrUe  )";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 1 and span.is_num());
            }

            cs = R"( "TrUe " )";
            expect(not (reader<>::typed_span<quoted>{reader<>::cell_span{cs}}).is_boolean());
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 1 and span.is_num() and span.num() == 1);
            }

            cs = R"( " F" )";
            using policy = csv_co::trim_policy::alltrim;
            expect(reader<policy>::typed_span<unquoted>{reader<policy>::cell_span{cs}}.is_boolean());
            expect(not reader<policy>::typed_span<quoted>{reader<policy>::cell_span{cs}}.is_boolean());
        };

        "no leading zeroes"_test = [&cs] {
            cs = R"( 01 )";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(span.is_boolean() and span.unsafe_bool() == 1 and span.is_num());
            }
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                reader<>::typed_span<quoted>::no_leading_zeroes(true);
                expect(!span.is_boolean() /*and span.unsafe_bool() == 1*/ and !span.is_num());
                reader<>::typed_span<quoted>::no_leading_zeroes(false);
            }
            cs = R"( 02.3 )";
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                expect(!span.is_boolean() and span.is_num());
            }
            {
                reader<>::typed_span<quoted> span{reader<>::cell_span{cs}};
                reader<>::typed_span<quoted>::no_leading_zeroes(true);
                expect(!span.is_boolean() and !span.is_num() and span.is_str());
                reader<>::typed_span<quoted>::no_leading_zeroes(false);
            }
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                reader<>::typed_span<unquoted>::no_leading_zeroes(true);
                expect(!span.is_boolean() and !span.is_num() and span.is_str());
                reader<>::typed_span<unquoted>::no_leading_zeroes(false);
            }
        };

        "utf-8 ignore case comparison"_test = [&cs] {
            cs = R"( ПрИвЕт, C++ )";
            std::string cs2 = R"( привет, c++ )";
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                reader<>::typed_span<unquoted> span2{reader<>::cell_span{cs2}};
                reader<>::typed_span<unquoted>::case_insensitivity(false);
                expect (span.compare(span2) < 0);
            }
            {
                reader<>::typed_span<unquoted> span{reader<>::cell_span{cs}};
                reader<>::typed_span<unquoted> span2{reader<>::cell_span{cs2}};
                reader<>::typed_span<unquoted>::case_insensitivity(true);
                expect(span.compare(span2) == 0);
                reader<>::typed_span<unquoted>::case_insensitivity(false);
            }
        };


    };

    "generate column names"_test = [] {
        { 
            csv_co::reader r("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27\n");
            auto column_cells = generate_column_names(r);
            expect(column_cells[0] == "a");
            expect(column_cells[2] == "c");
            expect(column_cells[25] == "z");
            expect(column_cells[26] == "aa");
        }
        { 
            csv_co::reader r("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26\n");
            expect(nothrow([&]() {
                auto column_cells = generate_column_names(r);
                expect(column_cells[0] == "a");
                expect(column_cells[2] == "c");
                expect(column_cells[25] == "z");
            }));
        }
        { 
            csv_co::reader r("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28\n");
            expect(throws([&]() {
                generate_column_names(r);
            }));
        }

    };

    "obtain_header_and_skip"_test = [] {
        struct args {
            bool no_header = false;
        } a;

        csv_co::reader r("h1,h2,h3,h4\n");
        r.skip_rows(1); // no skip if eof
        expect(throws([&]() {
            obtain_header_and_<::csvsuite::cli::skip_header>(r, a);
        }));
        r.skip_rows(0); // no skip if 0.
        expect(nothrow([&]() {
            auto h = obtain_header_and_<::csvsuite::cli::no_skip_header>(r, a);
            expect(h[0] == "h1");
            expect(h[3] == "h4");
        }));
        a.no_header = true;
        expect(nothrow([&]() {
            auto h = obtain_header_and_<::csvsuite::cli::skip_header>(r, a);
            expect(h[0] == "a");
            expect(h[3] == "d");
        }));
    };

    "max_field_size_checker"_test = [] {
        struct args {
            unsigned maxfieldsize = 10;
        } a;

        csv_co::reader<csv_co::trim_policy::alltrim> reader("h1\n\" aФ2345678Ю\"\n");
        max_field_size_checker mfsc1(reader, a, 1, init_row{1});

        reader.run_rows([&](auto &span) {
            // Note, typed_span, constructed from this cell_span should already have numeric_locale imbued.
            for (auto &e: span)
                expect(nothrow([&]() {mfsc1.check(e);}));
        });

        a.maxfieldsize = 9;

        expect(throws([&]() {
            reader.run_rows([&](auto &span) {
                for (auto &e: span)
                    mfsc1.check(e);
            });
        }));

        a.maxfieldsize = 10;
        max_field_size_checker mfsc2(reader, a, 1, init_row{1});
        expect(nothrow([&]() { mfsc2.check(std::string{"aФ2345678Ю"}); }));
        a.maxfieldsize = 9;
        max_field_size_checker mfsc3(reader, a, 1, init_row{1});
        expect(throws<std::exception>([&]() { mfsc3.check(std::string{"aФ2345678Ю"}); }));
        try {
            mfsc3.check(std::string{"\"aФ2345678Ю\""});
        } catch (std::exception const &e) {
            expect(std::string(e.what()) == "FieldSizeLimitError: CSV contains a field longer than the maximum length of 9 characters on line 1.");
        }
    };

    "acp transformer"_test = [] {
        std::string str{"\xC2\xA9\xC2\xA9\xC2\xA9\x31"}; //©©©1
        expect(str.size() == 7);
        encoding::acp_transformer t;
        t.transform(str);
        if (std::strcmp(t.acp(), "UTF-8") != 0) {
            expect(str.size() != 7);
        }
    };

    if (locale_support) {
        "localized typed_span type"_test = [] {

            std::locale de_loc("de_DE.utf-8");
            using reader_type = reader<>;
            reader_type::typed_span<unquoted>::imbue_num_locale(de_loc);

            cell_string cs = " \"-1.234.567,89e-1  \"   ";
            {
                reader_type::typed_span <unquoted> typed_field{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(typed_field.type()) == DataType::CSV_DOUBLE);

                expect(std::abs(typed_field.num() - -123456.789) <= 0.000000001);
                expect(typed_field.num() != -123456.789);
                expect(typed_field.precision() == 3);
            }

            cs = " \"-1.234.567,89d  \"";
            {
                reader_type::typed_span <unquoted> typed_field{reader_type::cell_span{cs.begin(), cs.end()}};
                // It depends
                expect(static_cast<DataType>(typed_field.type()) == DataType::CSV_DOUBLE || static_cast<DataType>(typed_field.type()) == DataType::CSV_STRING);
            }
            cs = " \"-1.234.567,89 EUR  \"";
            {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_DOUBLE);
                expect(std::abs(span.num() - -1234567.89) <= 0.000000001);
                expect(std::round(span.num()) == -1234568);
                expect(span.precision() == 2);
            }
            // TODO: fix it
#if 0
            cs = " \"-1.234.567,89 $  \"";
            {
                reader_type::typed_span<unquoted> span {reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_STRING);
            }
#endif
            cs = " \"-1.234.567,8 EUR  \"";
            {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_STRING);
            }
            cs = " \"-1.234.5678 EUR  \"";
            {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_STRING);
            }

            cs = " \"-1.234.555   \"";
            {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_INT32);
                expect(span.num() == -1234555);
                expect(span.precision() == 0);
            }
            cs = " \"-1.234.555,01 €  \"";
            {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_DOUBLE);
                expect(std::abs(span.num() - -1234555.01) < 0.000000001);
                expect(span.precision() == 2);
            }
            cs = " \"-1.234.555,00 €  \"";
            {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_DOUBLE);
                expect(span.num() == -1234555.00);
                expect(span.precision() == 0);
            }
            cs = " NAN"; {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_DOUBLE);
                expect(std::isnan(span.num()));
            }
            cs = " -infinity"; {
                reader_type::typed_span <unquoted> span{reader_type::cell_span{cs.begin(), cs.end()}};
                expect(static_cast<DataType>(span.type()) == DataType::CSV_DOUBLE);
                expect(std::isinf(span.num()));
            }
        };
    }

    using detect_date_and_datetime_types_reader_type = test_reader_r1;

    "detect date and datetime types"_test = [&] {
        struct args : common_args, type_aware_args {} a;
        {
            a.maxfieldsize = max_unsigned_limit;
            a.datetime_fmt = R"(%d/%m/%EY %I:%M:%S)";
            a.date_fmt = R"(%d/%m/%Y)"; // can not use %Y against 2-digit year in all cases!
            a.date_lib_parser = false;

            notrimming_reader_type r("a\n01/02/24\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            //what type - it depends... On many linuses date_t 
            expect (types[0] == column_type::text_t or types[0] == column_type::date_t);
            if (types[0] == column_type::date_t) {
                r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                    notrimming_reader_type::typed_span<csv_co::unquoted> elem {e};
                    // please, smile ;o)
                    expect(date_s(elem) == "0024-02-01" or date_s(elem) == "1924-02-01");
                }});
            }
        }
        {   // same date format and 4-digit year
            a.date_fmt = R"(%d/%m/%Y)";
            notrimming_reader_type r("a\n 01/02/2024\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::date_t);
        }
        {   // same date format and 1-digit day and month numbers
            notrimming_reader_type r("a\n1/2/1970\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
#if defined(__MINGW32__) || defined (__linux__) //MINGW can't handle with 1-digit form of d/m/y or h:m:s
            // UBUNTU 22 Docker runs well (date_t). The others can differ.
            expect (types[0] == column_type::date_t || types[0] == column_type::text_t);
#else
    #if defined (_MSC_VER)
            expect (types[0] == column_type::date_t);
    #endif
#endif
        }
        {   //WORKS WITH ISO 8601 BY DEFAULT for dates
            notrimming_reader_type r("a\n 2039-04-14 \n14/04/2039 \n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::date_t);
            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                notrimming_reader_type::typed_span<csv_co::unquoted> elem {e};
                auto const date_string = date_s(elem);
                expect(date_string.find("2039-04-14") != std::string::npos);                
            }});
        }

        {   // year is y instead of Y
            a.date_fmt = R"(%d/%m/%y)";
            detect_date_and_datetime_types_reader_type r("a\n01/02/24\n"); // FIXME: to change format we must change reader type
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
#if defined(__MINGW32__)  //MINGW can't handle with %y-format against year less then 38 and 20/21 centuries
            expect (types[0] == column_type::text_t);
#else
            expect (types[0] == column_type::date_t);

            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                detect_date_and_datetime_types_reader_type::typed_span<csv_co::unquoted> elem {e};
                // please smile again : 1924-02-01 may be at linux
                expect(date_s(elem) == "2024-02-01" or date_s(elem) == "1924-02-01");
            }});
#endif
        }
        {   // DateTime format %d/%m/%Y %I:%M:%S %p
            notrimming_reader_type r("a\n 01/02/2024 01:03:24\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::datetime_t); // TODO: it is wrong: %I must require %p
        }
        {
            // WORKS WITH ISO 8601 BY DEFAULT for DateTimes
            notrimming_reader_type r("a\n 2039-Apr-14 08:30:31\n14/04/2039 08:30:31\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::datetime_t);
            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                notrimming_reader_type::typed_span<csv_co::unquoted> elem {e};
                expect(datetime_s(elem).find("08:30:31") != std::string::npos);
            }});
        }
    };

    using another_reader_type = test_reader_r2;
    using yet_one_reader_type = test_reader_r3;

    "detect date and datetime types with Howard Hinnant's date library parser"_test = [&] {
        struct args : common_args, type_aware_args {} a;

        {
            expect(a.date_lib_parser);
            a.datetime_fmt = R"(%d/%m/%EY %H:%M:%S)";
            a.date_fmt = R"(%d/%m/%Y)"; // We really can use %Y against 2-digit year.
            another_reader_type r("a\n01/02/24\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::date_t);            
            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                another_reader_type::typed_span<csv_co::unquoted> elem {e};
                expect(date_s(elem) == "0024-02-01");
            }});
        }
        {   // same date format and 4-digit year
            another_reader_type r("a\n 01/02/2024\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::date_t);
        }
        {   // same date format and 1-digit day and month numbers
            another_reader_type r("a\n1/2/1969\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect(types[0] == column_type::date_t);
        }
        {
            // WORKS WITH ISO 8601 BY DEFAULT for dates
            another_reader_type r("a\n 2039-04-14 \n14/04/2039 \n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::date_t);
            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                another_reader_type::typed_span<csv_co::unquoted> elem {e};
                auto const date_string = date_s(elem);
                expect(date_string.find("2039-04-14") != std::string::npos);                
            }});
        }
        {   // year is y instead of Y
            a.date_fmt = R"(%d/%m/%y)";
            a.datetime_fmt = R"(%d/%m/%Y %I:%M:%S %p)";
            yet_one_reader_type r("a\n01/02/24\n"); // FIXME: to change format we must change reader type
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::date_t);

            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                yet_one_reader_type::typed_span<csv_co::unquoted> elem {e};
                auto const date_string = date_s(elem);
                expect(date_string == "2024-02-01");  
            }});
        }
        {   // DateTime format %d/%m/%Y %I:%M:%S %p
            yet_one_reader_type r("a\n01/02/2024 01:03:24 am\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::datetime_t);
        }
        {
            // WORKS WITH ISO 8601 BY DEFAULT for DateTimes
            yet_one_reader_type r("a\n 2039-Apr-14 20:30:31\n14/04/2039 08:30:31 pm\n");
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 1);
            expect (types[0] == column_type::datetime_t);
            r.run_rows ([](auto){}, [&](auto & span) { for (auto & e : span) {
                yet_one_reader_type::typed_span<csv_co::unquoted> elem {e};
                expect(datetime_s(elem).find("20:30:31") != std::string::npos);
            }});
        }

    };

    using typify_reader_type = test_reader_r4;

    "typify"_test = [&] {

        struct args : common_args, type_aware_args {} a;

        a.datetime_fmt = R"(%d/%m/%EY %I:%M:%S)";
        a.date_fmt = R"(%d/%m/%EY)";
        a.date_lib_parser = false;
        // 10  ,  NULL,  28/02/2023 08:08:08, 14/04/2023,              ,1sec
        // NULL,  F   ,  29/03/2023 08:08:08, 15/04/2023,  another text,1sec
        // -inf,  F   ,  30/03/2003 07:08:09, 16/04/2023,  yet one     ,1sec
        //notrimming_reader_type r("h1,h2,h3,h4,h5\n\"10\",,28/02/2023 08:08:08, 14/04/2023,\nn/a, F, 29/03/2023 08:08:08,15/04/2023,another text ");

        typify_reader_type r("h1,h2,h3,h4,h5,h6\n\"10.1\",,28/02/2023 08:08:08, 14/04/2023,,1s\nn/a, F, 29/03/2023 08:08:08,15/04/2023,another text,1s\n-inf,F,,16/04/2023,yet one,1s");
        {
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect(types.size() == 6 && blanks.size() == 6);
            expect (blanks[0] && blanks[1] && blanks[2] && !blanks[3] && blanks[4] && !blanks[5]);

            expect (types[0] == column_type::number_t);
            expect (types[1] == column_type::bool_t);
            expect (types[2] == column_type::datetime_t);
            expect (types[3] == column_type::date_t);
            expect (types[4] == column_type::text_t);
            expect (types[5] == column_type::timedelta_t);
        }

        // (now a.blanks == false) It is an influence to column types (all are text)

        a.no_inference = true; 
        {
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            expect (blanks[0]);
            expect (blanks[1]);
            expect (blanks[2]);
            expect (!blanks[3]);
            expect (blanks[4]);
            expect (!blanks[5]);

            for (auto && e : types)
                expect(e == column_type::text_t);
        }

        // It is an influence to considering all columns to be NOT NULL (not blanks, even though they contain nulls)
        
        a.blanks = true; 
        {
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            for (auto && e : blanks)
                expect(!e);
            for (auto && e : types)
                expect(e == column_type::text_t);
        }

        // blanks(true) now influences to column type detection by not converting empties to NULL
        // and those particular columns will be of text type

        a.no_inference = false;
        {
            auto [types, blanks] = std::get<1>(typify(r, a, typify_option::typify_without_precisions));
            for (auto && e : blanks)
                expect(!e);

            expect (types[0] == column_type::text_t);
            expect (types[1] == column_type::text_t);
            expect (types[2] == column_type::text_t);
            expect (types[3] == column_type::date_t);
            expect (types[4] == column_type::text_t);
            expect (types[5] == column_type::timedelta_t);
        }

    };

    "typify with precisions"_test = [&] {
        struct args : common_args, type_aware_args {} a;

        a.datetime_fmt = R"(%d/%m/%EY %I:%M:%S)";
        a.date_fmt = R"(%d/%m/%EY)";
        a.date_lib_parser = false;

        notrimming_reader_type r("h1\n10.1\n10.1234\n10.12\n");
        {
            auto [types, blanks, precisions] = std::get<0>(typify(r, a, typify_option::typify_with_precisions));
            expect(precisions.size() == 1);
            expect(types[0] == column_type::number_t);
            expect(precisions[0] == 4);
        }
    };

}
