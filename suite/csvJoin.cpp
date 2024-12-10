///
/// \file   utils/csvsuite/csvjoin.cpp
/// \author wiluite
/// \brief  Execute a SQL-like join to merge CSV files.

#include <cli.h>
#include <cli-compare.h>
#include "type_name.h"
#include <printer_concepts.h>

using namespace ::csvsuite::cli;

namespace csvjoin {
    struct Args final : ARGS_positional_files {
        std::string &num_locale = kwarg("L,locale", "Specify the locale (\"C\") of any formatted numbers.").set_default("C");
        bool &blanks = flag("blanks", R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> &null_value = kwarg("null-value","Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string &date_fmt = kwarg("date-format", "Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string &datetime_fmt = kwarg("datetime-format", "Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        std::string &columns = kwarg("c,columns", "The column name(s) on which to join. Should be either one name (or index) or a comma-separated list with one name (or index) for each file, in the same order that the files were specified. May also be left unspecified, in which case the two files will be joined sequentially without performing any matching.").set_default("");
        bool &outer_join = flag("outer", "Perform a full outer join, rather than the default inner join.");
        bool &left_join = flag("left", "Perform a left outer join, rather than the default inner join. If more than two files are provided this will be executed as a sequence of left outer joins, starting at the left.");
        bool &right_join = flag("right", "Perform a right outer join, rather than the default inner join. If more than two files are provided this will be executed as a sequence of right outer joins, starting at the right.");
        bool &no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        std::string std_input;

        void welcome() final {
            std::cout << "\nExecute a SQL-like join to merge CSV files on a specified column or columns.\n\n";
        }
    };
}
namespace csvjoin::detail::typify {
    enum class csvjoin_source_option {
        csvjoin_file_source,
        csvjoin_string_source
    };

    template <typename Reader, typename Args>
    auto typify(Reader & reader, Args const & args, typify_option option) -> typify_result {

        // Detect types and blanks presence in columns, also imbue cell locale
        update_null_values(args.null_value);

        struct hibernator {
            explicit hibernator(Reader & reader) : reader_(reader) { reader_.skip_rows(0); }
            ~hibernator() { reader_.skip_rows(0); }
        private:
            Reader & reader_;
        };

        hibernator h(reader);
        skip_lines(reader, args);
        auto header = obtain_header_and_<skip_header>(reader, args);

        if (!reader.cols()) // alternatively : if (!reader.rows())
            throw std::runtime_error("Typify(). Columns == 0. Vain to do next actions!"); // well, vain to do rest things

        //check_max_size(reader, args, header, init_row{1});

        fixed_array_2d_replacement<typename Reader::template typed_span<csv_co::unquoted>> table(header.size(), reader.rows());

        auto c_row{0u};
        auto c_col{0u};

        //auto const ir = init_row{args.no_header ? 1u : 2u};
        reader.run_rows([&] (auto & rowspan) {
            static struct tabular_checker {
                using cell_span_t= typename Reader::cell_span;
                tabular_checker (std::vector<cell_span_t> const & header, typename Reader::row_span const & row_span) {
                    if (header.size() != row_span.size())
                        throw typename Reader::exception("The number of header and data columns do not match. Use -K option to align.");
                }
            } checker (header, rowspan);
            //check_max_size(reader, args, rowspan, ir);

            for (auto & elem : rowspan)
                table[c_col++][c_row] = elem;

            c_row++;
            c_col = 0;
        });

        std::vector<column_type> task_vec (table.rows(), column_type::unknown_t);

        std::vector<std::size_t> column_numbers (task_vec.size());
        std::iota(column_numbers.begin(), column_numbers.end(), 0);

        transwarp::parallel exec(std::thread::hardware_concurrency());

        std::vector<unsigned char> blanks (task_vec.size(), 0);
        std::vector<unsigned> precisions (task_vec.size(), 0);

        imbue_numeric_locale(reader, args);
        [&option] {
            using unquoted_elem_type = typename Reader::template typed_span<csv_co::unquoted>;
            unquoted_elem_type::no_maxprecision(option == typify_option::typify_without_precisions);

            using quoted_elem_type = typename Reader::template typed_span<csv_co::quoted>;
            unquoted_elem_type::no_maxprecision(option == typify_option::typify_without_precisions);
        }();

        setup_date_parser_backend(reader, args);
        setup_leading_zeroes_processing(reader, args);

        //TODO: for now e.is_null() calling first is obligate. Can we do better?

        #define SETUP_BLANKS auto const n = e.is_null() && !args.blanks; if (!blanks[c] && n) blanks[c] = 1;

        auto task = transwarp::for_each(exec, column_numbers.cbegin(), column_numbers.cend(), [&](auto c) {
            if (std::all_of(table[c].begin(), table[c].end(), [&blanks, &c, &args](auto & e)  {
                SETUP_BLANKS
                return n || (!args.no_inference && e.is_boolean());
            })) {
                task_vec[c] = column_type::bool_t;
                return;
            }
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &blanks, &c](auto &e) {
                SETUP_BLANKS
                return n || (!args.no_inference && std::get<0>(e.timedelta_tuple()));
            })) {
                task_vec[c] = column_type::timedelta_t;
                return;
            }
            if (option == typify_option::typify_with_precisions) {
                if (std::all_of(table[c].begin(), table[c].end(), [&blanks, &c, &args, &precisions](auto & e) {
                    SETUP_BLANKS
                    auto const result = n || (!args.no_inference && e.is_num());
                    if (result and !n) {
                        if (precisions[c] < e.precision())
                            precisions[c] = e.precision();
                    }
                    return result;
                })) {
                    task_vec[c] = column_type::number_t;
                    return;
                }
            } else {
                if (std::all_of(table[c].begin(), table[c].end(), [&blanks, &c, &args](auto & e) {
                    SETUP_BLANKS
                    return n || (!args.no_inference && e.is_num());
                })) {
                    task_vec[c] = column_type::number_t;
                    return;
                }
            }
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &blanks, &c](auto & e) {
                SETUP_BLANKS
                return n || (!args.no_inference && std::get<0>(e.datetime(args.datetime_fmt)));
            })) {
                task_vec[c] = column_type::datetime_t;
                return;
            }
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &blanks, &c](auto &e) {
                SETUP_BLANKS
                return n || (!args.no_inference && std::get<0>(e.date(args.date_fmt)));
            })) {
                task_vec[c] = column_type::date_t;
                return;
            }
            // Text type: check ALL rows for an absent.
            if (std::all_of(table[c].begin(), table[c].end(), [&](auto &e) {
                if (e.is_null() && !blanks[c] && !args.blanks)
                    blanks[c] = true;
                return true;
            })) {
                task_vec[c] = column_type::text_t;
                return;
            }
        });

        #undef SETUP_BLANKS

        task->wait();

        for (auto & elem : task_vec) {
            assert(elem != column_type::unknown_t);
            if (args.no_inference and elem != column_type::text_t) {
                assert(elem == column_type::bool_t);  // all nulls in a column otherwise boolean
                elem = column_type::text_t;           // force setting it to text
            }
        }
        if (option == typify_option::typify_with_precisions)
            return std::tuple{task_vec, blanks, precisions};
        else
            return std::tuple{task_vec, blanks};
    }
}

