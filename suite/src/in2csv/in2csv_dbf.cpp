#include "../../include/in2csv/in2csv_dbf.h"
#include <cli.h>

#include "../../external/dbf_lib/include/bool.h"
#include "../../external/dbf_lib/include/dbf.h"

#include <functional>
#include "header_printer.h"

using namespace ::csvsuite::cli;
using namespace ::csvsuite::cli::encoding;

//TODO: do type-aware printing
//TODO: RAII
namespace in2csv::detail::dbf {

    void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args, std::ostream & os) {
        if (elem.is_null_value()) {
            os << "";
            return;
        }

        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            auto compose_text = [&](auto const & e) -> std::string {
                typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                if (another_rep.raw_string_view().find(',') != std::string_view::npos)
                    return another_rep;
                else
                    return another_rep.str();
            };
            os << (!args.blanks && is_null ? "" : compose_text(elem));
            return;
        }
        assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);

        using func_type = std::function<std::string(elem_type const &)>;

#if !defined(CSVSUITE_UNIT_TEST)
        static
#endif
        std::array<func_type, static_cast<std::size_t>(column_type::sz)> type2func {
                compose_bool_1_arg < elem_type >
                , [&](elem_type const & e) {
                    assert(!e.is_null());

                    static std::ostringstream ss;
                    ss.str({});

                    typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                    auto const value = another_rep.num();

                    if (std::isnan(value))
                        ss << "NaN";
                    else if (std::isinf(value))
                        ss << (value > 0 ? "Infinity" : "-Infinity");
                    else {
                        if (args.num_locale != "C") {
                            std::string s = another_rep.str();
                            another_rep.to_C_locale(s);
                            ss << s;
                        } else
                            ss << another_rep.str();
                    }
                    return ss.str();
                }
                , compose_datetime_1_arg < elem_type, in2csv_conversion_datetime >
                , compose_date_1_arg < elem_type >
                , [](elem_type const & e) {
                    auto str = std::get<1>(e.timedelta_tuple());
                    return str.find(',') != std::string::npos ? R"(")" + str + '"' : str;
                }
        };
        auto const type_index = static_cast<std::size_t>(types[col]) - 1;
        os << type2func[type_index](elem);
    }

    void impl::convert() {
        DBF_HANDLE handle = dbf_open(a.file.string().c_str(), nullptr);
        BOOL ok = (handle != nullptr);
        if (!ok)
            throw std::runtime_error(std::string("Unable to open file: ") + "'" + a.file.string() + "'");

        size_t rows = dbf_getrecordcount(handle);
        size_t cols = dbf_getfieldcount(handle);
        DBF_FIELD_INFO info;
     
        std::string header;
        using field_type = int;
        std::vector<field_type> types;

        if (a.linenumbers)
            std::cout << "line_number,";

        for (auto j = 0u; j < cols; j++) {
            dbf_getfield_info (handle, j, &info);
            header += (j ? "," : "") + recode_source(std::string(info.name), a);
            types.push_back(static_cast<field_type>(info.type));
        }

        std::string contents = header + '\n';
        std::size_t linenumber = 0;
        for (auto i = 0u; i < rows; i++) {
            if (a.linenumbers)
                std::cout << ++linenumber << ',';

            dbf_setposition(handle, i);
            for (auto j = 0u; j < cols; j++) {
                char temp[1024] = "";
                dbf_getfield(handle, dbf_getfieldptr(handle, j), temp, sizeof(temp), DBF_DATA_TYPE_ANY);
                contents += (j ? "," : "") + recode_source(std::string(temp), a);
            }
            contents += '\n';
        }
        dbf_close(&handle);

        a.skip_lines = 0;
        a.no_header = false;
        std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;
        if (!a.skip_init_space)
            variants = notrimming_reader_type(recode_source(std::move(contents), a));
        else
            variants = skipinitspace_reader_type(recode_source(std::move(contents), a));

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
