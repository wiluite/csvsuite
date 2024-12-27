///
/// \file   utils/csvsuite/include/cli.h
/// \author wiluite
/// \brief  Common small csv kit facilities.

#pragma once

#include "../external/argparse/argparse.hpp"
#include "../external/transwarp/transwarp.h"
#include "reader-bridge-impl.hpp"
#include <filesystem>
#include <functional>
#include <numeric>
#include "encoding.h"

#include <fcntl.h>
#if !defined(_MSC_VER)
  #include <unistd.h>
#else
  #include <io.h>
  #define STDIN_FILENO 0
  #define isatty _isatty
#endif

namespace csvsuite::cli {
    /// Returns zero-based index of a column by name or by order depending on the offset value 
    unsigned match_column_identifier (auto const & column_names, char const * c, auto column_offset);
}

namespace csvsuite::cli::detail{
    /// Strongly typed class template, borrowed from J Boccara
    template <typename T, typename Parameter>
    class NamedType {
    public:
        explicit NamedType(T value) : value_(std::move(value)) {}
        template<typename T_ = T>
        explicit NamedType(T&& value, typename std::enable_if<!std::is_reference<T_>{}, std::nullptr_t>::type = nullptr) : value_(std::move(value)) {}

        T& operator()() { return value_; }
        T const& operator()() const {return value_; }

    private:
        T value_;
    };

    using columns  = NamedType<std::string, struct ColumnsOption>;
    using excludes = NamedType<std::string, struct ExcludesOption>;
    using init_row = NamedType<std::size_t, struct InitRowParam>;

    /// Returns start column depending on the argument value
    auto get_column_offset(auto & args) noexcept {
        return args.zero ? 0 : 1;
    }

    /// Returns a 0,1,..,size-1 range
    auto iota_range(auto const & container)->std::vector<unsigned> {
        std::vector<unsigned> vec(container.size());
        std::iota (vec.begin(), vec.end(), 0);
        return {vec.begin(), vec.end()};
    }

    /// Checks if a C-style string value can be a number
    static bool python_isdigit (char const * value) {
        return std::all_of(&value[0], &value[0] + strlen(value), [](auto e) { return std::isdigit(e); });
    }

    /// Column Identifier Error exception class
    struct ColumnIdentifierError : std::runtime_error {
        template <typename ... Types>
        explicit constexpr ColumnIdentifierError(Types ... args) : std::runtime_error("") {
            store_next(args...);
        }

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return details.c_str();
        }

    private:
        std::string details;
        void store_next() noexcept {}
        template <class First, class ... Rest>
        void store_next(First && first, Rest && ... rest) {
            store_first(std::forward<First>(first));
            store_next(std::forward<Rest>(rest)...);
        }
        template <typename T>
        void store_first(T && v) {
            if constexpr(std::is_arithmetic_v<std::decay_t<T>>)
                details += std::to_string(v);
            else
                details += v;
        }
    };

    /// Throws the Column Identifier Error exception in common cases
    void throw_column_identifier_error(auto const & c_str) {
        throw ColumnIdentifierError("Invalid range ", c_str, ". Ranges must be two integers (by default: 1-based) separated by a - or : character.");
    }

    /// Handles the Column Identifier Error exception in common cases
    void column_identifier_error_handler(char const * c, auto const & column_names, auto & container, int column_offset) {
        std::string a_str, b_str, c_str = c;
        struct [[maybe_unused]] splitter {
            splitter(std::string const & c, char delim, std::string & a, std::string & b) {
                auto const iter = std::find(c.begin(), c.end(),delim);
                a.assign(c.begin(), iter);
                b.assign(iter + 1, c.end());
            }
        };
        if (std::any_of(c_str.cbegin(), c_str.cend(), [](auto & e) { return e == ':'; }))
            splitter {c_str, ':', a_str, b_str};
        else if (std::any_of(c_str.cbegin(), c_str.cend(), [](auto & e) { return e == '-'; }))
            splitter {c_str, '-', a_str, b_str};
        else
            throw;

        try {
            unsigned const a_int = a_str.empty() ? (column_offset ? 1 : 0) : std::stoi(a_str);
            unsigned const b_int = b_str.empty() ? column_names.size() + (column_offset ? 0 : -1) : std::stoi(b_str);
            for (auto x = a_int; x <= b_int; x++) {
                if constexpr (std::is_same_v<std::decay_t<decltype(container)>, std::unordered_set<unsigned>>)
                    container.insert(match_column_identifier(column_names, std::to_string(x).c_str(), column_offset));
                else
                    container.emplace_back(match_column_identifier(column_names, std::to_string(x).c_str(), column_offset));
            }
        } catch (std::invalid_argument const & ) {
            throw_column_identifier_error(c_str);
        } catch (std::out_of_range const &) {
            throw_column_identifier_error(c_str);
        }
    }

} /// namespace

namespace csvsuite::cli {

    using namespace detail;
    constexpr const unsigned max_unsigned_limit = std::numeric_limits<unsigned>::max();
    constexpr const unsigned long max_size_t_limit = std::numeric_limits<unsigned long>::max();
    constexpr const int min_int_limit = std::numeric_limits<int>::min();

    /// Common arguments for all the utilities    
    struct ARGS : argparse::Args {
        unsigned & maxfieldsize = kwarg("z,maxfieldsize","Maximum length of a single field in the input CSV file.").set_default(max_unsigned_limit);
        std::string & encoding = kwarg("e,encoding","Specify the encoding of the input CSV file.").set_default("UTF-8");
        bool & skip_init_space = flag("S,skipinitialspace","Ignore whitespace immediately following the delimiter.");
        bool & no_header = flag("H,no-header-row","Specify that the input CSV file has no header row. Will create default headers (a,b,c,...).");
        unsigned & skip_lines = kwarg("K,skip-lines", "Specify the number of initial lines to skip before the header row (e.g. comments, copyright notices, empty rows).").set_default(0);
        bool & verbose = flag("v,verbose", "A flag to toggle verbose.");
        bool & linenumbers = flag("l,linenumbers", "Insert a column of line numbers at the front of the output. Useful when piping to grep or as a simple primary key.");
        bool & zero = flag ("zero","When interpreting or displaying column numbers, use zero-based numbering instead of the default 1-based numbering.");
        bool &check_integrity = flag("Q,quick-check", "Quickly check the CSV source for matrix shape").set_default(true);
    };

    /// Common arguments for single-file utilities
    struct ARGS_positional_1 : ARGS {
        std::filesystem::path & file = arg("The CSV file to operate on. If omitted, will accept input as piped data via STDIN.").set_default(std::string{""});
    };