namespace csvjoin::detail {

    template<typename Reader, typename ElemType=std::string>
    struct reader_fake : ::csvsuite::cli::fixed_array_2d_replacement<ElemType> {
        using parent_class = ::csvsuite::cli::fixed_array_2d_replacement<ElemType>;
        using parent_class::table_impl;
        using reader_type = Reader;
        struct cell_span {
            explicit operator csv_co::cell_string() const {
                assert(false && "it should never be called");
                return {};
            }
            explicit cell_span(std::string const &){
                assert(false && "it should never be called");
            }
            explicit operator csv_co::unquoted_cell_string() const {
                assert(false && "it should never be called");
                return {};
            }
        };

        reader_fake(auto rows, auto cols) : parent_class(rows, cols) {}

        using value_row_span_cb_t = std::function<void(std::span<ElemType> const &span)>;

        void run_rows(value_row_span_cb_t v) {
            for (auto &&elem: table_impl)
                v(std::span<ElemType>(elem));
        }

        void skip_rows(auto /*rows*/) {}

        auto header() -> std::vector<cell_span> {
            return {};
        }

        struct exception : std::runtime_error {
            template<typename ... Ts>
            explicit constexpr exception(Ts ...) : std::runtime_error("") {}
        };

        void add(auto &&vect) {
            table_impl.emplace_back(std::forward<decltype(vect)>(vect));
        }
    };

    using notrimming_reader_or_fake_type = std::variant<notrimming_reader_type, reader_fake<notrimming_reader_type>>;
    using skipinitspace_reader_or_fake_type = std::variant<skipinitspace_reader_type, reader_fake<skipinitspace_reader_type>>;

    static_assert(!std::is_copy_constructible_v<reader_fake<notrimming_reader_type>>);
    static_assert(!std::is_copy_assignable_v<reader_fake<notrimming_reader_type>>);

    static_assert(
            std::is_move_constructible<reader_fake<notrimming_reader_type, typename notrimming_reader_type::template
            typed_span<csv_co::quoted>>>::value);
    static_assert(
            std::is_move_assignable<reader_fake<notrimming_reader_type, typename notrimming_reader_type::template
            typed_span<csv_co::quoted>>>::value);

