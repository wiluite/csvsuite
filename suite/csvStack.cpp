///
/// \file   suite/csvStack.cpp
/// \author wiluite
/// \brief  Stack up the rows from multiple CSV files.

#include <cli.h>
#include <printer_concepts.h>
#include <deque>
#include "external/glob/glob/glob.h"

using namespace ::csvsuite::cli;

namespace csvstack::detail {
    struct Args final : ARGS_positional_files {

        std::string & groups = kwarg("g,groups", "A comma-separated list of values to add as \"grouping factors\", one per CSV being stacked. These are "
            "added to the output as a new column. You may specify a name for the new column using the -n flag.").set_default("empty");
        std::string & group_name = kwarg("n,group-name", "A name for the grouping column, e.g. \"year\". Only used when also specifying -g.").set_default("");
        bool & filenames = flag("filenames", "Use the filename of each input file as its grouping value. When specified, -g will be ignored.");
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nStack up the rows from multiple CSV files, optionally adding a grouping value.\n\n";
        }
    };

    template <class OS>
    class printer {
        OS &os;
    public:
        explicit printer(OS &os = std::cout) : os(os) {}
        template <CsvKitCellSpanTableConcept Table>
        void write(Table & table, auto const & types_blanks, auto const & args)  {
            for (auto const & row : table) {
                write(row, types_blanks, args);
                print_CRLF(os);
            }
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
            } else {
            }
        }

        template <typename T>
        void write(T&& printable, auto && types_n_blanks, auto && args)
        requires (!CsvKitCellSpanTableConcept<T>
                  && !CsvKitCellSpanRowConcept<T>
                  && !CellSpanRowConcept<T>
                  && !CsvReaderConcept<T>) {
            if constexpr (std::is_same_v<std::vector<std::string>, std::decay_t<T>>) { // print header
                if (args.linenumbers)
                    os << "line_number,";

                std::for_each(printable.begin(), printable.end()-1, [&](auto const & elem) {
                    os << elem << ',';
                });
                os << printable.back();
                print_LF(os);
            } else
            if constexpr (std::is_same_v<std::vector<std::string>, std::decay_t<decltype(std::declval<T>()[0])>>) { // print body
                for (auto && elem : printable) {
                    if (args.linenumbers) {
                        static unsigned line_nums = 0;
                        os << ++line_nums << ',';
                    }

                    auto col = 0u;
                    std::for_each(elem.begin(), elem.end()-1, [&](auto const & e) {
                        os << e << ',';
                    });
                    os << elem.back();
                    print_LF(os);
                }
            } else {
                std::cerr << "No implementation for printing of this kind of table\n";
                std::cerr << type_name<T>() << std::endl;
            }
        }

    };