    struct ARGS_positional_1_any_format : ARGS {
        std::filesystem::path & file = arg("The file of a specified format to operate on. If omitted, will accept input as piped data via STDIN.").set_default(std::string{""});
    };

    /// Common arguments for multiple-file utilities
    struct ARGS_positional_files : ARGS {
        std::vector<std::string> & files = arg("The CSV files to operate on.").multi_argument().set_default(std::vector<std::string>{});
    };

    /// Quickly checks a CSV source for matrix shape
    void quick_check(auto && r, auto const & args) {
        if (!args.check_integrity)
            return;

        auto cols = 0u;
        auto row = 1ul;
        std::unordered_map<unsigned , unsigned> cols_map;
        r.run_spans([&](auto e) {
            cols++;
        }, [&] {
            if (!cols_map.contains(cols))
                cols_map[cols] = row + args.skip_lines; // write current line to column's quantity not yet present.
            cols = 0;
            row++;
        });

        if (cols_map.size() > 1) {
            if (cols_map.contains(1))
                throw std::runtime_error(std::string("The document has 1 column at ") + std::to_string(cols_map[1]) + " row...");

            std::string error_message = "The document has different numbers of columns :";
            for (auto const & e : cols_map) {
                if (e.first == 1 and e.second == row + args.skip_lines - 1)
                    continue;
                error_message += (" " + std::to_string(e.first));
            }
            error_message += " at least at rows :";
            for (auto e : cols_map) {
                if (e.first == 1 and e.second == row + args.skip_lines - 1)
                    continue;
                error_message += (" " + std::to_string(e.second));
            }
            error_message += "...\nEither use/reuse the -K option for alignment, or use the csvClean utility to fix it.";

            throw std::runtime_error(error_message.c_str());
        }
    }

    /// Throws if there were given mutually exclusive arguments
    void throw_if_names_and_no_header(auto const & args) {
        if (args.names and args.no_header)
            throw std::runtime_error("RequiredHeaderError: You cannot use --no-header-row with the -n or --names options.");
    }

    /// Displays column names and indices with the -n/--names option
    template<class OS, class C, class Args>
    void print_header(OS & os, C &c, Args const &args) {
        auto index = args.zero ? 0 : 1;
        for (auto const &elem: c)
            os << std::setw(3) << index++ << ": " << elem.operator csv_co::unquoted_cell_string() << '\n';
    }

    /// Skips empty lines or copyright
    void skip_lines(auto & reader, auto const & args) {
        if (args.skip_lines)
            reader.skip_rows(args.skip_lines); // if more lines to skip than available - no skip!
    }

    /// Generates substitues for column names, if no header
    auto generate_column_names(auto & reader);

    /// Returns header cell range. Throws an exception if no more
    auto obtain_header(auto & reader, auto const & args) {
        auto header = (!args.no_header) ? reader.header() : generate_column_names(reader);
        if (header.empty())
            throw std::runtime_error("Header is empty: no data anymore.");
        return header;
    }

    constexpr bool skip_header = true;
    constexpr bool no_skip_header = false;

    /// Optionally skips the header to the first data line
    template<bool should_skip_header = skip_header>
    auto obtain_header_and_(auto & reader, auto const & args) {
        auto header = obtain_header(reader, args);
        if (!args.no_header && should_skip_header)
            reader.skip_rows(1);
        return header;
    }

    /// Transforms the header cell range to the range of strings
    template <bool Quoted_or_Unquoted>
    inline auto header_to_strings(auto const & cell_header) {
        std::vector<std::string> v;
        v.reserve(cell_header.size());
        for (auto const & e : cell_header) {
            if constexpr(Quoted_or_Unquoted == csv_co::quoted)
                v.emplace_back(e.operator csv_co::cell_string());
            else
                v.emplace_back(e.operator csv_co::unquoted_cell_string());
        }
        return v;
    }

    /// Returns the standard input stream as a standard string
    auto read_standard_input(auto &)->std::string {
        std::string result;
        for (std::string input;;) {
            std::getline(std::cin, input);
            result += input;
            if (std::cin.eof())
                break;
            result += '\n';
        }
        return result;
    }

    /// The output precision representation class for the csvStat's standard/csv print visitors (except for 'most common values')    
    template <typename T>
    class spec_precision
    {
        const T& value;
        explicit spec_precision(const T& value) : value(value) {}

        template <typename T2>
        friend spec_precision<T2> spec_prec(const T2& value);

    public:
        void write(std::ostream& stream) const {
            std::ios_base::fmtflags flags = stream.flags();
            if (flags & std::ios::fixed) {
                stream.unsetf(std::ios_base::floatfield);
                auto const c_prec = stream.precision();
                std::streamsize restore = stream.precision((value == 0 ? 0 : std::ceil(std::log10(std::abs(value)))) + c_prec);
                stream << value;
                if (std::trunc(value) == value)
                    stream << std::use_facet<std::numpunct<char>>(stream.getloc()).decimal_point();
                stream.precision(restore);
                stream.flags(flags);
            } else
                stream << value;
        }
    };

    /// The output precision representation function that uses the spec_precision<T> template
    template <typename T>
    inline spec_precision<T> spec_prec(const T& value) {
        return spec_precision<T>(value);
    }

    /// Stream output operator working with the spec_precision<T> template
    template <typename T>
    inline std::ostream& operator << (std::ostream& stream, const spec_precision<T>& value) {
        value.write(stream);
        return stream;
    }

    /// The output precision representation class for most common CSV values
    template <typename T>
    class csv_mcv_precision
    {
        const T& value;
        explicit csv_mcv_precision(const T& value) : value(value) {}

        template <typename T2>
        friend csv_mcv_precision<T2> csv_mcv_prec(const T2& value);

    public:
        void write(std::ostream& stream) const {
            std::ios_base::fmtflags flags = stream.flags();
            if (flags & std::ios::fixed) {
                stream.unsetf(std::ios_base::floatfield);
                auto const c_prec = stream.precision();
                std::streamsize restore = stream.precision((!value ? 0 : std::ceil(std::log10(std::abs(value)))) + c_prec);
                stream << value;
                stream.precision(restore);
                stream.flags(flags);
            } else
                stream << value;
        }
    };

    /// The output precision representation function that uses the csv_mcv_precision<T> template
    template <typename T>
    inline csv_mcv_precision<T> csv_mcv_prec(const T& value) {
        return csv_mcv_precision<T>(value);
    }