    auto parse_join_column_names(auto &&join_string) {
        std::istringstream stream(join_string);
        std::vector<std::string> result;
        for (std::string word; std::getline(stream, word, ',');)
            result.push_back(word);
        return result;
    }

    template <class OS>
    class printer {
        OS &os;

    public:
        explicit printer(OS &os = std::cout) : os(os) {}
        template <typename T>
        void write(T&& printable, auto && types_n_blanks, auto && args)
        requires (!CsvKitCellSpanTableConcept<T> && !CsvKitCellSpanRowConcept<T> && !CellSpanRowConcept<T> && !CsvReaderConcept<T>) {
            if constexpr (std::is_same_v<std::vector<std::string>, std::decay_t<T>>) { // print header
                if (args.linenumbers)
                    os << "line_number,";

                std::for_each(printable.begin(), printable.end()-1, [&](auto const & elem) {
                    os << elem << ',';
                });
                os << printable.back();
                print_LF(os);
            } else if constexpr (std::is_same_v<std::vector<std::string>, std::decay_t<decltype(std::declval<T>()[0])>>) { // print body
                for (auto && elem : printable) {
                    if (args.linenumbers) {
                        static unsigned line_nums = 0;
                        os << ++line_nums << ',';
                    }

                    auto col = 0u;
                    std::for_each(elem.begin(), elem.end()-1, [&](auto const & e) {
                        if constexpr (std::is_same_v<std::decay_t<decltype(types_n_blanks)>,ts_n_blanks_type>) {
                            using elem_type = typename std::decay_t<T>::reader_type::template typed_span<csv_co::unquoted>;
                            static_assert(std::is_same_v<std::decay_t<decltype(e)>, std::string>);
                            print_func(elem_type{e}, col++, types_n_blanks, args);
                            os << ',';
                        }  else
                            os << e << ',';
                    });
                    if constexpr (std::is_same_v<std::decay_t<decltype(types_n_blanks)>,ts_n_blanks_type>) {
                        using elem_type = typename std::decay_t<T>::reader_type::template typed_span<csv_co::unquoted>;
                        static_assert(std::is_same_v<std::decay_t<decltype(elem.back())>, std::string>);
                        print_func(elem_type{elem.back()}, col, types_n_blanks, args);
                    } else
                        os << elem.back();
                    print_LF(os);
                }
            } else {
                std::cerr << "No implementation for printing of this kind of table\n";
                std::cerr << type_name<T>() << std::endl;
            }
        }

