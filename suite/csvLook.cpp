///
/// \file   utils/csvsuite/csvLook.cpp
/// \author wiluite
/// \brief  Render a CSV file in the console.

#include <cli.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <tuple>
#include <array>

using namespace ::csvsuite::cli;

namespace csvlook {
    struct Args final : ARGS_positional_1 {
        std::string & num_locale = kwarg("L,locale","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool & blanks = flag("blanks",R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> & null_value = kwarg("null-value", "Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        unsigned long & max_rows = kwarg("max-rows","The maximum number of rows to display before truncating the data.").set_default(max_size_t_limit);
        unsigned & max_columns = kwarg("max-columns","The maximum number of columns to display before truncating the data.").set_default(max_unsigned_limit);
        unsigned & max_column_width = kwarg("max-column-width","Truncate all columns to at most this width. The remainder will be replaced with ellipsis.").set_default(max_unsigned_limit);
        unsigned & max_precision = kwarg("max-precision","The maximum number of decimal places to display. The remainder will be replaced with ellipsis.").set_default(3u);
        bool & no_number_ellipsis = flag("no-number-ellipsis","Disable the ellipsis if --max-precision is exceeded.");
        bool & no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        std::string & glob_locale = kwarg("G,glob-locale", "Superseded global locale.").set_default("C");
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nRender a CSV file in the console as a Markdown-compatible, fixed-width table.\n\n";
        }

        // column precision to hack
        // in order to escape a Clang sanitizer false-positive accessing i in num func

        mutable unsigned precision_locally{};
    };

    void look(std::monostate &, auto const &) {}

    constexpr const char * native_ellipsis = "...";
    constexpr unsigned native_ellipsis_len = std::char_traits<char>::length(native_ellipsis);

    namespace detail {
        void check_str_symbols_func_compatibility(std::string_view);
        void ostream_fixed_precision (auto & os, auto const & elem, unsigned, unsigned, bool);
        void make_required_adjustments(auto const & args);
    }

