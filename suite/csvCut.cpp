///
/// \file   suite/csvCut.cpp
/// \author wiluite
/// \brief  Filter and truncate a CSV source.

#include <cli.h>
#include <type_traits>
#include <iostream>
#include <deque>

using namespace ::csvsuite::cli;

namespace csvcut {

    struct Args final : ARGS_positional_1 {
        bool & names = flag ("n,names","Display column names and indices from the input CSV and exit.");
        std::string & columns = kwarg("c,columns","A comma-separated list of column indices, names or ranges to be extracted, e.g. \"1,id,3-5\".").set_default("all columns");
        std::string & not_columns = kwarg("C,not-columns","A comma-separated list of column indices, names or ranges to be excluded, e.g. \"1,id,3-5\".").set_default("no columns");
        bool & x_ = flag("x,delete-empty-rows", "After cutting delete rows which are completely empty.");
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nFilter and truncate CSV files. Like the Unix \"cut\" command, but for tabular data.\n\n";
        }
    };

    void cut(std::monostate &, auto const &) {}

    void cut(auto & reader, auto const & args) {
        using namespace csv_co;

        throw_if_names_and_no_header(args);
        skip_lines(reader, args);
        quick_check(reader, args);
        auto const header = obtain_header_and_<no_skip_header>(reader, args);

        if (args.names) {
            check_max_size(reader, args, header, init_row{1});
            print_header(std::cout, header, args);
            return;
        }

        args.columns = args.columns == "all columns" ? "" : args.columns;
        args.not_columns = args.not_columns == "no columns" ? "" : args.not_columns;

        static char delim = std::decay_t<decltype(reader)>::delimiter_type::value;

        try {
            auto const ids = parse_column_identifiers(columns{args.columns}, header, get_column_offset(args), excludes(args.not_columns));
            std::ostringstream oss;
            std::ostream & os = args.asap ? std::cout : oss;
            auto print_container = [&] (auto & row_span, auto const & args) {
                if (args.linenumbers) {
                    if (args.no_header) {
                        if constexpr(!std::is_same_v<typename std::decay_t<decltype(reader)>::row_span, std::decay_t<decltype(row_span)>>) {
                            static auto line = 0ul;
                            if (line)
                                os << line << delim;
                            line++;
                        } else {
                            static auto line = 1ul;
                            if (line)
                                os << line << delim;
                            line++;
                        }
                    } else {
                        if constexpr(std::is_same_v<typename std::decay_t<decltype(reader)>::row_span, std::decay_t<decltype(row_span)>>) {
                            static auto line = 0ul;
                            if (line)
                                os << line << delim;
                            line++;
                        }
                    }
                }
                os << compose_text(row_span[ids.front()]);
                for (auto it = ids.cbegin() + 1; it != ids.cend(); ++it)
                    os << delim << compose_text(row_span[*it]);
                os << '\n';
            };

            struct say_ln {
                explicit say_ln(std::decay_t<decltype(args)> const & args, std::ostream & os) {
                    if (args.linenumbers)
                        os << "line_number" << delim;
                }
            };

            if (args.no_header) {
                check_max_size(reader, args, header, init_row{1});
                static say_ln ln (args, os);
                print_container(header, args);
            }

            reader.run_rows([&] (auto & row_span) {
                static_assert(std::is_same_v<typename std::decay_t<decltype(reader)>::row_span, std::decay_t<decltype(row_span)>>);
                if (!args.no_header)
                    static say_ln ln (args, os);
                static_assert(!std::is_same_v<std::decay_t<decltype(row_span)>, std::decay_t<decltype(header)>>);

                check_max_size(reader, args, row_span, init_row{1});

                if (!args.x_ )
                    print_container(row_span, args);
                else {
                    bool empty = true;
                    for (auto e : ids)
                        empty = empty && row_span[e].operator unquoted_cell_string().empty();

                    if (!empty)
                        print_container(row_span, args);
                }
            });
            if (!args.asap)
                std::cout << oss.str();

        } catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }

} ///namespace 

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[])
{
    using namespace csvcut;

    auto const args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(read_standard_input, cut)

    return 0;
}

#endif
