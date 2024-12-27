#include "../../include/in2csv/in2csv_json.h"
#include <cli.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include "header_printer.h"

using namespace ::csvsuite::cli;

namespace in2csv::detail::json {

    void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args, std::ostream & os) {
        if (elem.is_null_value())
            return;

        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            os << (!args.blanks && is_null ? "" : compose_text(elem));
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
                    if (!compose_numeric_corner_cases(ss, another_rep, args))
                        ss << another_rep.str();
                    return ss.str();
                }
                , compose_datetime_1_arg < elem_type, in2csv_conversion_datetime >
                , compose_date_1_arg < elem_type >
                , compose_timedelta_1_arg< elem_type >
        };
        auto const type_index = static_cast<std::size_t>(types[col]) - 1;
        os << type2func[type_index](elem);
    }

    void impl::convert() {
        using namespace jsoncons;

        class ojson_holder {
            std::shared_ptr<ojson> ptr;
        public:
            explicit ojson_holder(impl_args const & a) : ptr (
                [&a] {
                    if (a.file.empty() or a.file == "_") {
                        std::string foo;
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                foo += static_cast<char>(r);
                            else
                                break;
                        }

                        return new ojson(ojson::parse(foo));
                    } else
                        return new ojson;
                }(),
                [&](ojson* descriptor) {
                    delete descriptor;  
                }
            ) {
                if (!(a.file.empty() or a.file == "_")) {
                    std::ifstream is(a.file.string());
                    is >> *ptr;
                }
            }
            void obtain_csv(std::ostream & os) {
                csv::csv_stream_encoder encoder(os);
                ptr->dump(encoder);

            }
            void obtain_csv(std::ostream & os, std::string const & key) {
                csv::csv_stream_encoder encoder(os);
                ojson w = ptr->at(key);
                w.dump(encoder);
            }
        } doc(a);

        std::ostringstream oss;
        if (a.key.empty())
            doc.obtain_csv(oss);
        else
            doc.obtain_csv(oss, a.key);

        a.skip_lines = 0;
        a.no_header = false;
        std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;
        if (!a.skip_init_space)
            variants = notrimming_reader_type(recode_source(oss.str(), a));
        else
            variants = skipinitspace_reader_type(recode_source(oss.str(), a));

        std::visit([&](auto & reader) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(reader)>, std::monostate>) {
                auto types_and_blanks = std::get<1>(typify(reader, a, typify_option::typify_without_precisions));

                auto const header = obtain_header_and_<skip_header>(reader, a);
                std::ostream & os = std::cout;
                print_head(a, header, os);

                std::size_t line_nums = 0;
                reader.run_rows(
                    [&](auto rowspan) {
                        if (a.linenumbers)
                            os << ++line_nums << ',';

                        auto col = 0u;
                        using elem_type = typename std::decay_t<decltype(rowspan.back())>::reader_type::template typed_span<csv_co::unquoted>;
                        std::for_each(rowspan.begin(), rowspan.end() - 1, [&](auto const & e) {
                            print_func(elem_type{e}, col++, types_and_blanks, a, os);
                            os << ',';
                        });
                        print_func(elem_type{rowspan.back()}, col, types_and_blanks, a, os);
                        os << '\n';
                    }
                );

            }
        }, variants);

    }
}