    void look(auto & reader, auto const & args) {
        using namespace detail;
        check_str_symbols_func_compatibility(args.glob_locale);
        setup_global_locale(args.glob_locale);

        static_assert(native_ellipsis_len == 3);
        constexpr const char * mini_ellipsis = "..";
        constexpr unsigned mini_ellipsis_len = std::char_traits<char>::length(mini_ellipsis);
        static_assert(mini_ellipsis_len == 2);
        constexpr const char * micro_ellipsis = ".";
        constexpr unsigned micro_ellipsis_len = std::char_traits<char>::length(micro_ellipsis);
        static_assert(micro_ellipsis_len == 1);

        using namespace csv_co;

        skip_lines(reader, args);
        quick_check(reader, args);

        make_required_adjustments(args);

#if !defined(__clang__) || __clang_major__ >= 16
        auto [types, blanks, precisions] = std::get<0>(typify(reader, args, typify_option::typify_with_precisions));
#else
        auto types_blanks_precisions_tuple = std::get<0>(typify(reader, args, typify_option::typify_with_precisions));
        auto & types = std::get<0>(types_blanks_precisions_tuple);
        auto & blanks = std::get<1>(types_blanks_precisions_tuple);
        auto & precisions = std::get<2>(types_blanks_precisions_tuple);
#endif

        skip_lines(reader, args);
        auto const header = obtain_header_and_<skip_header>(reader, args);

        std::vector<unsigned> max_sizes (header.size());
        std::transform (header.begin(), header.end(), max_sizes.begin(),[&](auto & elem) {
            return std::min(args.max_column_width, static_cast<unsigned>(csv_co::csvsuite::str_symbols(elem.operator csv_co::unquoted_cell_string())));
        });

        using reader_type = std::decay_t<decltype(reader)>;
        using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;

        try {
            auto detect_max_sizes = [&] {
                std::size_t row = 0;
                auto const ir = init_row{args.no_header ? 1u : 2u};

                reader.run_rows([&](auto & row_span) {
                    unsigned i = 0;
                    for (auto & elem : row_span) {
                        args.precision_locally = precisions[i];
                        auto max_size_func = [&](auto && elem) {
                            if (elem.is_null_value())
                                return std::max(std::min(max_sizes[i], args.max_column_width), micro_ellipsis_len);
                            auto const is_null = elem.is_null();
                            if (types[i] == column_type::text_t or (!args.blanks && is_null)) {
                                return !args.blanks && is_null ? std::max(std::min(max_sizes[i], args.max_column_width), micro_ellipsis_len) :
                                    std::max(std::min(std::max(max_sizes[i], elem.unsafe_str_size_in_symbols()), args.max_column_width), micro_ellipsis_len);
                            }
                            assert(!is_null && (!args.blanks || (args.blanks && !blanks[i])) && !args.no_inference);
                            using args_type = decltype(args);
                            using type_aware_function = std::function<unsigned(unsigned, elem_type const &, args_type args)>;
                            static std::array<type_aware_function, static_cast<std::size_t>(column_type::sz) - 1> type2func {
                                [&](unsigned current_size, auto const & e, args_type args) {
                                    constexpr unsigned false_len = 5u;
                                    constexpr unsigned true_len = 4u;

                                    auto const value = (e.is_boolean(), static_cast<bool>(e.unsafe()));
                                    auto const unrestricted_size = (value ? std::max(current_size, true_len) : std::max(current_size, false_len));
                                    return std::max(std::min(unrestricted_size, args.max_column_width), micro_ellipsis_len);
                                }
                                , [&](unsigned current_size, auto const & e, args_type args) {
#ifndef _MSC_VER
                                    static
#endif
                                    num_stringstream ss(args.glob_locale);
                                    ostream_fixed_precision(ss, e, args.max_precision, args.precision_locally, args.no_number_ellipsis);
                                    unsigned const size = csv_co::csvsuite::str_symbols(ss.str());
#ifndef _MSC_VER
                                    ss.str({});
#endif
                                    return std::max(std::min(std::max(current_size, size), args.max_column_width), micro_ellipsis_len);
                                }
                                , [&](unsigned current_size, auto, args_type args) {
                                    unsigned const sz = std::size("yyyy-mm-dd hh:mm:ss") - 1;
                                    return std::max(std::min(std::max(current_size, sz), args.max_column_width), micro_ellipsis_len);
                                }
                                , [&](unsigned current_size, auto, args_type args) {
                                    unsigned const sz = std::size("yyyy-mm-dd") - 1;
                                    return std::max(std::min(std::max(current_size, sz), args.max_column_width), micro_ellipsis_len);
                                }
                                , [&](unsigned current_size, auto const & e, args_type args) {
                                    unsigned const sz = std::get<1>(e.timedelta_tuple()).length();
                                    return std::max(std::min(std::max(current_size, sz), args.max_column_width), micro_ellipsis_len);
                                }
                            };
                            return type2func[static_cast<std::size_t>(types[i]) - 1](max_sizes[i], elem, args);
                        };
                        max_sizes[i] = max_size_func(elem_type{elem});
                        i++;
                    }
                    if (++row == args.max_rows)
                        throw typename reader_type::implementation_exception();
                });
            };

            if (args.max_rows && args.max_columns)
                detect_max_sizes();
        } catch ( typename reader_type::implementation_exception const &) {
            // may be awaitable reader::implementation_exception
        } catch (typename reader_type::exception const &) {
            throw;
        }

        std::ostringstream oss;
        std::ostream & oss_ = args.asap ? std::cout : oss;
        auto print_func_impl = [&] (auto && elem_str, unsigned max_size, std::size_t row) {
            auto const refactor_elem = [&]() -> std::tuple<unsigned, unsigned, std::string> {
                auto const str = csv_co::csvsuite::to_basic_string_32(elem_str);
                if (str.size() <= max_size)
                    return std::tuple{str.size(), elem_str.size(), elem_str};
                else {
                    auto newstr = str.substr(0, max_size);
                    if (row != args.max_rows) {
                        auto const offset = max_size - native_ellipsis_len;
                        std::transform(newstr.cbegin() + offset, newstr.cbegin() + max_size, newstr.begin() + offset, [](auto &) { return '.'; });
                    }
                    assert(max_size==newstr.size());
                    return std::tuple{max_size, elem_str.size(), csv_co::csvsuite::from_basic_string_32(newstr)};
                }
            };
            auto const [p,s,e] = refactor_elem();
            auto const w = [&](unsigned positions, unsigned elem_size) { return positions < max_size ? max_size + elem_size - positions : max_size; } (p, s);
            oss_.width(w);
            oss_ << (row != args.max_rows ? e : ((w < 2) ? micro_ellipsis : ((w < 3) ? mini_ellipsis : native_ellipsis))) << " | ";
        };

        static constexpr char const * const linenumbers_header = "line_numbers";
        constexpr unsigned linenumbers_header_length = std::char_traits<char>::length(linenumbers_header);
        static_assert (linenumbers_header_length == 12);

        auto print_hdr = [&] {
            oss_ << "| ";
            if (args.linenumbers)
                print_func_impl(std::string(linenumbers_header), std::min(linenumbers_header_length, args.max_column_width), max_size_t_limit - 1);

            auto col = 0u;
            for (auto &elem: header) {
                oss_.setf(types[col] == column_type::text_t || args.no_inference || (args.blanks && blanks[col]) ? std::ios::left : std::ios::right, std::ios::adjustfield);
                print_func_impl(elem.operator csv_co::unquoted_cell_string(), max_sizes[col], max_size_t_limit - 1);
                if (++col == args.max_columns && col != header.size()) {
                    oss_ << "... |";
                    break;
                }
            }
            oss_ << "\n| ";

            if (args.linenumbers) {
                for (auto i = 0ul; i < std::min(linenumbers_header_length, args.max_column_width); i++)
                    oss_ << '-';
                oss_ << " | ";
            }

            col = 0u;
            for (auto const &elem: max_sizes) {
                oss_.width(elem);
                oss_.fill('-');
                oss_ << '-';
                oss_.fill(' ');
                oss_ << " | ";
                if (++col == args.max_columns && col != max_sizes.size()) {
                    oss_ << "--- |";
                    break;
                }
            }
            oss_ << '\n';
        };

        if (args.max_columns)
            print_hdr();
        else {
            // print header corner cases.
            oss_ << "| ";
            if (args.linenumbers)
                print_func_impl(std::string(linenumbers_header), std::min(linenumbers_header_length, args.max_column_width), max_size_t_limit - 1);
            oss_ << "... |\n| ";
            if (args.linenumbers) {
                for (auto i = 0ul; i < std::min(linenumbers_header_length, args.max_column_width); i++)
                    oss_ << '-';
                oss_ << " | ";
            }
            oss_ << "--- |\n";
        }

        tune_ostream<custom_boolean_and_groping_sep_facet>(oss_);

        auto line_numbers_print = [&] (auto index) {
            if (args.linenumbers) {
                oss_.setf(args.no_inference? std::ios::left : std::ios::right, std::ios::adjustfield);
                print_func_impl(std::to_string(index + 1), std::min(linenumbers_header_length, args.max_column_width), index);
            }
        };

        try {
            auto print_body = [&] {
                auto print_func = [&](auto && elem, std::size_t col, std::size_t row) {
                    if (elem.is_null_value()) {
                        oss_.setf(std::ios::left, std::ios::adjustfield);
                        print_func_impl(std::string{}, max_sizes[col], row);
                        return;
                    }
                    bool const is_null = elem.is_null();
                    if (types[col] == column_type::text_t or (!args.blanks && is_null)) {
                        oss_.setf(std::ios::left, std::ios::adjustfield);
                        print_func_impl(!args.blanks && is_null ? "" : compose_text(elem), max_sizes[col], row);
                        return;
                    }

                    assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);
                    using func_type = std::function<std::string(elem_type const &, std::any)>;
                    static std::array<func_type, static_cast<std::size_t>(column_type::sz) - 1> type2func {
                            compose_bool<elem_type>
                            , [&](elem_type const & elem, std::any info) {
                                assert(!elem.is_null());
                                static num_stringstream ss(args.glob_locale);
                                ss.str({});
                                auto const [max_prec, col_precision, no_num_ellipsis] = std::any_cast<std::tuple<unsigned, unsigned, bool>>(info);
                                ostream_fixed_precision(ss, elem, max_prec, col_precision, no_num_ellipsis);
                                return ss.str();
                            }
                            , compose_datetime<elem_type>
                            , compose_date<elem_type>
                            , [&](elem_type const & elem, std::any const &) {
                                return std::get<1>(elem.timedelta_tuple());
                            }
                    };

                    oss_.setf(std::ios::right, std::ios::adjustfield);
                    auto const type_index = static_cast<std::size_t>(types[col]) - 1;
                    using num_tuple_t = std::tuple<unsigned, unsigned, bool>;
                    std::any info = (type_index == 1) ? num_tuple_t(args.max_precision, precisions[col], args.no_number_ellipsis) : std::any{};
                    print_func_impl(type2func[type_index](elem, info), max_sizes[col], row);
                };
                std::size_t row = 0;
                reader.run_rows([&] (auto & row_span) {
                    oss_ << "| ";
                    line_numbers_print(row);

                    auto col = 0u;
                    for (auto & elem : row_span) {
                        print_func(elem_type{elem}, col, row);
                        if (++col == args.max_columns && col != row_span.size()) {
                            oss_ << "... |";
                            break;
                        }
                    }
                    oss_ << "\n";
                    if (row++ == args.max_rows)
                        throw typename std::decay_t<decltype(reader)>::implementation_exception();
                });
            };

            if (args.max_columns && args.max_rows)
                print_body();
            else {
                // print body corner cases.
                auto line_numbers_dotted_print = [&] {
                    if (args.linenumbers) {
                        oss_.setf(args.no_inference ? std::ios::left : std::ios::right, std::ios::adjustfield);
                        oss_.width(std::min(linenumbers_header_length, args.max_column_width));
                        oss_ << "...";
                        oss_ << " | ";
                    }
                };

                if (!args.max_columns) {
                    unsigned long const rows = reader.rows();
                    for (auto i = 0ul; i < std::min(rows, args.max_rows); i++) {
                        oss_ << "| ";
                        line_numbers_print(i);
                        oss_ << "... |\n";
                    }
                    if (rows > args.max_rows) {
                        oss_ << "| ";
                        line_numbers_dotted_print();
                        oss_ << "... |\n";
                    }
                } else {
                    oss_ << "| ";
                    line_numbers_dotted_print();

                    auto col = 0u;
                    for (auto const &sz: max_sizes) {
                        oss_.setf(types[col] == column_type::text_t || args.no_inference || (args.blanks && blanks[col]) ? std::ios::left : std::ios::right, std::ios::adjustfield);
                        oss_.width(sz);
                        oss_ << (sz >= native_ellipsis_len ? native_ellipsis : (sz == 1 ? micro_ellipsis : mini_ellipsis));
                        oss_ << " | ";
                        if (++col == args.max_columns && col != max_sizes.size()) {
                            oss_ << "... |";
                            break;
                        }
                    }
                    oss_ << '\n';
                }
            }

        } catch (...) {} // may be awaitable reader::implementation_exception

