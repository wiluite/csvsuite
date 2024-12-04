#include "../../include/in2csv/in2csv_fixed.h"
#include <cli.h>

using namespace ::csvkit::cli;
using namespace ::csvkit::cli::encoding;

namespace in2csv::detail::fixed {

    void convert_impl(auto & reader, auto & args) {
        static_assert(std::is_same_v<std::decay_t<decltype(reader)>, notrimming_reader_type> or
                      std::is_same_v<std::decay_t<decltype(reader)>, skipinitspace_reader_type>);

        // assume skip_lines only for the data file.
        auto skip_lns = args.skip_lines;
        args.skip_lines = 0;

        skip_lines(reader, args); // args uses neither "file" nor "schema", so that's fine.
        quick_check(reader, args); // args uses neither "file" nor "schema", so that's fine again.

        std::vector<std::string> columns;

        std::unordered_map<std::string, std::vector<std::variant<std::string, unsigned>>> all;
        using elem_type = typename std::decay_t<decltype(reader)>::template typed_span<csv_co::unquoted>;
        static std::locale loc("C");
        elem_type::imbue_num_locale(loc);

        reader.run_rows(
            [&columns](auto rowspan) {
                for (auto e : rowspan)
                    columns.push_back(e.operator csv_co::unquoted_cell_string());
            }
            ,[&](auto rowspan) {
                unsigned col = 0;
                for (auto e : rowspan) {
                   auto et {elem_type{e}};
                   if (et.is_num())
                       all[columns[col]].emplace_back(static_cast<unsigned>(et.num()));
                   else if (et.is_str())
                       all[columns[col]].push_back(et.operator csv_co::unquoted_cell_string());
                   else
                      throw std::runtime_error("A value of unsupported type or a null value is in the schema file.");
                   ++col;
                }
            }
        );
        if (all.find("column") == all.end())
            throw std::runtime_error("ValueError: A column named \"column\" must exist in the schema file.");
        if (all.find("start") == all.end())
            throw std::runtime_error("ValueError: A column named \"start\" must exist in the schema file.");
        if (all.find("length") == all.end())
            throw std::runtime_error("ValueError: A column named \"length\" must exist in the schema file.");

        auto & names = all["column"];
        auto & starts = all["start"];
        auto & lengths = all["length"];
        assert(names.size() == starts.size());
        assert(names.size() == lengths.size());

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
            for (auto const & e : names)
                std::cout << std::get<0>(e) << (std::addressof(names.back()) != std::addressof(e) ? "," : "");
            std::cout << '\n';
        };
        print_header();

        auto process = [&](auto & args, auto & istrm) {
            std::string ln;
            // TODO: process getline result below.
            while (skip_lns--)
                std::getline(istrm, ln, '\n');

            for (; std::getline(istrm, ln, '\n');) {
                if (args.linenumbers) {
                    static std::size_t linenumber = 0; 
                    std::cout << ++linenumber << ',';
                }
                // TODO: fixme. If recode_source() is called not once - be sure to reconsider encodings names again.
                auto _ = recode_source(std::move(ln), args);
                for (auto i = 0u; i < names.size(); i++) {
                    auto b = bytes_from(_, 0, std::get<1>(starts[i]));
                    auto e = bytes_from(_, b, std::get<1>(lengths[i]));
                    auto piece = std::string(_.begin() + b, _.begin() + e + b);
                    piece.erase(piece.find_last_not_of(' ') + 1);
                    std::cout << piece << (i < names.size() - 1 ? "," : "");
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
