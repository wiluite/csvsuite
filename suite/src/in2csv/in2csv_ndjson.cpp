///
/// \file   suite/src/in2csv/in2csv_ndjson.cpp
/// \author wiluite
/// \brief  Implementation of the ndjson-to-csv converter.

#include "../../include/in2csv/in2csv_ndjson.h"
#include <cli.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include "header_printer.h"

using namespace ::csvsuite::cli;

namespace in2csv::detail::ndjson {
    using column_name_type = std::string;
    using column_values_type = std::vector<std::string>;
    std::unordered_map<column_name_type, column_values_type> csv_map;
    std::size_t total_rows;

    void fill_func (auto && elem, auto const & header, unsigned col, auto && types_n_blanks, auto const & args) {
         
        auto align_columns = [&] {
            while(csv_map[header[col]].size() < total_rows)
                csv_map[header[col]].push_back("");
        };

        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        //TODO: Check whether we need to call is_null_value() as well.
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            align_columns();
            csv_map[header[col]].push_back(!args.blanks && is_null ? "" : compose_text(elem));
            return;
        }
        assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);

        using func_type = std::function<std::string(elem_type const &)>;

#if !defined(CSVKIT_UNIT_TEST)
        static
#endif
        std::array<func_type, static_cast<std::size_t>(column_type::sz)> type2func {
                compose_bool_1_arg < elem_type >
                , [&](elem_type const & e) {
                    static std::ostringstream ss;
                    typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                    compose_numeric(ss, another_rep, args);
                    return ss.str();
                }
                , compose_datetime_1_arg < elem_type, in2csv_conversion_datetime >
                , compose_date_1_arg < elem_type >
                , compose_timedelta_1_arg< elem_type >
        };
        align_columns();
        csv_map[header[col]].push_back(type2func[static_cast<unsigned>(types[col]) - 1](elem));
    }

    void impl::convert() {
        using namespace jsoncons;

        class holder {
            json_decoder<ojson> decoder_;
            std::shared_ptr<json_stream_reader> reader_ptr;
        public:
            explicit holder(impl_args const & a) : reader_ptr (
                [&a, this] {
                    if (a.file.empty() or a.file == "_") {
                        static std::stringstream ss;
                        ss.str({});
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                ss << static_cast<char>(r);
                            else
                                break;
                        }
                        if (ss.str().back() != '\n')
                            ss << '\n';
                        return new json_stream_reader(ss, decoder_);
                    } else {
                        static std::ifstream is(a.file.string());
                        return new json_stream_reader(is, decoder_);
                    }
                }(),
                [&](json_stream_reader* descriptor) {
                    delete descriptor;  
                }
            ) {}
            json_decoder<ojson> & decoder() {
                return decoder_;
            }
            json_stream_reader & reader() {
                return *reader_ptr;
            }
        } doc(a);

        std::vector<std::string> result_header;
        auto update_result_header = [&result_header](auto const & new_header) {
            for (auto & e : new_header)
                if (std::find(result_header.cbegin(), result_header.cend(), e) == result_header.cend())
                    result_header.push_back(e);
        };

        total_rows = 0;
        csv_map.clear();
        while (!doc.reader().eof()) {
            doc.reader().read_next();

            ojson _ = doc.decoder().get_result();
            std::ostringstream oss;
            oss << '[' << print(_) << ']';
            ojson next_csv = ojson::parse(oss.str());
            oss.str({});
            csv::csv_stream_encoder encoder(oss);
            next_csv.dump(encoder);

            a.skip_lines = 0;
            a.no_header = false;
            std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;
            if (!a.skip_init_space)
                variants = notrimming_reader_type(recode_source(oss.str(), a));
            else
                variants = skipinitspace_reader_type(recode_source(oss.str(), a));

            std::visit([&](auto & reader) {
                if constexpr(!std::is_same_v<std::decay_t<decltype(reader)>, std::monostate>) {
                    // no skip_lines() needed
                    auto types_and_blanks = std::get<1>(typify(reader, a, typify_option::typify_without_precisions));
                    // no skip_lines() needed again
                    auto header = string_header(obtain_header_and_<skip_header>(reader, a));
                    update_result_header(header);

                    reader.run_rows([&](auto span) {
                        unsigned col = 0;
                        using elem_type = typename std::decay_t<decltype(span.back())>::reader_type::template typed_span<csv_co::unquoted>;
                        for (auto & e : span)
                            fill_func(elem_type{e}, header, col++, types_and_blanks, a);
                            
                        total_rows++;
                        for (auto & e : result_header)
                            if (csv_map[e].size() != total_rows)
                                csv_map[e].emplace_back("");
                    });
                }
            }, variants);
        }

        std::cout << result_header.front();
        std::for_each(std::cbegin(result_header) + 1, std::cend(result_header), [](auto & e) { std::cout << ',' << e; });
        std::cout << '\n';

        for (auto i = 0u; i < total_rows; i++) {
            std::cout << csv_map[result_header.front()][i];
            std::for_each(std::cbegin(result_header) + 1, std::cend(result_header), [&](auto & e) { std::cout << ',' << csv_map[e][i]; });
            std::cout << '\n';
        }
    }
}