        if (!args.asap)
            std::cout << oss.str();
    }

    std::string special_read_standard_input(auto const & args) {
        std::string csv;
        if (args.max_rows == max_size_t_limit)
            args.max_rows /= 2;
        std::size_t const rows_to_read = std::size_t(args.max_rows) + (args.no_header ? 0 : 1) + args.skip_lines + 1;
        std::size_t next_row = 0;

        std::string result;
        for (std::string input;;) {
            std::getline(std::cin, input);
            result += input;
            if (std::cin.eof() || ++next_row >= rows_to_read)
                break;
            result += '\n';
        }
        return result;
#if 0
        while (!std::cin.eof() && ++next_row < rows_to_read) {
            static std::string temp;
            std::getline(std::cin, temp);
            csv += temp;
            csv += '\n';
        }
        return {csv.begin(), csv.end() - 1};
#endif
    };

} /// namespace

namespace csvlook::detail {
    void check_str_symbols_func_compatibility(std::string_view loc_name) {
        std::ostringstream oss;
        try {
            oss.imbue(std::locale(std::string(loc_name)));
            oss.setf(std::ios::fixed, std::ios::floatfield);
            oss.precision(4);
            oss << 1000000.1000;
            try {
                csv_co::csvsuite::str_symbols_deep_check(oss.str());
            } catch (std::exception &) {
                throw std::runtime_error("The current global locale contender is incompatible with the string length detection feature. Use -G=C for safe case.");
            }
        } catch (...) {
            std::cerr << "Locale: " << std::quoted(loc_name) << '\n';
            throw std::runtime_error(::csvsuite::cli::bad_locale_message);
        }
    }

    void ostream_fixed_precision (auto & os, auto const & elem, unsigned max_precision, unsigned col_precision, bool no_ellipsis) {
        auto const value = elem.num();
        os.precision(std::min(max_precision, col_precision));
        if (std::isnan(value))
            os << "NaN";
        else if (std::isinf(value)) {
            os << (value> 0 ? "Infinity" : "-Infinity");
            os << "Infinity";
        } else
            os << value << ((!no_ellipsis and max_precision < col_precision) ? "\xe2\x80\xa6" : "");
    }

    void make_required_adjustments(auto const & args) {
        args.linenumbers = !(!args.max_columns and args.linenumbers) && args.linenumbers;

        if (args.linenumbers) {
            auto const tmp = args.max_columns;
            args.max_columns--;
            if (tmp < args.max_columns)
                args.max_columns = 0;
        }
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {

    using namespace csvlook;

    auto args = argparse::parse<Args>(argc, argv);

    if (args.verbose)
        args.print();

    if (args.max_column_width < native_ellipsis_len) {
        std::cerr << "It is impossible to have --max-column-width less than " << native_ellipsis_len << ".\n";
        return 0;
    }

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(special_read_standard_input, look)

    return 0;
}

#endif