    /// Stream output operator working with the csv_mcv_precision<T> template
    template <typename T>
    inline std::ostream& operator << (std::ostream& stream, const csv_mcv_precision<T>& value) {
        value.write(stream);
        return stream;
    }

    /// To be documented
    template<class... Ts>
    std::string to_string(Ts&&... ts) {
        std::ostringstream oss;
        (oss << ... << std::forward<Ts>(ts));
        return oss.str();
    }

    /// To be documented
    template<class SS, class... Ts>
    void to_stream(SS & ss, Ts&&... ts) {
        (ss << ... << std::forward<Ts>(ts));
    }

    /// Custom boolean and grouping separator facet class
    struct custom_boolean_and_grouping_sep_facet : std::numpunct<char> {
        custom_boolean_and_grouping_sep_facet() {
            s = std::use_facet<std::numpunct<char>>(std::locale()).grouping();
            t = std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep();
            d = std::use_facet<std::numpunct<char>>(std::locale()).decimal_point();
        }
        [[nodiscard]] std::string do_truename() const override {
            return "True";
        }
        [[nodiscard]] std::string do_falsename() const override {
            return "False";
        }
        [[nodiscard]] char do_thousands_sep() const override { return t; }                         
        [[nodiscard]] char do_decimal_point() const override { return d; }
        [[nodiscard]] std::string do_grouping() const override { return s; }

    private:
        std::string s;
        char t;
        char d;
    };

    /// Custom boolean and no-grouping separator facet class
    struct custom_boolean_and_no_grouping_sep_facet : custom_boolean_and_grouping_sep_facet {
        [[nodiscard]] std::string do_grouping() const override { return ""; }
    };

    /// Custom boolean facet class
    struct custom_boolean_facet : std::numpunct<char> {
        [[nodiscard]] std::string do_truename() const override {
            return "True";
        }
        [[nodiscard]] std::string do_falsename() const override {
            return "False";
        }
    };

    /// Sets up default or replaces the C++ global locale with specific one
    static void setup_global_locale(std::string_view default_global_locale) {
        std::locale::global(std::locale(std::string(default_global_locale)));
    }

    /// Detects that not only C/Posix locale supported
    static bool detect_locale_support() noexcept {
        auto curr = std::locale();
        try {
            std::locale::global(std::locale("en_US"));
        } catch (...) {
            // "Wide-spread locales are not supported!";
            return false;
        }
        std::locale::global(curr);
        return true;
    }

    /// Tunes an output stream with a specified decimal format, like %.3f, %.4e, %.15g
    static void tune_format(std::ostream & os, char const * fmt) {
        int prec;
        char float_type;
#if defined (_MSC)
        int res = sscanf_s(fmt, "%%.%d%c", &prec, &float_type, 1);
#else
        int res = sscanf(fmt, "%%.%d%c", &prec, &float_type);
#endif
        float_type = static_cast<char>(std::toupper(float_type));
        if (res != 2 or (float_type != 'F' and float_type != 'G' and float_type != 'E'))
            throw std::runtime_error(std::string("Wrong format: ") + fmt + ". Format examples: %.3f, %.4e, %.15g, etc.");
        os.setf((float_type == 'F' ? std::ios::fixed : (float_type == 'E' ? std::ios::scientific : static_cast<std::ios_base::fmtflags>(0))), std::ios::floatfield);
        os.precision(prec);
    }

    /// Tunes an output stream for the global locale with a specified decimal format
    template <typename ... facets>
    void tune_ostream(auto & os, char const * fmt = "%.3f") {
        // global locale is supposed to be already set
        os.imbue(std::locale(std::locale(), (new facets)...));
        tune_format(os, fmt);
    }

    /// Tunes an output stream with special facets, depending on arguments, and with those things mentioned
    void tune_ostream(auto & os, auto const & args) {
        if (!args.no_grouping_sep)
            tune_ostream<custom_boolean_and_grouping_sep_facet>(os, args.decimal_format.c_str());
        else
            tune_ostream<custom_boolean_and_no_grouping_sep_facet>(os, args.decimal_format.c_str());
    }

    namespace trim_policy {
        /// Trimming policy class used for cr-trailing strings   
        template<char const *list>
        struct trimming_trailing_cr {
        public:
            inline static void trim(std::string &s) {
                s.erase(s.find_last_not_of(list) + 1);
            }
            inline static auto ret_trim(std::string s) {
                s.erase(s.find_last_not_of(list) + 1);
                return s;
            }
            inline static csv_co::unquoted_cell_string ret_trim(csv_co::unquoted_cell_string s) {
                s.erase(s.find_last_not_of(list) + 1);
                return s;
            }
        };
        static char constexpr crchar[] {'\r', '\0'};
        using crtrim = trimming_trailing_cr<crchar>;
    }

    namespace trim_policy {
        /// Trimming policy class used for the -S option
        template<char const *list, char const *list2>
        struct trimming_init_space {
        public:
            inline static void trim(std::string &s) {
                s.erase(0, s.find_first_not_of(list));
                s.erase(s.find_last_not_of(list2) + 1);
            }
            inline static auto ret_trim(std::string s) {
                s.erase(0, s.find_first_not_of(list));
                s.erase(s.find_last_not_of(list2) + 1);
                return s;
            }
            inline static csv_co::unquoted_cell_string ret_trim(csv_co::unquoted_cell_string s) {
                s.erase(0, s.find_first_not_of(list));
                s.erase(s.find_last_not_of(list2) + 1);
                return s;
            }
        };

        static char constexpr init_space_chars[] = " \t\r";
        using init_space_trim = trimming_init_space<init_space_chars, crchar>;
    }

    /// Alias of the reader type used for the most CSV readers instantiations
    using notrimming_reader_type = csv_co::reader<csvsuite::cli::trim_policy::crtrim>;
    /// Alias of the reader type used for the skipping initial space CSV readers instantiations
    using skipinitspace_reader_type = csv_co::reader<csvsuite::cli::trim_policy::init_space_trim>;

    /// Recodes a CSV source text endcoding to the UTF-8 one, if required
    void recode_source(auto & reader, auto const & args) {
        if (args.encoding == "UTF-8")  {
            auto const check_result = encoding::is_source_utf8(reader);
            if (check_result.error) {
                std::ostringstream oss;
                to_stream(oss, "Your file is not \"UTF-8\" encoded. Please specify the correct encoding with the -e flag.\n"
                    , "Decode error: simdutf can't decode byte 0x", std::hex
                    , static_cast<int>(static_cast<unsigned char>(*(reader.data() + check_result.count))), " in position "
                    , std::dec, check_result.count, ".\n");
               throw std::runtime_error(oss.str()) ;
            }
        } else {
            using reader_type_ = std::decay_t<decltype(reader)>;
            reader = reader_type_(encoding::recode_source_from(reader, args.encoding));
        }
    }