        // Pure single file in input
        using ts_n_blanks_type = std::tuple<std::vector<::csvsuite::cli::column_type>, std::vector<unsigned char>>;
        template<CsvReaderConcept R>
        void write(R && r, auto && types_n_blanks, auto && args)  {
            r.run_rows([&](auto & span) {
                if (args.linenumbers) {
                    static std::size_t line_nums = 0;
                    os << ++line_nums << ',';
                }
                auto col = 0u;
                using elem_type = typename std::decay_t<decltype(span.back())>::reader_type::template typed_span<csv_co::unquoted>;
                std::for_each(span.begin(), span.end()-1, [&](auto const & e) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(types_n_blanks)>,ts_n_blanks_type>) {
                        print_func(elem_type{e}, col++, types_n_blanks, args);
                        os << ',';
                    } else
                        os << e.operator csv_co::cell_string() << ',';
                });
                if constexpr (std::is_same_v<std::decay_t<decltype(types_n_blanks)>,ts_n_blanks_type>)
                    print_func(elem_type{span.back()}, col, types_n_blanks, args);
                else
                    os << (span.back()).operator csv_co::cell_string();
                print_LF(os);
            });
        }

    private:
        void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args) {
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

            // TODO: FIXME. Clang sanitizers complains for this in unittests.
            //  Although it seems neither false positives nor bloat code here.
#if !defined(BOOST_UT_DISABLE_MODULE)
            static
#endif
            std::array<func_type, static_cast<std::size_t>(column_type::sz)> type2func {
                    compose_bool_1_arg<elem_type>
                    , [&](elem_type const & e) {
                        assert(!e.is_null());

                        static std::ostringstream ss;
                        ss.str({});

                        // Surprisingly, csvsuite represents a number from file without distortion:
                        // knowing, that it is a valid number in any locale, it simply removes
                        // the thousands separators and replaces the decimal point with its
                        // C-locale equivalent. Thus, the number actually written to the file
                        // is output. and we have to do some tricks.
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
                    , compose_datetime_1_arg<elem_type>
                    , compose_date_1_arg<elem_type>
                    , [](elem_type const & e) {
                        auto str = std::get<1>(e.timedelta_tuple());
                        return str.find(',') != std::string::npos ? R"(")" + str + '"' : str;
                    }
            };
            auto const type_index = static_cast<std::size_t>(types[col]) - 1;
            os << type2func[type_index](elem);
        }
    };

    void join(auto && deq, auto const & args, auto & join_column_names) {
        using namespace csv_co;
        using reader_type = std::variant_alternative_t<0, typename std::decay_t<decltype(deq)>::value_type>;
        std::deque<unsigned> c_ids;
        std::deque<std::vector<std::string>> headers;
        std::deque<std::tuple<std::vector<column_type>, std::vector<unsigned char>>> ts_n_blanks;

        if (args.right_join)
            std::reverse(join_column_names.begin(), join_column_names.end());

        try {
            auto idx = 0u;
            std::for_each(deq.begin(), deq.end(), [&](auto &r) {

                auto quick_check = [&r, &args]() {
                    skip_lines(std::get<0>(r), args);
                    ::csvsuite::cli::quick_check(std::get<0>(r), args);
                };

                //TODO: FIXME --- THIS SHOULD REALLY WORK (and works in GCC10.2, MSVC, CLANG 16)
                // (COMPILER BUGS IN: GCC 11, CLANG 12,13,14,15)
#if 0
                std::visit([&](auto &&arg) {
                    if constexpr (!std::is_same_v<std::decay_t<decltype(arg)>, reader_fake<reader_type>>) {
                        quick_check();
                        ts_n_blanks.push_back(std::get<1>(typify::typify(arg, args, typify_option::typify_without_precisions)));
                    }
#else
                if (!std::holds_alternative<reader_fake<reader_type>>(r)) {
                    quick_check();
                    ts_n_blanks.push_back(std::get<1>(typify::typify(std::get<0>(r), args, typify_option::typify_without_precisions)));
                }
                std::visit([&](auto &&arg) {
#endif
                    skip_lines(arg, args);
                    auto const header = obtain_header_and_<skip_header>(arg, args);

                    max_field_size_checker size_checker(*std::get_if<0>(&r), args, header.size(), init_row{1});
                    // TODO: it is very hard to optimize this place (get rid of premature transformation - try it out)
                    //  but it is vain to do, due to no many resources spending.
                    check_max_size(header_to_strings<unquoted>(header), size_checker);

                    auto const q_header = header_to_strings<quoted>(header);

                    headers.push_back(q_header);
                    if (!join_column_names.empty())
                        c_ids.push_back(match_column_identifier(q_header, join_column_names[idx++].c_str(), get_column_offset(args)));
                }, r);
            });
        } catch (std::exception const &e) {
            std::cout << e.what() << std::endl;
            return;
        }

        auto print_results = [&](auto join_flag) {
            std::ostringstream oss;
            std::ostream & oss_ = args.asap ? std::cout : oss;
            printer p(oss_);
            struct non_typed_output {};
            p.write(headers[0], non_typed_output{}, args);
            join_flag ? p.write(std::get<1>(deq.front()), ts_n_blanks[0], args) : p.write(std::get<0>(deq.front()), ts_n_blanks[0], args);
            if (!args.asap)
                std::cout << oss.str();
        };

        auto concat_headers = [&headers](unsigned excl_v_idx = static_cast<unsigned>(-1)) {
            std::string subst;
            unsigned const h0_size = headers[0].size();
            std::replace_copy_if(headers[1].begin(), headers[1].end(), std::back_inserter(headers[0]),
                [&](auto const & n) {
                    if (!std::count(headers[0].cbegin(), headers[0].cend(), n))
                        return false;

                    auto mangled_part = 2u;
                    for (;;) {
                        auto const what_subst = n + '_' + std::to_string(mangled_part);
                        if (!std::count(headers[0].cbegin(), headers[0].cend(), what_subst)) {
#if 0
                            std::cerr << "Column name " << std::quoted(n) << " already exists in Table. "
                                      << "Column will be renamed to "
                                      << std::quoted(n + '_' + std::to_string(mangled_part)) << ".\n";
#endif
                            subst = what_subst;
                            return true;
                        }
                        mangled_part++;
                    }
                }, subst
            );
            if (excl_v_idx != static_cast<unsigned>(-1))
                headers[0].erase(headers[0].begin() + h0_size + excl_v_idx);
        };

        auto concat_ts_n_blanks = [&ts_n_blanks](unsigned excl_idx = static_cast<unsigned>(-1)) {
            unsigned const size_0 = std::get<0>(ts_n_blanks[0]).size();
            std::get<0>(ts_n_blanks[0]).insert(std::get<0>(ts_n_blanks[0]).end()
                , std::get<0>(ts_n_blanks[1]).begin(), std::get<0>(ts_n_blanks[1]).end());
            std::get<1>(ts_n_blanks[0]).insert(std::get<1>(ts_n_blanks[0]).end()
                , std::get<1>(ts_n_blanks[1]).begin(), std::get<1>(ts_n_blanks[1]).end());

            if (excl_idx != static_cast<unsigned>(-1)) {
                std::get<0>(ts_n_blanks[0]).erase(std::get<0>(ts_n_blanks[0]).begin() + size_0 + excl_idx);
                std::get<1>(ts_n_blanks[0]).erase(std::get<1>(ts_n_blanks[0]).begin() + size_0 + excl_idx);
            }
        };

        enum class exclude_c_column {
            yes,
            no
        }; 
        enum class union_join {
            yes,
            no
        }; 

        auto cycle_cleanup = [&](exclude_c_column is_c_excluded = exclude_c_column::yes, union_join is_union_join = union_join::no) {
            deq.pop_front();
            deq.pop_front();
            concat_headers(is_c_excluded == exclude_c_column::yes ? c_ids[1] : static_cast<unsigned>(-1));
            headers.erase(headers.begin() + 1);
            concat_ts_n_blanks(is_c_excluded == exclude_c_column::yes ? c_ids[1] : static_cast<unsigned>(-1));

            if (is_union_join == union_join::no)
                c_ids.erase(c_ids.begin() + 1);
            ts_n_blanks.erase(ts_n_blanks.begin() + 1);
        };

        auto union_join = [&cycle_cleanup, &deq, &args]() {
            while (deq.size() > 1) {
                std::size_t rows = 0u;
                std::size_t cols = 0u;
                std::for_each(deq.begin(), deq.begin() + 2, [&](auto &r) {
                    std::visit([&](auto &&arg) {
                        rows = std::max(rows, arg.rows());
                        cols += arg.cols();
                    }, r);
                });

                reader_fake<reader_type> impl{rows, cols};

                auto col_ofs = 0;
                std::for_each(deq.begin(), deq.begin() + 2, [&](auto &r) {
                    std::visit([&](auto &&arg) {
                        std::size_t row = 0;
                        auto const total_cols = arg.cols();
                        max_field_size_checker size_checker(*std::get_if<0>(&r), args, total_cols, init_row{args.no_header ? 1u : 2u});
                        arg.run_rows([&](auto &row_span) {
                            if constexpr (!std::is_same_v<std::remove_reference_t<decltype(row_span[0])>, std::string>)
                                check_max_size(row_span, size_checker);

                            unsigned col = 0;
                            for (auto &elem: row_span) {
                                if constexpr (std::is_same_v<std::remove_reference_t<decltype(elem)>, std::string>)
                                    impl[row][col_ofs + col++] = std::move(elem);
                                else
                                    impl[row][col_ofs + col++] = std::move(elem.operator cell_string());
                            }
                            ++row;
                        });
                        col_ofs += total_cols;
                    }, r);
                });

                cycle_cleanup(exclude_c_column::no, union_join::yes);
                deq.push_front(std::move(impl));
            }
        };

        auto pure_c_join = [&] {
            assert(!c_ids.empty());
            while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
                auto const & [types0, blanks0] = ts_n_blanks[0];
                auto const & [types1, blanks1] = ts_n_blanks[1];
#else
                auto const & types0 = std::get<0>(ts_n_blanks[0]);
                auto const & blanks0 = std::get<1>(ts_n_blanks[0]);
                auto const & types1 = std::get<0>(ts_n_blanks[1]);
                auto const & blanks1 = std::get<1>(ts_n_blanks[1]);
#endif
                reader_fake<reader_type> impl{0, 0};

                auto can_compare = [&]() {
                    return (types0[c_ids[0]] == types1[c_ids[1]]) or args.no_inference;
                };

                if (can_compare()) {
                    using namespace ::csvsuite::cli::compare::detail;
                    using elem_t = typename std::decay_t<decltype(std::get<0>(deq.front()))>::template typed_span<quoted>;
#if !defined(__clang__) || __clang_major__ >= 16
                    auto [_, fun] = obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                    , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0];
#else
                    auto fun = std::get<1>(obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                    , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0]);
#endif
                    auto & first_source = *deq.begin();
                    auto & second_source = *(deq.begin() + 1);
                    std::visit([&](auto &&arg) {

                        max_field_size_checker f_size_checker(*std::get_if<0>(&first_source), args, arg.cols(), init_row{args.no_header ? 1u : 2u});

                        arg.run_rows([&](auto &span) {
                            if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span[0])>, std::string>)
                                check_max_size(span, f_size_checker);

                            std::visit([&](auto &&arg) {

                                max_field_size_checker s_size_checker(*std::get_if<0>(&second_source), args, arg.cols(), init_row{args.no_header ? 1u : 2u});

                                arg.run_rows([&](auto &span1) {

                                    if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span1[0])>, std::string>)
                                        check_max_size(span1, s_size_checker);

                                    std::visit([&](auto &&cmp) {
                                        if (!cmp(elem_t{span[c_ids[0]]}, elem_t{span1[c_ids[1]]})) {
                                            std::vector<std::string> join_vec;
                                            assert(types1.size() == span1.size());
                                            join_vec.reserve(span.size() + span1.size() - 1);
                                            join_vec.assign(span.begin(), span.end());
                                            join_vec.insert(join_vec.end(), span1.begin(), span1.begin() + c_ids[1]);
                                            join_vec.insert(join_vec.end(), span1.begin() + c_ids[1] + 1, span1.end());
                                            impl.add(std::move(join_vec));
                                        }
                                    }, fun);
                                });
                            }, second_source);
                        });
                    }, first_source);
                }

                cycle_cleanup();
                deq.push_front(std::move(impl));
            }
        };

        auto left_or_right_join = [&] {

            assert(!c_ids.empty());
            while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
                auto const & [types0, blanks0] = ts_n_blanks[0];
                auto const & [types1, blanks1] = ts_n_blanks[1];
#else
                auto const & types0 = std::get<0>(ts_n_blanks[0]);
                auto const & blanks0 = std::get<1>(ts_n_blanks[0]);
                auto const & types1 = std::get<0>(ts_n_blanks[1]);
                auto const & blanks1 = std::get<1>(ts_n_blanks[1]);
#endif
                reader_fake<reader_type> impl{0, 0};

                auto can_compare = [&]() {
                    return (types0[c_ids[0]] == types1[c_ids[1]]) or args.no_inference;
                };

                auto compose_distinct_record = [&](auto const &span) {
                    std::vector<std::string> join_vec;
                    join_vec.reserve(span.size() + types1.size() - 1);
                    join_vec.assign(span.begin(), span.end());
                    std::vector<std::string> empty_vec(types1.size() - 1);
                    join_vec.insert(join_vec.end(), empty_vec.begin(), empty_vec.end());
                    return join_vec;
                };

                bool recalculate_types_blanks = false;
                (void)recalculate_types_blanks; // it is really used (Clang bug!)
                if (can_compare()) {
                    using namespace ::csvsuite::cli::compare::detail;
                    using elem_t = typename std::decay_t<decltype(std::get<0>(deq.front()))>::template typed_span<quoted>;
#if !defined(__clang__) || __clang_major__ >= 16
                    auto [_, fun] = obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                    , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0];
#else
                    auto fun = std::get<1>(obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                    , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0]);
