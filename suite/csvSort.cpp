///
/// \file   suite/csvSort.cpp
/// \author wiluite
/// \brief  Sort CSV files.

#include <cli.h>
#include <cli-compare.h>
#include <printer_concepts.h>
#include "external/poolstl/poolstl.hpp"

using namespace ::csvsuite::cli;

namespace csvsort {

    struct Args final : ARGS_positional_1 {
        std::string & num_locale = kwarg("L,locale","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool & blanks = flag("blanks",R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> & null_value = kwarg("null-value", "Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        bool & names = flag ("n,names","Display column names and indices from the input CSV and exit.");
        std::string & columns = kwarg("c,columns","A comma-separated list of column indices, names or ranges to sort by, e.g. \"1,id,3-5\".").set_default("all columns");
        bool & r = flag("r,reverse", "Sort in descending order.");
        bool & ignore_case = flag("i,ignore-case", "Perform case-independent sorting.");
        bool & no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        bool & date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported.").set_default(true);
        bool & parallel_sort = flag("p,parallel-sort", "Use parallel sort.").set_default(true);
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nSort CSV files. Like the Unix \"sort\" command, but for tabular data.\n\n";
        }
    };

    void sort(std::monostate &, auto const  &) {}

    template <class OS>
    class printer {
        OS &os;
    public:
        explicit printer(OS &os = std::cout) : os(os) {}
        template <CsvKitCellSpanTableConcept Table>
        void write(Table & table, auto const & types_blanks, auto const & args)  {
            for (auto const & row : table) {
                write(row, types_blanks, args);
                print_LF(os);
            }
        }

        // Used by write(Table&& table, auto && types_n_blanks, auto && args) above.
        template<typename Container/*, typename CellString = cell_string*/>
        void write(Container const & row, auto const & types_blanks, auto const & args) requires CsvKitCellSpanRowConcept<Container> {
            // typify and compromise_table_MxN filler can work with different cells (quoted and unquoted in same session)
            // So we must "imbue" date and datetime formats when printing almost common cells
            static auto date_format_provider = row.front().date(args.date_fmt);
            (void)date_format_provider;
            static auto datetime_format_provider = row.front().datetime(args.datetime_fmt);
            (void)datetime_format_provider;
#if !defined(__clang__) || __clang_major__ >= 16
            auto & [types, blanks] = types_blanks;
#else
            auto & types = std::get<0>(types_blanks);
            auto & blanks = std::get<1>(types_blanks);
#endif
            auto print_func_impl = [&] (auto && elem_str) {
                os << elem_str;
            };

            auto print_func = [&](auto && elem, std::size_t col) {
                using e_type = std::decay_t<decltype(elem)>;
                static_assert(e_type::is_quoted());
                using UElemType = typename e_type::template rebind<csv_co::unquoted>::other;
                auto & unquoted_elem = elem.operator UElemType const&();

                if (unquoted_elem.is_null_value())
                    return;

                bool const is_null = unquoted_elem.is_null();
                if (types[col] == column_type::text_t or (!args.blanks && is_null)) {
                    os << (!args.blanks && is_null ? "" : compose_text(unquoted_elem));
                    return;
                }

                using func_type = std::function<std::string(UElemType const &)>;

                static std::array<func_type, static_cast<std::size_t>(column_type::sz) - 1> type2func {
                        compose_bool_1_arg<UElemType>
                        , [&args](UElemType const & e) {
                            static std::ostringstream ss;
                            compose_numeric(ss, e, args);
                            return ss.str();
                        }
                        , compose_datetime_1_arg<UElemType>
                        , compose_date_1_arg<UElemType>
                        , compose_timedelta_1_arg<UElemType>
                };

                auto const type_index = static_cast<std::size_t>(types[col]) - 1;
                print_func_impl(type2func[type_index](unquoted_elem));
            };

            if (args.linenumbers) {
                static unsigned line_nums = 0;
                os << ++line_nums << ',';
            }

            unsigned col = 0;
            std::for_each(row.begin(), row.end() - 1, [&](auto & elem) {
                print_func(elem, col++);
                os << ',';
            });
            print_func(row.back(), col);
        }

