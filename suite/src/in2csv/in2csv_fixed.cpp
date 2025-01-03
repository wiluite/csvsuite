///
/// \file   suite/src/in2csv/in2csv_fixed.cpp
/// \author wiluite
/// \brief  Implementation of the fixed-to-csv converter.

#include "../../include/in2csv/in2csv_fixed.h"
#include <cli.h>

using namespace ::csvsuite::cli;
using namespace ::csvsuite::cli::encoding;

namespace in2csv::detail::fixed {
    void convert_impl(auto & reader, auto & args) {
        static_assert(std::is_same_v<std::decay_t<decltype(reader)>, notrimming_reader_type> or
                      std::is_same_v<std::decay_t<decltype(reader)>, skipinitspace_reader_type>);

        // assume skip_lines only for the data file.
        auto skip_lns = args.skip_lines;
        args.skip_lines = 0;

        skip_lines(reader, args); // args uses neither "file" nor "schema", so that's fine.
        quick_check(reader, args); // args uses neither "file" nor "schema", so that's fine again.

        schema_decoder s_dec(reader);

        auto bytes_from = [](std::string const & str, unsigned byte_offset, unsigned symbols) {
            unsigned bytes = 0;
            if (!symbols)
                return bytes;
            unsigned len = 0;
            for(auto ptr = str.begin() + byte_offset; ptr != str.end(); ptr++) {
                len += (*ptr & 0xc0) != 0x80;
                ++bytes;
                if (len == symbols) {
                    ++ptr;
                    while(ptr != str.end() and (len += (*ptr & 0xc0) != 0x80, len) == symbols) {
                        ++bytes;
                        ++ptr;
                    }
                    return bytes;
                }
            }
            return bytes;
        };

        auto print_header = [&] {
            if (args.linenumbers)
                std::cout << "line_number,";
            for (auto const & e : s_dec.names())
                std::cout << e << (std::addressof(s_dec.names().back()) != std::addressof(e) ? "," : "");
            std::cout << '\n';
        };
        print_header();

        auto process = [&](auto & args, auto & istrm) {
            std::string ln;

            while (skip_lns--)
                if (!std::getline(istrm, ln, '\n'))
                    throw std::runtime_error("ValueError: Too many lines to skip.");

            // ln.clear() to calm down the clang analyzer about we are using ln after moving it right underneath.
            for (;  ln.clear(), std::getline(istrm, ln, '\n');) {
                if (args.linenumbers) {
                    static std::size_t linenumber = 0; 
                    std::cout << ++linenumber << ',';
                }

                auto _ = recode_source(std::move(ln), args);
                for (auto i = 0u; i < s_dec.names().size(); i++) {
                    auto b = bytes_from(_, 0, s_dec.starts()[i]);
                    auto e = bytes_from(_, b, s_dec.lengths()[i]);
                    auto piece = std::string(_.begin() + b, _.begin() + e + b);
                    piece.erase(0, piece.find_first_not_of(' '));
                    piece.erase(piece.find_last_not_of(' ') + 1);
                    std::cout << compose_text(piece) << (i < s_dec.names().size() - 1 ? "," : "");
                }
                std::cout << '\n';
            }
        };

        if (args.file.empty() or args.file == "_") {
            std::stringstream oss{read_standard_input(args)};
            process(args, oss);
        } else {
            std::ifstream file(args.file);
            if (!(file.is_open()))
                throw std::runtime_error("Error opening the file: '" + args.file.string() + "'.");
            process(args, file);
        }
    }

    void impl::convert() {
        std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;

        if (!a.skip_init_space)
            variants = notrimming_reader_type(std::filesystem::path{a.schema});
        else
            variants = skipinitspace_reader_type(std::filesystem::path{a.schema});

        std::visit([&](auto & arg) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
                recode_source(arg, a);
                convert_impl(arg, a);
            }
        }, variants);
    }
}