#endif
                    auto & first_source = *deq.begin();
                    auto & second_source = *(deq.begin() + 1);

                    std::visit([&](auto &&arg) {

                        max_field_size_checker f_size_checker(*std::get_if<0>(&first_source), args, arg.cols(), init_row{args.no_header ? 1u : 2u});

                        arg.run_rows([&](auto &span) {

                            if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span[0])>, std::string>)
                                check_max_size(span, f_size_checker);

                            std::visit([&](auto &&arg) {

                                bool were_joins = false;
                                max_field_size_checker s_size_checker(*std::get_if<0>(&second_source), args, arg.cols(), init_row{args.no_header ? 1u : 2u});

                                arg.run_rows([&](auto &span1) {

                                    if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span1[0])>, std::string>)
                                        check_max_size(span1, s_size_checker);

                                    std::visit([&](auto &&cmp) {
                                        if (!cmp(elem_t{span[c_ids[0]]}, elem_t{span1[c_ids[1]]})) {
                                            std::vector<std::string> join_vec;
                                            assert(types1.size() == span1.size());
                                            join_vec.reserve(span.size() + span1.size() - 1);
                                            join_vec.assign(span.begin(), span.end());
                                            join_vec.insert(join_vec.end(), span1.begin(), span1.begin() + c_ids[1]);
                                            join_vec.insert(join_vec.end(), span1.begin() + c_ids[1] + 1, span1.end());
                                            impl.add(std::move(join_vec));
                                            were_joins = true;
                                        }
                                    }, fun);
                                });
                                if (!were_joins) {
                                    impl.add(std::move(compose_distinct_record(span)));
                                    recalculate_types_blanks = true;
                                }
                            }, second_source);
                        });
                    }, first_source);
                } else {
                    std::visit([&impl, &compose_distinct_record](auto &&arg) {
                        arg.run_rows([&](auto &span) {
                            impl.add(std::move(compose_distinct_record(span)));
                        });
                    }, *deq.begin());
                    recalculate_types_blanks = true;
                }
                cycle_cleanup();
                deq.push_front(std::move(impl));
            }
        };

