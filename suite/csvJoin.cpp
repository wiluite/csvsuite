///
/// \file   suite/csvJoin.cpp
/// \author wiluite
/// \brief  Execute a SQL-like join to merge CSV files.

#include <cli.h>
#include <cli-compare.h>
#include <cli-hash.h>
#include "type_name.h"
#include <printer_concepts.h>
#include "external/poolstl/poolstl.hpp"
#include "external/glob/glob/glob.h"

using namespace ::csvsuite::cli;
using namespace ::csvsuite::cli::compare;
using namespace ::csvsuite::cli::hash;

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
        bool &honest_outer_join = flag("honest-outer", "Typify outer joins result before printing.");
        bool &left_join = flag("left", "Perform a left outer join, rather than the default inner join. If more than two files are provided this will be executed as a sequence of left outer joins, starting at the left.");
        bool &right_join = flag("right", "Perform a right outer join, rather than the default inner join. If more than two files are provided this will be executed as a sequence of right outer joins, starting at the right.");
        bool &no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nExecute a SQL-like join to merge CSV files on a specified column or columns.\n\n";
        }
    };
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

        struct row_span : public std::span<cell_span> {};

        template <bool Unquoted = true>
        struct typed_span : public cell_span {
            using class_type = typed_span<Unquoted>;
            using reader_type = reader_fake;

            static std::locale & num_locale() {
                static std::locale loc;
                return loc;
            }
            [[nodiscard]] constexpr unsigned str_size_in_symbols() const {return 0;}
        };

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

                auto optionally_quoted = [this](auto const & elem)->std::ostream& {
                    if (elem.find(',') != std::string::npos)
                        os << std::quoted(elem);
                    else
                        os << elem;
                    return os;
                };
                std::for_each(printable.begin(), printable.end() - 1, [&](auto const & elem) {
                    optionally_quoted(elem) << ',';
                });
                optionally_quoted(printable.back());
                print_LF(os);
            } else if constexpr (std::is_same_v<std::vector<std::string>, std::decay_t<decltype(std::declval<T>()[0])>>) { // print body
                for (auto && elem : printable) {
                    if (args.linenumbers) {
                        static unsigned line_nums = 0;
                        os << ++line_nums << ',';
                    }

                    if constexpr (std::is_same_v<std::decay_t<decltype(types_n_blanks)>, ts_n_blanks_type>) {
                        auto col = 0u;
                        using elem_type = typename std::decay_t<T>::reader_type::template typed_span<csv_co::unquoted>;
                        static_assert(std::is_same_v<std::decay_t<decltype(elem.front())>, std::string>);
                        std::for_each(elem.begin(), elem.end() - 1, [&](auto const & e) {
                            print_func(elem_type{e}, col++, types_n_blanks, args);
                            os << ',';
                        });
                        print_func(elem_type{elem.back()}, col, types_n_blanks, args);
                    } else {
                        std::for_each(elem.begin(), elem.end() - 1, [&](auto const & e) { os << e << ','; });
                        os << elem.back();
                    }
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

                if constexpr (std::is_same_v<std::decay_t<decltype(types_n_blanks)>, ts_n_blanks_type>) {
                    auto col = 0u;
                    using elem_type = typename std::decay_t<decltype(span.back())>::reader_type::template typed_span<csv_co::unquoted>;
                    std::for_each(span.begin(), span.end() - 1, [&](auto const & e) {
                        print_func(elem_type{e}, col++, types_n_blanks, args);
                        os << ',';
                    });
                    print_func(elem_type{span.back()}, col, types_n_blanks, args);
                } else {
                    std::for_each(span.begin(), span.end() - 1, [&](auto const & e) {
                        os << e.operator csv_co::cell_string() << ',';
                    });
                    os << (span.back()).operator csv_co::cell_string();
                }
                print_LF(os);
            });
        }

    private:
        void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args) {
            if (elem.is_null_value())
                return;

            using elem_type = std::decay_t<decltype(elem)>;
            auto & [types, blanks] = types_n_blanks;

            bool const is_null = elem.is_null();

            bool text_or_null = args.outer_join and !args.honest_outer_join
                ? types[col] == column_type::text_t or is_null : types[col] == column_type::text_t or (!args.blanks and is_null);

            if (text_or_null) {
                os << (!args.blanks && is_null ? "" : compose_text(elem));
                return;
            }

            if (!args.outer_join or args.honest_outer_join)
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
                        static std::ostringstream ss;
                        typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                        compose_numeric(ss, another_rep, args);
                        return ss.str();
                    }
                    , compose_datetime_1_arg<elem_type>
                    , compose_date_1_arg<elem_type>
                    , compose_timedelta_1_arg<elem_type>
            };
            auto const type_index = static_cast<std::size_t>(types[col]) - 1;
            os << type2func[type_index](elem);
        }
    };

    void join(auto && deq, auto const & args, auto join_column_names) {
        using namespace csv_co;
        using reader_type = std::variant_alternative_t<0, typename std::decay_t<decltype(deq)>::value_type>;
        using args_type = std::decay_t<decltype(args)>;
        std::deque<unsigned> c_ids;
        std::deque<std::vector<std::string>> headers;
        std::deque<std::tuple<std::vector<column_type>, std::vector<unsigned char>>> ts_n_blanks;

        if (args.right_join)
            std::reverse(join_column_names.begin(), join_column_names.end());

        #include "include/csvjoin/auxiliary_queue_filler.h"
        #include "include/csvjoin/can_compare.h"
        #include "include/csvjoin/compare_composers.h"
        #include "include/csvjoin/cycle_cleanup.h"
        #include "include/csvjoin/union_join.h"
        #include "include/csvjoin/inner_join.h"
        #include "include/csvjoin/left_right_join.h"
        #include "include/csvjoin/outer_join.h"

        if (deq.empty())
            return;

        if (c_ids.empty())  // column ids are unspecified : UNION
            union_join();
        else if (!args.outer_join && !args.left_join && !args.right_join)        // just -c 1
            inner_join();
        else if (args.left_join || args.right_join)
            left_or_right_join();
        else
            outer_join();

        auto print_results = [&] {
            std::ostringstream oss;
            std::ostream & oss_ = args.asap ? std::cout : oss;
            printer p(oss_);
            struct non_typed_output {};
            p.write(headers[0], non_typed_output{}, args);
            std::visit([&](auto && arg){
                p.write(arg, ts_n_blanks[0], args);
            }, deq.front());
            if (!args.asap)
                std::cout << oss.str();
        };
        print_results();
    }

} //  namespace csvjoin