    /// Overridden function. Recodes a CSV source text endcoding to the UTF-8 one, if required
    std::string recode_source(std::string && s, auto const & args) {
        if (args.encoding == "UTF-8")  {
            auto const check_result = encoding::is_source_utf8(s);
            if (check_result.error) {
                std::ostringstream oss;
                to_stream(oss, "Your file is not \"UTF-8\" encoded. Please specify the correct encoding with the -e flag.\n");
                throw std::runtime_error(oss.str()) ;
            }
            return s;
        } else
            return encoding::recode_source_from(s, args.encoding);
    }

#define runner_impl(reader_type_, reader_arg_, function_)                                            \
    reader_type_ r(reader_arg_);                                                                     \
    recode_source(r, args);                                                                          \
    function_(r, args);

#define basic_reader_configurator_and_runner(std_cin_reader, function)                               \
    try {                                                                                            \
        if (args.file.empty() and isatty(STDIN_FILENO))                                              \
            throw std::runtime_error("Error: You must provide an input file or piped data.");        \
        if (!args.file.empty() and !isatty(STDIN_FILENO))                                            \
            throw std::runtime_error("Error: You have both: the input file and piped data.");        \
        if (args.file.empty())                                                                       \
            args.file = "_";                                                                         \
                                                                                                     \
        if (!args.skip_init_space) {                                                                 \
            if (!(args.file == "_")) {                                                               \
                runner_impl(notrimming_reader_type, std::filesystem::path{args.file}, function)      \
            } else {                                                                                 \
                runner_impl(notrimming_reader_type, std_cin_reader(args), function)                  \
            }                                                                                        \
        } else {                                                                                     \
            if (!(args.file == "_")) {                                                               \
                runner_impl(skipinitspace_reader_type, std::filesystem::path{args.file}, function)   \
            } else {                                                                                 \
                runner_impl(skipinitspace_reader_type, std_cin_reader(args), function)               \
            }                                                                                        \
        }                                                                                            \
    }                                                                                                \
    catch (std::exception const & e) {                                                               \
        std::cout << e.what() << std::endl;                                                          \
    }                                                                                                \
    catch (...) {                                                                                    \
        std::cout << "Unknown exception.\n";                                                         \
    }

    /// Consistently maps a column number to a letter
    static std::string letter_name(std::size_t column) {
        char const * const letters = "abcdefghijklmnopqrstuvwxyz";
        unsigned const count = std::strlen(letters);
        std::string s;
        while (column >= count) {
            s += letters[column % count];
            column -= count;
        }
        s += letters[column];
        return s ;
    }

    /// Having a reader object, generates a range of column names
    auto generate_column_names(auto & reader) {
        using cell_span_type = typename std::decay_t<decltype(reader)>::cell_span;
        auto const col_num = reader.cols();

        std::vector<cell_span_type> column_cells;
        column_cells.reserve(col_num);

        static std::vector<std::string> letter_names (col_num);
        letter_names.resize(col_num);

        unsigned i = 0;
        std::generate(letter_names.begin(), letter_names.end(), [&i] { 
            return letter_name(i++); 
        });
        for (auto const & e : letter_names) 
            column_cells.emplace_back(cell_span_type{e});
        return column_cells;
    }

    /// Checker of the maximum CSV field/cell length
    template <typename Reader, typename ARGS>
    class max_field_size_checker {
        template <typename T>
        void check_tmpl(T const & o) {
            if constexpr(std::is_same_v<T, std::string>)
                throw_if_exceeds(csv_co::csvsuite::str_symbols(o));
            else
                throw_if_exceeds(o.str_size_in_symbols());
        }

    public:
        max_field_size_checker(Reader const &, ARGS const & args, unsigned columns, init_row ir) 
            : args_(args), columns_(columns), current_row(ir()) {
            if (args_.maxfieldsize != max_unsigned_limit) {
                typed_span_check_impl = [&] (typename Reader::template typed_span<csv_co::unquoted> const & span) {
                    static_assert(std::is_const_v<std::remove_reference_t<decltype(span)>>);
                    check_tmpl(span);
                };

                string_check_impl = [&] (std::string const & s) {
                    check_tmpl(s);
                };

                row_span_check_impl = [&] (typename Reader::row_span & collection) {
                    auto v = header_to_strings<csv_co::unquoted>(collection);
                    for(auto & e: v)
                        check_tmpl(e);
                };

                vec_span_check_impl = [&] (std::vector<typename Reader::cell_span> const & collection) {
                    auto v = header_to_strings<csv_co::unquoted>(collection);
                    for(auto & e: v)
                        check_tmpl(e);
                };

                // if no numeric locale imbued, fall from typed_cell to cell_span and use unquoted string as string to count.
                try {
                    Reader::template typed_span<csv_co::unquoted>::num_locale();
                } catch(...) {
                    typed_span_check_impl = [&] (typename Reader::cell_span const & span) {
                        check_tmpl(std::string(static_cast<csv_co::unquoted_cell_string>(span)));
                    };
                }
            } else {
                typed_span_check_impl = [] (typename Reader::template typed_span<csv_co::unquoted> const &) {};
                string_check_impl = [] (std::string const & s) {};
                row_span_check_impl = [] (typename Reader::row_span &) {};
                vec_span_check_impl = [] (std::vector<typename Reader::cell_span> const &) {};
            }
        }

        // works for numeric-locale-imbued cells only.
        inline void check(typename Reader::cell_span const & cell_span) {
            typed_span_check_impl(typename Reader::template typed_span<csv_co::unquoted>{cell_span});
        }
        // works for any strings from cells that may be numeric-locale-free.
        void check(std::string const & s) {
            string_check_impl(s);
        }
        // works for any strings from cells that may be numeric-locale-free.
        void check(typename Reader::row_span & row_span) {
            row_span_check_impl(row_span);
        }
        // works for any strings from cells that may be numeric-locale-free.
        void check(std::vector<typename Reader::cell_span> const & vec) {
            vec_span_check_impl(vec);
        }