#if !defined(_MSC_VER)
        auto outer_join = [&] {
            assert(!c_ids.empty());
            assert (!args.left_join and !args.right_join and args.outer_join);
            while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
                auto const & [types0, blanks0] = ts_n_blanks[0];
                auto const & [types1, blanks1] = ts_n_blanks[1];
#else
                auto const & types0 = std::get<0>(ts_n_blanks[0]);
                auto const & blanks0 = std::get<1>(ts_n_blanks[0]);
                auto const & types1 = std::get<0>(ts_n_blanks[1]);
                auto const & blanks1 = std::get<1>(ts_n_blanks[1]);
#endif
                reader_fake<reader_type> impl{0, 0};
                auto can_compare = [&]() {
                    return (types0[c_ids[0]] == types1[c_ids[1]]) or args.no_inference;
                };

                auto compose_distinct_left_part = [&](auto const &span) {
                    std::vector<std::string> join_vec;
                    join_vec.reserve(span.size() + types1.size());
                    join_vec.assign(span.begin(), span.end());
                    std::vector<std::string> empty_vec(types1.size());
                    join_vec.insert(join_vec.end(), empty_vec.begin(), empty_vec.end());
                    return join_vec;
                };

                auto compose_distinct_right_part = [&](auto const &span) {
                    std::vector<std::string> join_vec;
                    join_vec.reserve(span.size() + types0.size());
                    std::vector<std::string> empty_vec(types0.size());
                    join_vec.assign(empty_vec.begin(), empty_vec.end());
                    join_vec.insert(join_vec.end(), span.begin(), span.end());
                    return join_vec;
                };

                bool recalculate_types_blanks = false;
                if (can_compare()) {
                    using namespace ::csvsuite::cli::compare::detail;
                    using elem_t = typename std::decay_t<decltype(std::get<0>(deq.front()))>::template typed_span<quoted>;
#if !defined(__clang__) || __clang_major__ >= 16
                    auto [_, fun] = obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                    , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0];
#else
                    auto fun = std::get<1>(obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                    , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0]);
