///
/// \file   utils/csvkit/csvclean.cpp
/// \author wiluite
/// \brief  Produce CSV freed from common errors.

#include <fstream>
#include <cli.h>

using namespace ::csvkit::cli;

#define TRY_TRANSFORM_UTF8_TO_ACTIVE_CODE_PAGE 0
namespace csvclean::detail {
    auto cp_transform(auto && transformer, auto & str) {
        // Try block entry is widely cheap, and exception will raise (if raise) just once.
        // On the first exception disable rest conversions.
        try {
            transformer.transform(str);
        } catch (std::runtime_error const & e) {
            // "Cannot convert ... from UTF-8 to ACP (active code page)
            transformer.disable();
        }
    }

    void print_span(auto & ostream, auto && span) {
#if defined(TRY_TRANSFORM_UTF8_TO_ACTIVE_CODE_PAGE) && (TRY_TRANSFORM_UTF8_TO_ACTIVE_CODE_PAGE)
        auto str = optional_quote(span);
        static encoding::acp_transformer t;
        // see c++ insights of how to pass span
        encoding::cp_transform(t, str);
        ostream << str;
#endif
        ostream << optional_quote(span);
    }

    void print_line (char delim, auto & ostream, auto const & spans) {
        std::for_each(spans.begin(), spans.end()-1, [&](auto & elem) {
            static_assert(!std::is_same_v<std::decay_t<decltype(elem)>, std::string>);
            print_span(ostream, elem);
            ostream << delim;
        });
        print_span(ostream, spans.back());
        ostream << '\n';
    }

    template <class T>
    struct base_header_printer {
        base_header_printer(char delim, auto & ostream, auto & header, auto const & args) : ostream_(ostream) {
            // For options -H -l there will be difference with python's csvkit (this is very discussion question)
            if (args.no_header and !args.linenumbers)
                static_cast<T&>(*this).no_header_specific_message(delim, header);
            else {
                ostream << (args.linenumbers ? std::string("line_number") + delim : std::string{});
                static_cast<T&>(*this).specific_message(delim, header);
                print_line(delim, ostream, header);
            }
        }
    protected:
        std::ofstream & ostream_;
    };

    // probably code bloat, refactor it
    struct header_printer final : base_header_printer<header_printer> {
        header_printer(char delim, auto & ostream, auto & header, auto const & args)
            : base_header_printer<header_printer>(delim, ostream, header, args) {}
        void specific_message(char, auto&) {}
        void no_header_specific_message(char, auto&) {}
    };

    // probably code bloat, refactor it
    struct erroneous_header_printer final : base_header_printer<erroneous_header_printer> {
        erroneous_header_printer(char delim, auto & ostream, auto & header, auto const & args)
            : base_header_printer<erroneous_header_printer>(delim, ostream, header, args) {
        }
        void specific_message(char delim, auto & header) {
            to_stream(ostream_, "line_number", delim, "msg");
            if (!header[0].empty())
                to_stream(ostream_, delim);
        }
        void no_header_specific_message(char delim, auto & header) {
            erroneous_header_printer::specific_message(delim, header);
            print_line(delim, ostream_, header);
        }
    };

    struct null_deleter {
        void operator()(void * o) const;
    };

    class ofstream_holder final : public std::ofstream {
    public:
        explicit ofstream_holder(std::string const & filename, std::shared_ptr<std::ofstream> & o) : std::ofstream(filename)
        {
            std::shared_ptr<std::ofstream> closer(this, null_deleter());
            o = closer;
        }
        ~ofstream_holder() override {
            assert(!is_open());
        }
    };

    void null_deleter::operator()(void * o) const {
        static_cast<ofstream_holder*>(o)->close();
    }

    template <typename Reader, typename ARGS>
    class specific_maxfieldsize_checker final : protected ::csvkit::cli::max_field_size_checker<Reader, ARGS> {
        void increment_row() override {}
    public:
        using base_class = ::csvkit::cli::max_field_size_checker<Reader, ARGS>;
        using base_class::check;

        specific_maxfieldsize_checker(Reader const & r, ARGS const & args, unsigned columns) 
            : base_class(r, args, columns, init_row{1}) {}

        void reset_current_row(auto row) noexcept {
            base_class::current_row = row;
            base_class::current_column = 0;   
        }
        void move_row() {
            base_class::current_row++;
        }
    };
}

namespace csvclean {
    struct Args : ARGS_positional_1 {
        bool & dry_run = flag("n,dry-run","Do not create output files. Information about what would have been done will be printed to STDERR.");

        void welcome() final {
            std::cout << "\nFix common errors in a CSV file.\n\n";
        }
    };

    void clean(std::monostate &, auto const &) {}

    void clean(auto & reader, auto const & args) {
        using namespace detail;

        auto constexpr delim = std::decay_t<decltype(reader)>::delimiter_type::value;
        auto constexpr quote = std::decay_t<decltype(reader)>::quote_type::value;

        skip_lines(reader, args);

        auto const header = obtain_header_and_<skip_header>(reader, args);

        auto errors = 0ul;

        auto cols = 0u;
        auto good_rows = 0ul;
        auto bad_rows = 0ul;
        auto rows = 0ul;

        std::vector<typename std::decay_t<decltype(reader)>::cell_span> vec;

        auto const columns = header.size();

        specific_maxfieldsize_checker sz_checker(reader, args, columns);
        for (auto const & e : header)
            sz_checker.check(e.operator csv_co::unquoted_cell_string());
        sz_checker.reset_current_row((args.no_header ? 1 : 2));
        
        std::shared_ptr<std::ofstream> err_closer, out_closer;

        reader.run_spans([&](auto s) {
            sz_checker.check(s.operator csv_co::unquoted_cell_string());
            vec.emplace_back(s);
            cols++;
        }, [&] {
            if (!rows && !cols) // process corner cases: 1. One line with no line break. 2. ...
                return;
            rows++;
            sz_checker.move_row();
            if (cols == columns) {
                if (!args.dry_run) {
                    static ofstream_holder out (args.file.stem().string() + "_out.csv", out_closer);
                    static header_printer hp(delim, out, header, args);
                    if (args.linenumbers)
                        to_stream(out, ++good_rows, delim);
                    print_line(delim, out, vec);
                }
            } else {
                errors++;
                if (!args.dry_run) {
                    static ofstream_holder err (args.file.stem().string() + "_err.csv", err_closer);
                    static erroneous_header_printer hp(delim, err, header, args);
                    if (args.linenumbers)
                        to_stream(err, ++bad_rows, delim);
                    to_stream(err, rows, delim, quote, "Expected ", columns, " columns, found ", cols, " columns", quote, delim);
                    print_line(delim, err, vec);
                } else
                    to_stream(std::cerr, "Line ", rows, ": Expected ", columns, " columns, found ", cols, " columns\n");
            }
            vec.clear();
            cols = 0;
        });

        if (!args.dry_run) {
            if (rows == errors) {
                // In this case we lose the header print. Let's make up for it.
                std::ofstream out(args.file.stem().string() + "_out.csv");
                header_printer (delim, out, header, args);
            }

            if (!errors)
                std::cout << "No errors.\n";
            else
                to_stream(std::cout, errors, " error", (errors > 1 ? "s" : ""), " logged to ", args.file.stem().string() + "_err.csv\n");
        }
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)
int main(int argc, char * argv[]) {
    using namespace csvclean;

    auto const args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(read_standard_input, clean)

    return 0;
}
#endif