    private:
        void throw_if_exceeds(std::size_t symbols) {
            if (symbols > args_.maxfieldsize) {
                std::ostringstream oss;
                to_stream(oss, "FieldSizeLimitError: CSV contains a field longer than the maximum length of "
                    , args_.maxfieldsize, " characters on line ", current_row, ".");
                throw typename Reader::exception(oss.str());
            }
            increment_row();
        }

        std::function<void (typename Reader::template typed_span<csv_co::unquoted> const &)> typed_span_check_impl;
        std::function<void (std::string const & )> string_check_impl;
        std::function<void (typename Reader::row_span &)> row_span_check_impl;
        std::function<void (std::vector<typename Reader::cell_span> const &)> vec_span_check_impl;
        ARGS const & args_;
        unsigned columns_;

    protected:
        unsigned current_column = 0; // let it be 0-based
        std::size_t current_row = 1; // well, let it be 1-based!

        virtual void increment_row() {
            if (++current_column == columns_) {
                current_column = 0;
                current_row++;
            }
        }

    };

    constexpr bool establish_checker = false;
    constexpr bool establish_new_checker = true;

    /// Function that checks maximum field size not exceeding the given.
    template <bool Reestablish = establish_checker, typename Reader, typename Args>
    inline void check_max_size(Reader & reader, Args const & args, typename Reader::template typed_span<csv_co::unquoted> & collection, init_row ir) {
        static max_field_size_checker checker(reader, args, collection.size(), ir);
        for (auto & e: collection)
            checker.check(e);
    }
    /// Override. Function that checks maximum field size
    template <bool Reestablish = establish_checker, typename Reader, typename Args>
    inline void check_max_size(Reader & reader, Args const & args, typename Reader::template typed_span<csv_co::quoted> & collection, init_row ir) {
        static max_field_size_checker checker(reader, args, collection.size(), ir);
        for (auto & e: collection)
            checker.check(e);
    }
    /// Override. Function that checks maximum field size
    template <bool Reestablish = establish_checker, typename Reader, typename Args>
    inline void check_max_size(Reader & reader, Args const & args, typename Reader::row_span & rspan, init_row ir) {
        static max_field_size_checker checker(reader, args, rspan.size(), ir);
        checker.check(rspan);
    }
    /// Override. Function that checks maximum field size
    template <bool Reestablish = establish_checker, typename Reader, typename Args>
    inline void check_max_size(Reader & reader, Args const & args, std::vector<typename Reader::cell_span> const & vec, init_row ir) {
        static max_field_size_checker checker(reader, args, vec.size(), ir);
        checker.check(vec);
    }
    /// Override. Function that checks maximum field size
    template<typename Collection, typename Reader, typename Args>
    void check_max_size(Collection const & collection, max_field_size_checker<Reader, Args> & checker) {
        for (auto & e: collection)
            checker.check(e);
    }

    /// A primitive helper matrix class to wrap vector of vector of elements
    template <typename ElemType>
    struct fixed_array_2d_replacement {
    private:
        using field_array = std::vector<ElemType>;
    public:
        using table = std::vector<field_array>;
    protected:
    public:
        table table_impl;
    public:
        explicit operator table& () {
            return table_impl;
        }
        decltype(auto) operator[](size_t r) {
            return table_impl[r];
        }
        [[nodiscard]] auto begin() const {
            return table_impl.cbegin();
        }
        [[nodiscard]] auto end() const {
            return table_impl.cend();
        }
        auto begin() {
            auto & impl_ref = table_impl;
            return impl_ref.begin();
        }
        auto end() {
            auto & impl_ref = table_impl;
            return impl_ref.end();
        }

        fixed_array_2d_replacement(auto rows, auto cols) : table_impl(rows, field_array(cols)) {}
        [[nodiscard]] auto rows() const {
            return table_impl.size();
        }
        [[nodiscard]] auto cols() const {
            return table_impl[0].size();
        }
        fixed_array_2d_replacement(fixed_array_2d_replacement&&) noexcept = default;
        fixed_array_2d_replacement& operator=(fixed_array_2d_replacement&&) noexcept = default;
    };

    static_assert(!std::is_copy_constructible_v<fixed_array_2d_replacement<std::string>>);
    static_assert(!std::is_copy_assignable_v<fixed_array_2d_replacement<std::string>>);
    static_assert(std::is_move_constructible<fixed_array_2d_replacement<typename notrimming_reader_type::template typed_span<csv_co::quoted>>>::value);
    static_assert(std::is_move_assignable<fixed_array_2d_replacement<typename notrimming_reader_type::template typed_span<csv_co::quoted>>>::value);

    constexpr const char * bad_locale_message = "Your development tool, operation system, standard library, or combination thereof does not allow this particular locale to be used within this utility.";
    static std::locale & cell_numeric_locale(char const * const name) {
        try {
            static std::map<std::string, std::locale> localemap;
            if (auto search = localemap.find(name); search != localemap.end())
                return search->second;
            else {
                localemap[name] = std::locale(name);
                return localemap[name];
            }
        } catch (...) {
            std::cerr << "Locale: " << std::quoted(name) << '\n';
            throw std::runtime_error(bad_locale_message);
        }
    }

    /// Binds a apecified numeric locale to all kinds of cells
    void imbue_numeric_locale(auto & reader, auto const & args) {
        using reader_type = std::decay_t<decltype(reader)>;

        using unquoted_elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
        unquoted_elem_type::imbue_num_locale(cell_numeric_locale(args.num_locale.c_str()));

        using quoted_elem_type = typename reader_type::template typed_span<csv_co::quoted>;
        quoted_elem_type::imbue_num_locale(cell_numeric_locale(args.num_locale.c_str()));
    }

    /// Binds a apecified date and datetime parser to all kinds of cells
    void setup_date_parser_backend(auto & reader, auto const & args) {
        using reader_type = std::decay_t<decltype(reader)>;

        using unquoted_elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
        unquoted_elem_type::setup_date_parser_backend(!args.date_lib_parser ? 
            unquoted_elem_type::date_parser_backend_t::compiler_supported : unquoted_elem_type::date_parser_backend_t::date_lib_supported);

        using quoted_elem_type = typename reader_type::template typed_span<csv_co::quoted>;
        quoted_elem_type::setup_date_parser_backend(!args.date_lib_parser ? 
            quoted_elem_type::date_parser_backend_t::compiler_supported : quoted_elem_type::date_parser_backend_t::date_lib_supported);
    }