namespace csvjoin::detail {
    void check_arg_semantics (auto const & args) {
        if (args.left_join and args.right_join)
            throw std::runtime_error("It is not valid to specify both a left and a right join.");

        if ((args.left_join or args.right_join or args.outer_join) and args.columns.empty())
            throw std::runtime_error("You must provide join column names when performing an outer join.");

        if (!args.outer_join and args.honest_outer_join)
            throw std::runtime_error("You cannot provide honest-outer flag without outer joins.");
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

    enum class csvjoin_source_option {
        csvjoin_file_source,
        csvjoin_string_source
    };

} // namespace csvjoin::detail

namespace csvjoin {
    using namespace detail;
    void join_wrapper(auto & args, csvjoin_source_option source_type = csvjoin_source_option::csvjoin_file_source) {
        using namespace csvjoin::detail;

        check_arg_semantics(args);

        std::variant<std::deque<notrimming_reader_or_fake_type>, std::deque<skipinitspace_reader_or_fake_type>> variants;

        auto fill_deque = [&args, &variants, source_type](auto & deq) {

            using real_reader_t = std::variant_alternative_t<0, typename std::decay_t<decltype(deq)>::value_type>;

            if (args.files.empty() or (args.files.size() == 1 and args.files[0] == "_")) {
                if (isatty(STDIN_FILENO))
                    throw std::runtime_error("You must provide an input file or piped data.");
                args.files = std::vector<std::string>{"_"};
            }
            else {
                // process a chance we are dealing with file patterns
                std::vector<std::string> updated_names;
                for (auto & elem : args.files) {
                    if (elem.find_first_of("*?[") != std::string::npos)
                        for (auto& match: glob::glob(elem))
                            updated_names.emplace_back(match.string());
                    else
                        updated_names.emplace_back(std::move(elem));
                }

                if (updated_names.empty())
                    throw std::runtime_error(std::string("Invalid argument: ") + R"(')" + args.files[0] + R"(')");

                args.files = std::move(updated_names);
            }

            for (auto && elem : args.files)
                if (source_type == csvjoin_source_option::csvjoin_file_source) {
                    auto r = elem != "_" ? real_reader_t{std::filesystem::path{elem}} : real_reader_t{read_standard_input(args)};
                    deq.emplace_back(std::move(r));
                }
                else
                    deq.emplace_back(real_reader_t{elem});

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
        std::visit([&](auto && arg) { join(arg, args, get_join_column_names(args));}, variants);
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
        join_wrapper(args);
    } catch (std::exception const & e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
#endif //BOOST_UT_DISABLE_MODULE
