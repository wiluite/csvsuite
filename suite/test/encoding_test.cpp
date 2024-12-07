///
/// \file   test/encoding_test.cpp
/// \author wiluite
/// \brief  Text encodings tests.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"
#include <csv_co/reader.hpp>
#include <encoding.h>

int main() {
    using namespace boost::ut;
    using namespace csv_co;
    using namespace ::csvsuite::cli::encoding;

#if defined (WIN32)
    cfg<override> ={.colors={.none="", .pass="", .fail=""}};
#endif

    "source_size"_test = [] {
        reader<> r {"0,1,2"};
        expect(r.size() == 5);
    };

    "bom_source_size"_test = [] {
        reader<> r {std::string("\xef\xbb\xbf")+"0,1,2"};
        expect(r.size() == 5);
    };

    "bad_bom_source_size"_test = [] {
        reader<> r {std::string("\xef\xaa\xbf")+"0,1,2"};
        expect(r.size() == 8);
    };

    "source_is_utf8"_test = [] {
        reader<> r {std::string(R"(0,1,2)")};
        auto const res = is_source_utf8(r);
        expect(static_cast<int>(res.error) == 0);
    };

    "source_is_not_utf8"_test = [] {
        reader<> r {std::string("\x30\x2c\x31\x2c\x32\xa9")}; // LATIN-1
        expect(r.size() == 6);
        auto const res = is_source_utf8(r);
        expect(static_cast<int>(res.error) != 0);
        expect(res.count == 5);
    };

    "recode_source_from_utf-16le"_test = [] {
        std::string s;
        std::array<char,6> a {0x31, 0, 0x2c, 0, 0x4b, 0x04}; // russian : "1,ы"
        std::copy(a.begin(), a.end(), std::back_inserter(s));
        reader<> r {s};
        expect(r.size() == 6);
        r = reader<>(recode_source_from(r, "utf-16le"));
        expect(r.size() == 4);
        expect(std::string(r.data(), r.data()+r.size()) == "1,ы");
    };

    "recode_source_from_utf-16-be"_test = [] {
        std::string s;
        std::array<char,6> a {0, 0x31, 0, 0x2c, 0x04, 0x4b}; // russian : "1,ы"
        std::copy(a.begin(), a.end(), std::back_inserter(s));
        reader<> r {s};
        expect(r.size() == 6);
        r = reader<>(recode_source_from(r, "utf-16-be"));
        expect(r.size() == 4);
        expect(std::string(r.data(), r.data()+r.size()) == "1,ы");
    };

    "recode_source_from_utf-32-le_BOM"_test = [] {
        std::string s;
        std::array<char,16> a {static_cast<char>(0xff), static_cast<char>(0xfe), 0x00, 0x00, 0x31, 0, 0, 0, 0x2c, 0, 0, 0, 0x4b, 0x04, 0, 0 }; // russian : "1,ы"
        std::copy(a.begin(), a.end(), std::back_inserter(s));
        reader<> r {s};
        expect(r.size() == 16);
        r = reader<>(recode_source_from(r, "utf-32-le"));
        expect(r.size() == 4);
        expect(std::string(r.data(), r.data()+r.size()) == "1,ы");
    };

    "reiconv_recode_source_from_shift-jis"_test = [] {
        reader<> r {std::filesystem::path{"bjarne.csv"}};
        expect(r.size() == 18);
        expect(nothrow([&] { r = reader<>{recode_source_from(r, "CP932")};})); //sjis shift_jis shift-jis
        expect(r.size() == 18);
        expect(std::string(r.data(), r.data()+r.size()) == "Hello,Бьярне");
    };

    "test latin1"_test = [] {
        reader<> r {std::filesystem::path{"test_latin1.csv"}};
        expect(r.size() == 17);
        expect(nothrow([&] { r = reader<>{recode_source_from(r, "latin1")};}));
        expect(r.size() == 18);
        expect(std::string(r.data(), r.data()+r.size()) == "a,b,c\n1,2,3\n4,5,\xc2\xa9");
    };

    "bad_reiconv_recode_source_from_sjis"_test = [] {
        reader<> r {std::string("\x30\x2c\x31\x2c\x32\xa9\xa9")}; // latin-1
        expect(r.size() == 7);
        try {
            recode_source_from(r, "UTF-8");
        } catch (std::exception const & e) {
            expect (!strcmp(e.what(), "iconv_converter: Invalid multibyte sequence at position 5"));
        }
    };

    "active code page name found"_test = [] {
        auto result = active_code_page_name();
#if defined (__unix__)
        expect (!std::get<0>(result));
        expect (std::string(std::get<1>(result)).substr(0,5) == "UTF-8");
#else
        expect (std::get<0>(result));
        expect (std::string(std::get<1>(result)).substr(0,2) == "CP");
#endif
    };

}