    /// Specifies whether to take into account the leading zeros for numerics
    void setup_leading_zeroes_processing(auto & reader, auto const & args) {
        using reader_type = std::decay_t<decltype(reader)>;

        using unquoted_elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
        unquoted_elem_type::no_leading_zeroes(args.no_leading_zeroes);

        using quoted_elem_type = typename reader_type::template typed_span<csv_co::quoted>;
        quoted_elem_type::no_leading_zeroes(args.no_leading_zeroes);
    }

    /// Creates a set of user-defined null values
    static void update_null_values(std::vector<std::string> & null_values) {
        csv_co::get_null_value_set().clear();
        for (auto & e: null_values) {
            std::transform(e.begin(), e.end(), e.begin(), ::toupper);
            csv_co::get_null_value_set().insert(e);
        }
    }

    /// The kinds of cell typifying process
    enum class typify_option {
        typify_with_precisions,
        typify_without_precisions,
        typify_without_precisions_and_blanks
    };

    enum struct column_type {
        unknown_t,
        bool_t,
        number_t,
        datetime_t,
        date_t,
        timedelta_t,
        text_t,
        sz
    };

    using typify_with_precisions_result = std::tuple<std::vector<column_type>, std::vector<unsigned char>, std::vector<unsigned>>;
    using typify_without_precisions_result = std::tuple<std::vector<column_type>, std::vector<unsigned char>>;
    using typify_without_precisions_and_blanks_result = std::tuple<std::vector<column_type>>;
    using typify_result = std::variant<typify_with_precisions_result, typify_without_precisions_result, typify_without_precisions_and_blanks_result>;

