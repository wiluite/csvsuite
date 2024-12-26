#pragma once

namespace {
    auto is_number = [](std::string const & name) {
        return std::all_of(name.begin(), name.end(), [](auto c) {return std::isdigit(c);});
    };

    std::vector<std::string> & get_header() {
        static std::vector<std::string> header;
        return header;
    }

    unsigned header_cell_index = 0;
    std::vector<unsigned> can_be_number;

    void OutputHeaderNumber(std::ostringstream & oss, const double number, unsigned) {
        // now we have native header, and so "1" does not influence on the nature of this column
        can_be_number.push_back(1);

        std::ostringstream header_cell;
        csvsuite::cli::tune_format(header_cell, "%.16g");

        header_cell << number;
        get_header().push_back(header_cell.str());
        oss << get_header().back();
        ++header_cell_index;
    }

    inline bool is_date_column(unsigned column) {
        return can_be_number[column] and std::find(dates_ids.begin(), dates_ids.end(), column) != std::end(dates_ids);
    }

    inline bool is_datetime_column(unsigned column) {
        return can_be_number[column] and std::find(datetimes_ids.begin(), datetimes_ids.end(), column) != std::end(datetimes_ids);
    }

    template <bool DecimalForInts>
    inline void OutputNumber(std::ostringstream & oss, const double number, unsigned column) {
        // now we have first line of the body, and so "1" really influence on the nature of this column
        if (can_be_number.size() < get_header().size())
            can_be_number.push_back(1);

        if (is_date_column(column)) {
            using date::operator<<;
            std::ostringstream local_oss;
            local_oss << to_chrono_time_point(number);
            auto str = local_oss.str();
            oss << std::string{str.begin(), str.begin() + 10};
        } else
        if (is_datetime_column(column)) {
            using date::operator<<;
            std::ostringstream local_oss;
            oss << to_chrono_time_point(number);
        } else {
            oss << number;
            if constexpr(DecimalForInts) {
                if (std::round(number) == number)
                    oss << ".0";
            }
        }
    }

    std::vector<std::string> generate_header(unsigned length) {
        std::vector<std::string> letter_names (length);
        unsigned i = 0;
        std::generate(letter_names.begin(), letter_names.end(), [&i] {
            return csvsuite::cli::letter_name(i++);
        });

        return letter_names;
    }

    void generate_and_print_header(std::ostream & oss, auto const & args, unsigned column_count, use_date_datetime_excel use_d_dt) {
        if (args.no_header) {
            get_header() = generate_header(column_count);
            for (auto & e : get_header())
                oss << (std::addressof(e) == std::addressof(get_header().front()) ? e : "," + e);
            oss << '\n';
            get_date_and_datetime_columns(args, get_header(), use_d_dt);
        }
    }

    void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args, std::ostream & os) {
        using namespace csvsuite::cli;
        if (elem.is_null_value()) {
            os << "";
            return;
        }
        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            os << (!args.blanks && is_null ? "" : compose_text(elem));
            return;
        }
        assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);

        using func_type = std::function<std::string(elem_type const &)>;

#if !defined(CSVSUITE_UNIT_TEST)
        static
#endif
        std::array<func_type, static_cast<std::size_t>(column_type::sz)> type2func {
                compose_bool_1_arg<elem_type>
                , [&](elem_type const & e) {
                    static std::ostringstream ss;
                    typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                    if (!ostream_numeric_corner_cases(ss, another_rep, args))
                        ss << another_rep.str();
                    return ss.str();
                }
                , compose_datetime_1_arg<elem_type, in2csv_conversion_datetime>
                , compose_date_1_arg<elem_type>
                , compose_timedelta_1_arg<elem_type>
        };
        auto const type_index = static_cast<std::size_t>(types[col]) - 1;
        os << type2func[type_index](elem);
    }

    void transform_csv(auto & args, std::ostringstream & from, std::ostream & to) {
        using namespace csvsuite::cli;
        args.skip_lines = 0;
        args.no_header = false;
        std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;

        if (!args.skip_init_space)
            variants = notrimming_reader_type(from.str());
        else
            variants = skipinitspace_reader_type(from.str());

        std::visit([&](auto & arg) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
                auto types_and_blanks = std::get<1>(typify(arg, args, typify_option::typify_without_precisions));
                std::size_t line_nums = 0;
                arg.run_rows(
                        [&](auto) {
                            if (args.linenumbers)
                                to << "line_number,";

                            std::for_each(get_header().begin(), get_header().end() - 1, [&](auto const & elem) {
                                to << elem << ',';
                            });

                            to << get_header().back() << '\n';
                        }
                        ,[&](auto rowspan) {
                            if (args.linenumbers)
                                to << ++line_nums << ',';

                            auto col = 0u;
                            using elem_type = typename std::decay_t<decltype(rowspan.back())>::reader_type::template typed_span<csv_co::unquoted>;
                            std::for_each(rowspan.begin(), rowspan.end()-1, [&](auto const & e) {
                                print_func(elem_type{e}, col++, types_and_blanks, args, to);
                                to << ',';
                            });
                            print_func(elem_type{rowspan.back()}, col, types_and_blanks, args, to);
                            to << '\n';
                        }
                );
            }
        }, variants);
    }

    void print_sheets(auto const & a, auto sheet_name_fun, auto sheet_index_fun, auto fill_sheets_fun, auto print_sheet_fun) {
        if (!a.write_sheets.empty()) {
            std::vector<std::string> sheet_names = [&] {
                std::vector<std::string> result;
                if (a.write_sheets != "-") {
                    std::istringstream stream(a.write_sheets);
                    for (std::string word; std::getline(stream, word, ',');) {
                        if (is_number(word))
                            result.push_back(sheet_name_fun(word));
                        else {
                            sheet_index_fun(word);
                            result.push_back(word);
                        }
                    }
                } else
                    result = fill_sheets_fun();

                return result;
            }();

            std::vector<std::string> sheet_filenames (sheet_names.size());
            int cursor = 0;
            for (auto const & e : sheet_names) {
                auto const filename = "sheets_" + (a.use_sheet_names ? e : std::to_string(cursor)) + ".csv";
                std::ofstream ofs(filename);
                try {
                    print_sheet_fun(sheet_index_fun(e), ofs, a, use_date_datetime_excel::no);
                } catch(std::exception const & ex) {
                    std::cerr << ex.what() << std::endl;
                }
                cursor++;
            }

        }
    }
}

