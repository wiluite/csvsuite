///
/// \file   suite/csvGrep.cpp
/// \author wiluite
/// \brief  Search CSV files.

#include <cli.h>
#include <regex>
#include <printer_concepts.h>

using namespace ::csvsuite::cli;

namespace csvgrep {
    struct Args final : ARGS_positional_1 {
        bool & names = flag ("n,names","Display column names and indices from the input CSV and exit.");
        std::string & columns = kwarg("c,columns","A comma-separated list of column indices, names or ranges to be searched, e.g. \"1,id,3-5\".").set_default("none");
        std::string & match = kwarg("m,match","A string to search for.").set_default("");
        std::string & regex = kwarg("r,regex","A regular expression to match.").set_default("");
        bool & r_icase = flag("r-icase","Character matching should be performed without regard to case.");
        std::string & f = kwarg("f,file","A path to a file. For each row, if any line in the file (stripped of line separators) is an exact match of the cell value, the row matches.").set_default("");
        bool & invert = flag("i,invert-match","Select non-matching rows, instead of matching rows.");
        bool & any = flag("a,any-match", "Select rows in which any column matches, instead of all columns.");
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nSearch CSV files. Like the Unix \"grep\" command, but for tabular data.\n\n";
        }
    };

    void grep(std::monostate &, auto const &) {}

    template <class OS>
    class printer {
        OS &os;
    public:
        explicit printer(OS &os = std::cout) : os(os) {}
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
            } else {
                assert(false);
            }
        }

        template<typename CellStringT = csv_co::cell_string, typename Container>
        void write(Container &&row, auto && args, std::size_t line) requires CellSpanRowConcept<Container> {
            if (args.linenumbers)
                os << line << ',';

            std::copy(row.begin(), row.end() - 1, std::ostream_iterator<CellStringT>(os, ","));
            std::copy(row.end() - 1, row.end(), std::ostream_iterator<CellStringT>(os, ""));
            print_LF(os);
        }
    };

    void grep(auto & reader, auto const & args) {
        using namespace csv_co;

        skip_lines(reader, args);
        quick_check(reader, args);
        auto const header = obtain_header_and_<skip_header>(reader, args);
        check_max_size(reader, args, header, init_row{1});

        if (args.names) {
            print_header(std::cout, header, args);
            return;
        }

        args.columns = args.columns == "none" ? "" : args.columns;

        if (args.columns.empty())
            throw std::runtime_error("You must specify at least one column to search using the -c option.");

        if (args.regex.empty() and args.f.empty() and args.match.empty())
            throw std::runtime_error("One of -r, -m or -f must be specified, unless using the -n option.");

        if (args.regex.empty() and args.r_icase)
            throw std::runtime_error("Syntax option type --r-icase is for regex search only.");

        try {
            auto ids = parse_column_identifiers(columns{args.columns}, header, get_column_offset(args), excludes(std::string{}));

            std::ostringstream oss;
            std::ostream & oss_ = args.asap ? std::cout : oss;
            printer p{oss_};
            // csvstack seems to have non-typed output
            std::vector<std::string> string_header(header.size());
            std::transform(header.begin(), header.end(), string_header.begin(), [](auto & elem) {return compose_text(elem);});

            p.write(string_header, args);

            auto all_hits = [&](auto const & span, auto f) {
                auto hits = 0u;
                for (auto idx : ids)
                    if (f(span,idx))
                        hits++;
                return hits == ids.size();
            };

            auto any_hits = [&](auto const & span, auto f) {
                auto hits = 0u;
                for (auto idx : ids) {
                    if (f(span,idx)) {
                        hits++;
                        break;
                    }
                }
                return hits != 0;
            };

            using row_span_type = typename std::decay_t<decltype(reader)>::row_span;
            std::function<bool(row_span_type, std::function<bool(row_span_type, unsigned)>)> func;

            if (args.any)
                func = any_hits;
            else
                func = all_hits;

            auto search_and_output = [&] (auto hit_func) {
                std::size_t row = 1;
                auto const ir = init_row{args.no_header ? 1u : 2u};
                reader.run_rows([&](auto & row_span) {
                    check_max_size(reader, args, row_span, ir);
                    auto const chk_result = func(row_span, hit_func);
                    if ((chk_result and !args.invert) or (!chk_result and args.invert))
                        p.write<cell_string>(row_span, args, row);
                    ++row;
                });
                if(!args.asap)
                    std::cout << oss.str();
            };

            // in priority order:
            if (!args.regex.empty()) {
                auto regex_constants = std::regex_constants::ECMAScript;
                if (args.r_icase)
                    regex_constants |= std::regex_constants::icase;

                const std::regex expr(args.regex, regex_constants);

                search_and_output([&](auto const & span, auto idx) {
                    return (std::regex_search(span[idx].operator cell_string(), expr) or
                            std::regex_search(span[idx].operator unquoted_cell_string(), expr));
                });

            } else
            if (!args.f.empty()) {
                std::ifstream f (args.f);
                if (!f.good()) {
                    std::cout << "Error argument -f/--file: can't open '" << args.f <<"': " << strerror(errno) << ".\n";
                    return;
                }
                std::unordered_set<std::string> s_set;
                while (!f.eof()) {
                    static std::string temp;
                    std::getline(f, temp);
                    if (!temp.empty())
                        s_set.insert(temp);
                }
                auto hit_func = [&](auto const & span, auto idx) {
                    for (auto const & e : s_set) {
                        if (span[idx].operator cell_string().find(e) != std::string::npos or
                            span[idx].operator unquoted_cell_string().find(e) != std::string::npos)
                            return true;
                    }
                    return false;
                };

                search_and_output(hit_func);

            } else
            if (!args.match.empty()) {
                search_and_output([&](auto const & span, auto idx) {
                    return (span[idx].operator cell_string().find(args.match) != std::string::npos or
                            span[idx].operator unquoted_cell_string().find(args.match) != std::string::npos);
                });

                return;
            }

        }  catch (ColumnIdentifierError const& e) {
            std::cerr << e.what() << '\n';
        }
    }

} ///namespace

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {

    using namespace csvgrep;

    auto args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    basic_reader_configurator_and_runner(read_standard_input, grep)

    return 0;
}

#endif