    /// Detects types, blanks and precisions for every column
    template <typename Reader, typename Args>
    auto typify(Reader & reader, Args const & args, typify_option option) -> typify_result {

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

        if (!reader.cols())
            throw std::runtime_error("Typify(). Columns == 0. Vain to do next actions!"); // well, vain to do rest things

        {
            max_field_size_checker size_checker(reader, args, header.size(), init_row{1});
            check_max_size(header, size_checker);
        }

        fixed_array_2d_replacement<typename Reader::template typed_span<csv_co::unquoted>> table(header.size(), reader.rows());

        auto c_row{0u};
        auto c_col{0u};

        max_field_size_checker size_checker(reader, args, header.size(), init_row{args.no_header ? 1u : 2u});

        reader.run_rows([&] (auto & rowspan) {
            static struct tabular_checker {
                using cell_span_t= typename Reader::cell_span;
                tabular_checker (std::vector<cell_span_t> const & header, typename Reader::row_span const & row_span) {
                    if (header.size() != row_span.size())
                        throw typename Reader::exception("The number of header and data columns do not match. Use -K option to align.");
                }
            } checker (header, rowspan);

            check_max_size(rowspan, size_checker);

            for (auto & elem : rowspan)
                table[c_col++][c_row] = elem;

            c_row++;
            c_col = 0;
        });

        std::vector<column_type> types (table.rows(), column_type::unknown_t);

        std::vector<std::size_t> column_numbers (types.size());
        std::iota(column_numbers.begin(), column_numbers.end(), 0);
    
        transwarp::parallel exec(std::thread::hardware_concurrency());

        std::vector<unsigned char> blanks (types.size(), 0);
        std::vector<unsigned> precisions (types.size(), 0);

        imbue_numeric_locale(reader, args);
        [&option] {
            using unquoted_elem_type = typename Reader::template typed_span<csv_co::unquoted>;
            unquoted_elem_type::no_maxprecision(option != typify_option::typify_with_precisions);

            using quoted_elem_type = typename Reader::template typed_span<csv_co::quoted>;
            unquoted_elem_type::no_maxprecision(option != typify_option::typify_with_precisions);
        }();

        setup_date_parser_backend(reader, args);
        setup_leading_zeroes_processing(reader, args);

        //TODO: for now e.is_null() calling first is obligate. Can we do better?

        std::function<void(unsigned, bool)> setup_blanks;

        if (option != typify_option::typify_without_precisions_and_blanks)
            setup_blanks = [&](unsigned c, bool n) {
                if (!blanks[c] && n)
                    blanks[c] = 1;
            };
        else
            setup_blanks = std::function<void(unsigned, bool)>();

        auto task = transwarp::for_each(exec, column_numbers.cbegin(), column_numbers.cend(), [&](auto c) {
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &c, &setup_blanks](auto & e)  {
                auto const n = e.is_null_or_null_value() && !args.blanks;
                setup_blanks(c, n);
                return n || (!args.no_inference && e.is_boolean());
            })) {
                types[c] = column_type::bool_t;
                return;
            }
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &c, &setup_blanks](auto &e) {
                auto const n = e.is_null_or_null_value() && !args.blanks;
                setup_blanks(c, n);
                return n || (!args.no_inference && std::get<0>(e.timedelta_tuple()));
            })) {
                types[c] = column_type::timedelta_t;
                return;
            }
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &c, &setup_blanks](auto & e) {
                auto const n = e.is_null_or_null_value() && !args.blanks;
                setup_blanks(c, n);
                return n || (!args.no_inference && std::get<0>(e.datetime(args.datetime_fmt)));
            })) {
                types[c] = column_type::datetime_t;
                return;
            }
            if (std::all_of(table[c].begin(), table[c].end(), [&args, &c, &setup_blanks](auto &e) {
                auto const n = e.is_null_or_null_value() && !args.blanks;
                setup_blanks(c, n);
                return n || (!args.no_inference && std::get<0>(e.date(args.date_fmt)));
            })) {
                types[c] = column_type::date_t;
                return;
            }
            if (option == typify_option::typify_with_precisions) {
                if (std::all_of(table[c].begin(), table[c].end(), [&args, &c, &precisions, &setup_blanks](auto & e) {
                    auto const n = e.is_null_or_null_value() && !args.blanks;
                    setup_blanks(c, n);
                    auto const result = n || (!args.no_inference && e.is_num());
                    if (result and !n) {
                        if (precisions[c] < e.precision())
                            precisions[c] = e.precision();
                    }
                    return result;
                })) {
                    types[c] = column_type::number_t;
                    return;
                }
            } else {
                if (std::all_of(table[c].begin(), table[c].end(), [&args, &c, &setup_blanks](auto & e) {
                    auto const n = e.is_null_or_null_value() && !args.blanks;
                    setup_blanks(c, n);
                    return n || (!args.no_inference && e.is_num());
                })) {
                    types[c] = column_type::number_t;
                    return;
                }
            }
            // Text type: check ALL rows for an absent.
            if (std::all_of(table[c].begin(), table[c].end(), [&](auto &e) {
                auto const n = e.is_null_or_null_value() && !args.blanks;
                setup_blanks(c, n);
                return true;
            })) {
                types[c] = column_type::text_t;
                return;
            }
        });

        task->wait();

        for (auto & elem : types) {
            assert(elem != column_type::unknown_t);
            if (args.no_inference and elem != column_type::text_t) {
                assert(elem == column_type::bool_t);  // all nulls in a column otherwise boolean
                elem = column_type::text_t;           // force setting it to text
            }
        }
        if (option == typify_option::typify_with_precisions)
            return std::tuple{types, blanks, precisions};
        else
        if (option == typify_option::typify_without_precisions)
            return std::tuple{types, blanks};
        else
            return std::tuple{types};
    }

    /// Just simple and effective trimming a string
    inline auto trim_string = [](std::string & s) {
        s.erase(0, s.find_first_not_of(' '));
        s.erase(s.find_last_not_of(' ') + 1);
    };

    /// Parses and checks column indentifiers given by user
    auto parse_column_identifiers(auto && ids, auto && column_names, auto && column_offset, auto && excl)->std::vector<unsigned> {
        if (column_names.empty())
            return {};
        if (ids().empty() && excl().empty())
            return iota_range(column_names);
        std::vector<unsigned> columns;

        if (!ids().empty()) {
            for (auto c = strtok(ids().data(),","); c != nullptr; c = strtok(nullptr, ",")) {
                std::string column {c};
                trim_string(column);
                try {
                    columns.emplace_back(match_column_identifier(column_names, column.c_str(), column_offset));
                }
                catch (ColumnIdentifierError const &) {
                    column_identifier_error_handler(column.c_str(), column_names, columns, column_offset);
                }
            }
        } else
            columns = iota_range(column_names);

        std::unordered_set<unsigned> not_columns;
        if (!excl().empty()) {
            for (auto c = strtok(excl().data(),","); c != nullptr; c = strtok(nullptr, ",")) {
                std::string column {c};
                trim_string(column);
                try {
                    not_columns.insert(match_column_identifier(column_names, column.c_str(), column_offset));
                } catch (ColumnIdentifierError const &) {
                    column_identifier_error_handler(column.c_str(), column_names, not_columns, column_offset);
                }
            }
        }

        std::vector<unsigned> result;
        std::for_each(columns.begin(), columns.end(), [&](auto & elem) {
            if (!not_columns.contains(elem))
                result.emplace_back(elem);
        });

        return result;
    }

    /// To be documented
    unsigned match_column_identifier (auto const & column_names, char const * c, auto column_offset) {
        auto const digital = python_isdigit(c);
        if (!digital) {
            auto const iter = std::find(column_names.begin(), column_names.end(), c);
            if (iter != column_names.end())
                return std::distance(column_names.begin(), iter);
        }
        if (digital) {
            int const col = std::stoi(c) - column_offset;
            if (col < 0)
                throw ColumnIdentifierError("Column '", col + column_offset, "' is invalid. Columns are (default) 1-based.");
            if (static_cast<unsigned>(col) >= column_names.size())
                throw ColumnIdentifierError("Column ", col + column_offset, " is invalid. The last column is '", column_names[column_names.size()-1], "' at index ", column_names.size() - 1 + column_offset, ".");
            return col;
        } else {
            auto column_names_string = [](auto const & container) {
                std::string s;
                std::for_each(container.begin(), container.end() - 1, [&s](auto & elem) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(elem)>, std::string>)
                        s += std::string("'" + elem + "',");
                    else
                        s += std::string("'" + elem.operator csv_co::cell_string() + "',");
                });
                if constexpr (std::is_same_v<std::decay_t<decltype(container.back())>, std::string>)
                    s += std::string("'" + container.back() + "'");
                else
                    s += std::string("'" + container.back().operator csv_co::cell_string() +"'");
                return s;
            };

            throw ColumnIdentifierError("Column '", c, "' is invalid. It is neither an integer nor a column name. Column names are: ", column_names_string(column_names), ".");
        }
    }
    /// Gets datetime time point from a cell span
    auto datetime_time_point(auto const & elem) {
        return std::get<1>(elem.datetime());  
    }
    /// Gets datetime string from a time point (sys_seconds)
    static auto datetime_s(date::sys_seconds const & tp) {
        std::stringstream ss;
        date::to_stream(ss, "%Y-%m-%d %H:%M:%S", tp); 
        return ss.str(); 
    }
    /// Gets datetime string from a time point (sys_seconds), alternative representation
    static auto datetime_s_json(date::sys_seconds const & tp) {
        std::stringstream ss;
        date::to_stream(ss, "%Y-%m-%dT%H:%M:%S", tp);
        return ss.str();
    }
    /// Gets datetime string from a cell span
    static auto datetime_s(auto const & elem) {
        auto const tp = std::get<1>(elem.datetime());
        static_assert(std::is_same_v<decltype(tp), date::sys_seconds const>); 
        return datetime_s(tp);
    }
    /// Gets datetime string from a cell span, alternative representation
    static auto datetime_s_json(auto const & elem) {
        auto const tp = std::get<1>(elem.datetime());
        static_assert(std::is_same_v<decltype(tp), date::sys_seconds const>);
        return datetime_s_json(tp);
    }
    /// Gets date time point from a cell span
    auto date_time_point(auto const & elem) {
        return std::get<1>(elem.date());  
    }
    /// Gets date string from a time point (sys_seconds)
    static auto date_s(date::sys_seconds const & tp) {
        std::stringstream ss;
        date::to_stream(ss, "%Y-%m-%d", tp); 
        return ss.str(); 
    }
    /// Gets date string from a cell span
    static auto date_s(auto const & elem) {
        auto const tp = std::get<1>(elem.date());
        static_assert(std::is_same_v<decltype(tp), date::sys_seconds const>);
        return date_s(tp); 
    }

    template <typename T>
    class bool_stringstream : public std::stringstream {
    public:
        bool_stringstream() {
            if constexpr(std::is_same_v<T,void>) {
                static std::locale loc(""); 
                this->imbue(std::locale(loc, new custom_boolean_and_no_grouping_sep_facet));
            }        
        }
    };

    template <typename T, typename Q=void>
    inline std::string compose_bool_DRY (T const & elem) {
        static bool_stringstream<Q> ss;
        ss.rdbuf()->str("");
        ss << std::boolalpha << (elem.is_boolean(), static_cast<bool>(elem.unsafe()));
        return ss.str();
    }

    template <typename T, typename Q=void>
    std::string compose_bool(T const & elem, std::any const &) {
        return compose_bool_DRY<T, Q>(elem);
    }

    template <typename T, typename Q=void>
    std::string compose_bool_1_arg(T const & elem) {
        return compose_bool_DRY<T, Q>(elem);
    }

    struct in2csv_conversion_datetime;

    template <typename T, typename Q=void>             
    inline std::string compose_datetime_DRY(T const & elem) {
        if constexpr(std::is_same_v<Q,void>)
            return datetime_s(elem);
        else
        if constexpr(std::is_same_v<Q,in2csv_conversion_datetime>)
            return datetime_s_json(elem);
        else
            return std::string("\"") + datetime_s_json(elem) + '"';
    }

    template <typename T, typename Q=void>
    std::string compose_datetime(T const & elem, std::any const &) {
        return compose_datetime_DRY<T, Q>(elem);
    }

    template <typename T, typename Q=void>
    std::string compose_datetime_1_arg(T const & elem) {
        return compose_datetime_DRY<T, Q>(elem);
    }

    template <typename T, typename Q=void>
    inline std::string compose_date_DRY(T const & elem) {
        if constexpr(std::is_same_v<Q,void>)
            return date_s(elem);
        else
            return std::string("\"") + date_s(elem) + '"';
    }

    template <typename T, typename Q=void>
    std::string compose_date(T const & elem, std::any const &) {
        return compose_date_DRY<T, Q>(elem);
    }


    template <typename T, typename Q=void>
    std::string compose_date_1_arg(T const & elem) {
        return compose_date_DRY<T, Q>(elem);
    }

    template <typename T, typename Q=void>
    inline std::string compose_timedelta_DRY(T const & elem) {
        auto const timedelta = std::get<1>(elem.timedelta_tuple());
        return timedelta.find(',') != std::string::npos ? R"(")" + timedelta + '"' : timedelta;
    }

    template <typename T, typename Q=void>
    inline std::string compose_timedelta_1_arg(T const & elem) {
        return compose_timedelta_DRY<T, Q>(elem);
    }

    template <typename T, typename Q=void>
    inline std::string compose_timedelta(T const & elem, std::any const &) {
        return compose_timedelta_DRY<T, Q>(elem);
    }

    /// Returns optionally quoted cell string from a type-agnostic span (cell_span, not typed_span)
    std::string compose_text(auto && span)
        requires(std::is_same_v<std::decay_t<decltype(span)>, typename std::decay_t<decltype(span)>::reader_type::cell_span>) {
        auto constexpr line_break = std::decay_t<decltype(span)>::reader_type::line_break_type::value;
        auto full = span.operator csv_co::cell_string();
        if (full.find_first_of(",\n") != std::string::npos)
            return full;
        // TODO: use std and comment this in more details:
        if constexpr('\n' != line_break) {
            bool stay_quoted = false;
            std::remove_const_t<decltype(std::string::npos)> it;
            while ((it = full.find(line_break)) != std::string::npos) {
                full.replace(it, 1, "\n");
                stay_quoted = true;
            }
            if (stay_quoted)
                return full;
        }
        return span.operator csv_co::unquoted_cell_string();
    }

    /// Returns optionally quoted cell string from a type-aware cell span (typed_span, not cell_span)
    inline std::string compose_text(auto const & e) requires(std::decay_t<decltype(e)>::is_unquoted()) {
        using elem_type = std::decay_t<decltype(e)>;
        static_assert(elem_type::is_unquoted());
        if (e.raw_string_view().find_first_of(",\n") != std::string_view::npos)
            return e;       // convertion to std::string: "Unquoted, please turn quoted!"
        else
            return e.str(); // unquoted via str()
    };

    inline bool ostream_numeric_corner_cases(std::ostringstream & ss, auto const & rep, auto const & args) {
        assert(!rep.is_null());
        ss.str({});

        // Surprisingly, csvkit represents numbers from source without distortion:
        // knowing, that they are valid numbers in a locale, it simply (almost for
        // sure) removes the thousands separators and replaces the decimal point
        // with its C-locale equivalent. So, numbers in a source are actually the
        // outputs too, we only have to do some tricks.

        auto const value = rep.num();

        if (std::isnan(value)) {
            ss << "NaN";
            return true;
        }
        if (std::isinf(value)) {
            ss << (value > 0 ? "Infinity" : "-Infinity");
            return true;
        }
        if (args.num_locale != "C") {
            std::string s = rep.str();
            rep.to_C_locale(s);
            ss << s;
            return true;
        }
        return false;
    }

    class num_stringstream : public std::stringstream {
    public:
        explicit num_stringstream(std::string_view new_locale) {
            imbue(std::locale(std::string(new_locale)));
            tune_ostream<custom_boolean_and_grouping_sep_facet>(*this);
        }
        num_stringstream() {
            tune_ostream(*this);
        }
    };

    class json_indenter {
    public:
        explicit json_indenter(int const indent) : indent(indent), indent_delta(indent) {}
        [[nodiscard]] std::string add_indent() const {
            return add_lf() + (indent >= 0 ? std::string(indent, ' ') : std::string());
        }
        void inc_indent() const noexcept {
            if (indent > 0)
                indent += indent_delta;
        }
        void dec_indent() const noexcept {
            if (indent > 0)
                indent -= indent_delta;
        }
        [[nodiscard]] std::string add_lf() const {
            std::string s;
            if (indent >= 0)
                s = '\n';
            return s;
        }
        [[nodiscard]] int get_indent() const noexcept {
            return indent;
        }
        
    private:
        mutable 
        int indent;
        int indent_delta;
    };

#if 0
#ifndef unix
    class ActiveCodePage {
    public:
        ActiveCodePage() : m_old_code_page(::GetConsoleOutputCP()) {
            ::SetConsoleOutputCP(GetACP());
        }
        ~ActiveCodePage() {
            ::SetConsoleOutputCP(m_old_code_page);
        }
    private:
        UINT m_old_code_page;
    };
#else
    class ActiveCodePage {
    public:
        ActiveCodePage() = default;
    };
#endif
#else
#ifndef unix
    class OutputCodePage65001 {
    public:
        OutputCodePage65001() : m_old_code_page(::GetConsoleOutputCP()) {
            ::SetConsoleOutputCP(65001);
        }
        ~OutputCodePage65001() {
            ::SetConsoleOutputCP(m_old_code_page);
        }
    private:
        UINT m_old_code_page;
    };
#else
    class OutputCodePage65001 {
    public:
       OutputCodePage65001() = default;
    };
#endif

#endif

}