#endif
                    auto & first_source = *deq.begin();
                    auto & second_source = *(deq.begin() + 1);

                    std::visit([&](auto &&arg) {
                        max_field_size_checker f_size_checker(*std::get_if<0>(&first_source), args, arg.cols(), init_row{args.no_header ? 1u : 2u});
                        arg.run_rows([&](auto &span) {
                            if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span[0])>, std::string>)
                                check_max_size(span, f_size_checker);

                            std::visit([&](auto &&arg) {
                                bool were_joins = false;
                                max_field_size_checker s_size_checker(*std::get_if<0>(&second_source), args, arg.cols(), init_row{args.no_header ? 1u : 2u});
                                arg.run_rows([&](auto &span1) {
                                    if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span1[0])>, std::string>)
                                        check_max_size(span1, s_size_checker);

                                    std::visit([&](auto &&cmp) {
                                        if (!cmp(elem_t{span[c_ids[0]]}, elem_t{span1[c_ids[1]]})) {
                                            std::vector<std::string> join_vec;
                                            assert(types1.size() == span1.size());
                                            join_vec.reserve(span.size() + span1.size());
                                            join_vec.assign(span.begin(), span.end());
                                            join_vec.insert(join_vec.end(), span1.begin(), span1.end());
                                            impl.add(std::move(join_vec));
                                            were_joins = true;
                                        }
                                    }, fun);
                                });
                                if (!were_joins)
                                    impl.add(std::move(compose_distinct_left_part(span)));
                            }, second_source);
                        });
                    }, first_source);

                    // Here, outer join right table
                    std::visit([&](auto &&arg) {
                        arg.run_rows([&](auto &span) {
                            std::visit([&](auto &&arg) {
                                bool were_joins = false;
                                arg.run_rows([&](auto &span1) {
                                    std::visit([&](auto &&cmp) {
                                        if (!cmp(elem_t{span[c_ids[1]]}, elem_t{span1[c_ids[0]]}))
                                            were_joins = true;
                                    }, fun);
                                });
                                if (!were_joins) {
                                    impl.add(std::move(compose_distinct_right_part(span)));
                                    recalculate_types_blanks = true;
                                }
                            }, first_source);
                        });
                    }, second_source);
                } else {
                    std::visit([&](auto &&arg) {
                        arg.run_rows([&impl, &compose_distinct_left_part](auto &span) {
                            impl.add(std::move(compose_distinct_left_part(span)));
                        });
                    }, *deq.begin());
                    std::visit([&](auto &&arg) {
                        arg.run_rows([&impl, &compose_distinct_right_part](auto &span) {
                            impl.add(std::move(compose_distinct_right_part(span)));
                        });
                    }, *(deq.begin() + 1));
                    recalculate_types_blanks = true;
                }

                cycle_cleanup(exclude_c_column::no);

                if (recalculate_types_blanks && !deq.empty()) {
                    typename std::decay_t<decltype(std::get<0>(deq.front()))> tmp_reader(impl.operator typename reader_fake<reader_type>::table &());
                    struct {
                        bool no_header = true;
                        unsigned skip_lines = 0;
                        std::string num_locale;
                        std::string datetime_fmt;
                        std::string date_fmt;
                        bool no_leading_zeroes;
                        bool blanks{};
                        mutable std::vector<std::string> null_value;
                        bool no_inference{};
                        bool date_lib_parser{};
                        unsigned maxfieldsize = max_unsigned_limit;
                    } tmp_args{
                        true
                        , 0
                        , args.num_locale
                        , args.datetime_fmt
                        , args.date_fmt
                        , args.no_leading_zeroes
                        , args.blanks
                        , args.null_value
                        , args.no_inference
                        , args.date_lib_parser
                        , args.maxfieldsize
                    };
                    ts_n_blanks[0] = std::get<1>(typify::typify(tmp_reader, tmp_args, typify_option::typify_without_precisions));
                }
                deq.push_front(std::move(impl));
                assert(deq.size() == ts_n_blanks.size());
                assert(deq.size() == c_ids.size());
            }

        };