//TODO: -no-table option
    auto parse_groups (auto && groups) {
        if (groups.empty())
            return std::vector<std::string> {{""}};
        std::istringstream stream (groups);
        std::vector<std::string> result;
        for (std::string word; std::getline(stream,word,',') ;)
            result.push_back(word);
        return result;
    }

    auto obtain_origins_and_headers(auto & readers_vector, auto const & args) {
        auto rows = 0u;
        std::unordered_set<std::string> header_fields;
        std::vector<std::vector<std::string>> headers;
        for (auto & elem : readers_vector) {
            std::visit([&](auto && r) {
                r.skip_rows(0);
                skip_lines(r, args);
                auto const header = obtain_header_and_<skip_header>(r, args);
                check_max_size(r, args, header, init_row{1});
                rows += r.rows();
                header_fields.insert(header.begin(), header.end());
                std::vector<std::string> string_header(header.size());
                std::transform(header.begin(), header.end(), string_header.begin(), [](auto & elem) {return compose_text(elem);});
                headers.push_back(string_header);
            }, elem);
        }
        return std::tuple(rows, header_fields.size(), headers);
    }

    auto put_first(auto & r_man, auto const & args, auto cols, auto const & group_names) {
        return std::visit([&](auto && r) {
            r.skip_rows(0);
            skip_lines(r, args);
            obtain_header_and_<skip_header>(r, args);
            max_field_size_checker size_checker(r, args, r.cols(), init_row{args.no_header ? 1u : 2u});
            bool const groups_or_filenames = args.groups != "empty" or args.filenames;
            r.run_rows([&](auto & row_span) {
                check_max_size(row_span, size_checker);
                if (groups_or_filenames)
                    std::cout << (args.filenames ? args.files[0] : group_names[0]) << ',';

                std::cout << row_span.front().operator csv_co::cell_string();
                std::for_each (row_span.begin() + 1, row_span.end(), [](auto & elem) {
                    std::cout << ',' << elem.operator csv_co::cell_string();
                });
                auto i = row_span.size() + (groups_or_filenames ? 1 : 0);
                while (i++ < cols)
                    std::cout << ',';
                std::cout << '\n';
            });
            r_man.get_readers().pop_front();
        }, r_man.get_readers()[0]);
    }

    void put_rest(auto & r_man, auto const & args, auto total_cols, auto const& replace_vec, auto const & group_names) {
        auto replace_idx = 0ul;
        auto group_idx = 1ul;
        for (auto & reader_elem : r_man.get_readers()) {
            std::visit([&](auto & r) {
                r.skip_rows(0);
                skip_lines(r, args);
                obtain_header_and_<skip_header>(r, args);
                unsigned const cols = r.cols();

                max_field_size_checker size_checker(r, args, cols, init_row{args.no_header ? 1u : 2u});
                bool const groups_or_filenames = args.groups != "empty" or args.filenames;
                std::vector<std::string> row(total_cols);
                r.run_rows([&](auto & row_span) {
                    check_max_size(row_span, size_checker);
                    if (groups_or_filenames)
                        row[0] = args.filenames ? args.files[group_idx] : group_names[group_idx];

                    auto col_idx = 0;
                    for (auto & elem : row_span)
                        row[replace_vec[replace_idx + col_idx++]] = elem;

                    std::cout << row.front();
                    std::for_each(row.begin() + 1, row.end(), [](auto & elem) {
                        std::cout << ',' << elem;
                    });
                    std::cout << '\n';
                });

                replace_idx += cols;
                group_idx++;
            }, reader_elem);
        };
    }

    auto fill_replace_vec(auto const & headers, auto & replace_vec, auto const & args) {
        std::vector<std::string> final_header = {headers[0].begin(),headers[0].end()};

        if (args.groups != "empty" or args.filenames)
            final_header.insert(final_header.begin(), args.group_name.empty() ? "group" : args.group_name);

        for_each(headers.begin()+1, headers.end(), [&](auto & header) {
            for (auto & col_name : header) {
                auto const it = std::find(final_header.begin(), final_header.end(), col_name);
                if (it == final_header.end()) {
                    final_header.push_back(col_name);
                    replace_vec.push_back(final_header.size()-1);
                } else
                    replace_vec.push_back(it-final_header.begin());
            }
        });
        return final_header;
    }

    template <typename ... r_types>
    struct readers_manager {
        auto & get_readers() {
            return readers;
        }

        template<typename ReaderType>
        auto set_readers(auto & args) {
            if (args.files.empty() or (args.files.size() == 1 and args.files[0] == "_")) {
                if (isatty(STDIN_FILENO))
                    std::cerr << "No input file or piped data provided. Waiting for standard input:\n";
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

            for (auto & elem : args.files) {
                auto reader {elem != "_" ? ReaderType{std::filesystem::path{elem}} : (ReaderType{read_standard_input(args)})};
                recode_source(reader, args);
                skip_lines(reader, args);
                quick_check(reader, args);
                get_readers().push_back(std::move(reader));
            }
        }
    private:
        std::deque<std::variant<r_types...>> readers;
    };

}

namespace csvstack {

    template <typename ReaderType>
    void stack(auto & args) {
        using namespace csv_co;
        using namespace detail;
        std::vector<std::string> group_names;
        if (args.groups != "empty") {
            group_names = parse_groups(args.groups);
            if (group_names.size() != args.files.size())
                throw std::runtime_error("The number of grouping values must be equal to the number of CSV files being stacked.");
        }

        readers_manager<ReaderType> r_man;
        r_man.template set_readers<ReaderType>(args);

        auto [rows, cols, headers] = obtain_origins_and_headers(r_man.get_readers(), args);

        std::vector<unsigned> replace_vec;
        auto const header = fill_replace_vec(headers, replace_vec, args);
        auto const total_cols = cols + (args.groups == "empty" && !args.filenames ? 0 : 1);

        std::ostringstream oss;
        std::ostream & oss_ = args.asap ? std::cout : oss;

        printer p(oss_);
        p.write(header, args);

        put_first(r_man, args, total_cols, group_names);
        put_rest(r_man, args, total_cols, replace_vec, group_names);

        if (!args.asap)
            std::cout << oss.str();
    }

} /// namespace

#if !defined(BOOST_UT_DISABLE_MODULE)
int main(int argc, char * argv[]) {
    auto args = argparse::parse<csvstack::detail::Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        if (!args.skip_init_space)
            csvstack::stack<notrimming_reader_type>(args);
        else
            csvstack::stack<skipinitspace_reader_type>(args);
    }
    catch (std::exception const & e) {
        std::cerr << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception.\n";
    }
}
#endif