        template <typename T>
        void write(T&& printable, auto && args)
        requires (!CsvKitCellSpanTableConcept<T> && !CsvKitCellSpanRowConcept<T> && !CellSpanRowConcept<T> && !CsvReaderConcept<T>) {
            if constexpr (std::is_same_v<std::vector<std::string>, std::decay_t<T>>) { // print header
                if (args.linenumbers)
                    os << "line_number,";

                std::for_each(printable.begin(), printable.end()-1, [&](auto const & elem) {
                    os << elem << ',';
                });
                os << printable.back();
                print_LF(os);
            } else
                assert(false);
        }

    };

    template <typename Reader, typename Args>
    void setup_string_comparison_type(Reader &, Args const &args) {
        using unquoted_elem_type = typename Reader::template typed_span<csv_co::unquoted>;
        unquoted_elem_type::case_insensitivity(args.ignore_case);
    }

    void sort(auto & reader, auto const & args) {
        using namespace csv_co;

        skip_lines(reader, args);
        quick_check(reader, args);

        auto const header = obtain_header_and_<skip_header>(reader, args);

        if (args.names) {
            print_header(std::cout, header, args);
            return;
        }

        try {
            args.columns = args.columns == "all columns" ? "" : args.columns;
            std::string not_columns;
            auto const ids = parse_column_identifiers(columns{args.columns}, header, get_column_offset(args), excludes{not_columns});

            setup_string_comparison_type(reader, args);

            auto const types_blanks = std::get<1>(typify(reader, args, typify_option::typify_without_precisions));

            using namespace ::csvsuite::cli::compare;
            // Filling in data to sort.
            // It is sufficient to have csv_co::quoted cell_spans in it, because comparison is quite sophisticated and takes it into account
            compromise_table_MxN table(reader, args);

            auto cfa = obtain_compare_functionality<std::decay_t<decltype(table[0][0])>>(ids, types_blanks, args);
            if (args.r) {
                if (args.parallel_sort)
                    std::sort(poolstl::par, table.begin(), table.end(), sort_comparator(std::move(cfa), std::greater<>()));
                else
                    std::sort(table.begin(), table.end(), sort_comparator(std::move(cfa), std::greater<>()));
            }
            else {
                if (args.parallel_sort)
                    std::sort(poolstl::par, table.begin(), table.end(), sort_comparator(std::move(cfa), std::less<>()));
                else
                    std::sort(table.begin(), table.end(), sort_comparator(std::move(cfa), std::less<>()));
            }
            // Force detecting types for all rest (not-comparable) cells concurrently to reduce result output time
            for_each(poolstl::par, table.begin(), table.end(), [&](
#ifdef _MSC_VER
                    std::decay_t<decltype(table[0])> & item
#else
                    auto & item
#endif
                    ) {
                for (auto & elem : item) {
                    using UElemType = typename std::decay_t<decltype(elem)>::template rebind<csv_co::unquoted>::other;
                    elem.operator UElemType const&().type();
                }
            });

            std::ostringstream oss;
            std::ostream & oss_ = args.asap ? std::cout : oss;
            printer p(oss_);

            std::vector<std::string> string_header(header.size());
            std::transform(header.cbegin(), header.cend(), string_header.begin(), [&](
#ifdef _MSC_VER
                    std::decay_t<decltype(reader)>::cell_span const & elem
#else
                    auto & elem
#endif
                    ) {
                return compose_text(elem);
            });

            p.write(string_header, args);
            p.write(table, types_blanks, args);

            if (!args.asap)
                std::cout << oss.str();

        }  catch (ColumnIdentifierError const& e) {
            std::cerr << e.what() << '\n';
        }

    }

} ///namespace

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[])
{
    using namespace csvsort;

    auto args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(read_standard_input, sort)

    return 0;
}

#endif