#endif

        if (deq.empty())
            return;

        bool const join = deq.size() > 1;

        if (c_ids.empty())  // column ids are unspecified : UNION
            union_join();
        else if (!args.outer_join && !args.left_join && !args.right_join)        // just -c 1
            pure_c_join();
        else if (args.left_join || args.right_join)
            left_or_right_join();
        else {
#if !defined(_MSC_VER)
            outer_join();
#else
            throw std::runtime_error("MSVC can't handle to compile the code for this operation!");
#endif
        }
        print_results(join);
    }
} //  namespace csvjoin

namespace csvjoin::detail {
    void check_arg_semantics (auto const & args) {
        if (args.left_join and args.right_join)
            throw std::runtime_error("It is not valid to specify both a left and a right join.");

        if ((args.left_join or args.right_join or args.outer_join) and args.columns.empty())
            throw std::runtime_error("You must provide join column names when performing an outer join.");
    }
    auto get_join_column_names (auto const & args) {
        std::vector<std::string> join_column_names;
        if (!args.columns.empty()) {
            join_column_names = parse_join_column_names(args.columns);
            if (join_column_names.size() == 1) {
                join_column_names.resize(args.files.size());
                std::fill(join_column_names.begin()+1, join_column_names.end(), join_column_names[0]);
            }
            if (join_column_names.size() != args.files.size())
                throw std::runtime_error ("The number of join column names must match the number of files, or be a single "
                                          "column name that exists in all files.");
        }
        return join_column_names;
    }
} // namespace csvjoin::detail

namespace csvjoin {
    using namespace detail::typify;
    void join_wrapper(auto const & args, csvjoin_source_option source_type = csvjoin_source_option::csvjoin_file_source) {
        using namespace csvjoin::detail;

        check_arg_semantics(args);
        auto join_column_names = get_join_column_names(args);

        std::variant<std::deque<notrimming_reader_or_fake_type>, std::deque<skipinitspace_reader_or_fake_type>> variants;

        auto fill_deque = [&args, &variants, source_type](auto & deq) {
            if (args.files.empty()) {
#if !defined(BOOST_UT_DISABLE_MODULE)
                deq.emplace_back(std::variant_alternative_t<0, typename std::decay_t<decltype(deq)>::value_type> {args.std_input});
#endif
            } else {
                for (auto && elem : args.files)
                    if (source_type == csvjoin_source_option::csvjoin_file_source)
                        deq.emplace_back(std::variant_alternative_t<0, typename std::decay_t<decltype(deq)>::value_type> {std::filesystem::path{elem}});
                    else
                        deq.emplace_back(std::variant_alternative_t<0, typename std::decay_t<decltype(deq)>::value_type>{elem});
            }
            if (args.right_join)
                std::reverse(deq.begin(), deq.end());

            variants = std::move(deq);
        };

        if (!args.skip_init_space) {
            std::deque<notrimming_reader_or_fake_type> d;
            fill_deque(d);
        } else {
            std::deque<skipinitspace_reader_or_fake_type> d;
            fill_deque(d);
        }
        std::visit([&](auto && arg) { join(arg, args, join_column_names);}, variants);
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)
int main(int argc, char * argv[])
{
    using namespace csvjoin;

    auto args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        if (args.files.empty())
            args.std_input = read_standard_input(args);
        join_wrapper(args);
    } catch (notrimming_reader_type::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (skipinitspace_reader_type::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
    return 0;
}
#endif //BOOST_UT_DISABLE_MODULE
