///
/// \file   utils/csvkit/csvstat.cpp
/// \author wiluite
/// \brief  Print descriptive statistics for each column in a CSV file.

#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <utility>
#include <numeric>
#include <cmath>
#include <cli.h>

using namespace ::csvkit::cli;
using namespace csv_co;

namespace csvstat {

    struct Args : ARGS_positional_1 {
        std::string &num_locale = kwarg("L,locale", "Specify the locale (\"C\") of any formatted numbers.").set_default("C");
        bool &blanks = flag("blanks", R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> &null_value = kwarg("null-value","Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string &date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string &datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        bool &csv = flag("csv", "Output results as a CSV table, rather than plain text.");
        bool &json = flag("json", "Output results as a JSON test, rather than plain text.");
        int &indent = kwarg("i,indent","Indent the output JSON this many spaces. Disabled by default.").set_default(min_int_limit);
        bool &names = flag("n,names", "Display column names and indices from the input CSV and exit.");
        std::string &columns = kwarg("c,columns","A comma-separated list of column indices, names or ranges to be examined, e.g. \"1,id,3-5\".").set_default("all columns");
        // *** operations begin
        bool &type = flag("type", "Only output data type.");
        bool &nulls = flag("nulls", "Only output whether columns contain nulls.");
        bool &non_nulls = flag("non-nulls", "Only output counts of non-null values.");
        bool &unique = flag("unique", "Only output counts of unique values.");
        bool &min = flag("min", "Only output smallest values.");
        bool &max = flag("max", "Only output largest values.");
        bool &sum = flag("sum", "Only output sums.");
        bool &mean = flag("mean", "Only output means.");
        bool &median = flag("median", "Only output medians.");
        bool &stdev = flag("stdev", "Only output standard deviations.");
        bool &len = flag("len", "Only output the length of the longest values.");
        bool &max_precision = flag("max-precision", "Only output the most decimal places.");
        bool &freq = flag("freq", "Only output lists of frequent values.");
        unsigned long &freq_count = kwarg("freq-count","The maximum number of frequent values to display.").set_default(5ul);
        // *** operations end
        bool &count = flag("count", "Only output total row count.");

        std::string &decimal_format = kwarg("decimal-format","%-format specification for printing decimal numbers. Defaults to locale-specific formatting with \"%.3f\"").set_default("%.3f");
        bool &no_grouping_sep = flag("G,no-grouping-separator", "Do not use grouping separators in decimal numbers");
        bool &no_inference = flag("I,no-inference","Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        bool &no_mdp = flag("no-mdp,no-max-precision","Do not calculate most decimal places.");
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);

        void welcome() final {
            std::cout << "\nPrint descriptive statistics for each column in a CSV file.\n\n";
        }
    };

    template<class TabularType, class ArgsType>
    struct base {
        base(std::reference_wrapper<TabularType> _2d, std::reference_wrapper<ArgsType const> args, std::size_t col = 0, std::string col_name = "", unsigned index = 0, bool has_blanks = false);
        base(base const & other)  noexcept = default;
        base& operator= (base const & other)  noexcept = default;
        base(base && other)  noexcept = default;
        base& operator= (base && other)  noexcept = default;
        [[nodiscard]] std::size_t row_count() const;
    private:
        std::reference_wrapper<TabularType> _2d_;
        std::reference_wrapper<ArgsType const> args_;

        unsigned column_; //logical "output index" (zero-based)
        std::string column_name_;
        unsigned phys_index_; // physical index of csv column (zero-based)
        bool blanks_;
        std::shared_ptr<std::ostringstream> single_op_ostream;

        void print_column_header(std::ostream & os) const;
        void default_none_output(std::size_t output_lines);

        virtual void type(std::size_t output_lines) = 0;
        virtual void nulls(std::size_t output_lines);
        virtual void non_nulls(std::size_t output_lines);
        virtual void unique(std::size_t output_lines) = 0;
        virtual void min(std::size_t output_lines);
        virtual void max(std::size_t output_lines);
        virtual void sum(std::size_t output_lines);
        virtual void mean(std::size_t output_lines);
        virtual void median(std::size_t output_lines);
        virtual void stdev(std::size_t output_lines);
        virtual void len(std::size_t output_lines);
        virtual void max_precision(std::size_t output_lines);
        virtual void freq(std::size_t output_lines);
        auto prepare_mcv_vec(auto & mcv_map);

        std::size_t null_values_{0};
        std::size_t non_null_values_{0};
        std::size_t unique_values_{0};
    protected:
        [[nodiscard]] std::reference_wrapper<TabularType> const & dim_2() const { return _2d_; };
        template <typename OutputType = std::string>
        inline void compose_operation_result(std::size_t lines, OutputType const &) const;
        inline void complete(std::size_t null_number, auto & mcv_map, auto & mcv_vec);
    public:
        void single_operation(auto output_lines);
        void operation_result() const;
        // accessors
        [[nodiscard]] auto column() const { return column_; }
        [[nodiscard]] auto column_name() const { return column_name_; }
        [[nodiscard]] auto phys_index() const { return phys_index_; }
        [[nodiscard]] inline auto blanks() const { return blanks_; }
        [[nodiscard]] auto nulls() const { return null_values_; }
        [[nodiscard]] inline std::size_t & non_nulls() { return non_null_values_; }
        [[nodiscard]] std::size_t non_nulls() const { return non_null_values_; }
        [[nodiscard]] std::size_t uniques() const { return unique_values_; }
        [[nodiscard]] std::reference_wrapper<ArgsType const> const & args() const { return args_; }
    };

    template<class B>
    struct number_class final : B {
        using B::B;
        template <class ... T>
        explicit number_class(T &&... args);
        void prepare();
        struct result;
        std::shared_ptr<result> r;
    private:
        void type(std::size_t output_lines) override;
        void unique(std::size_t output_lines) override;
        void min(std::size_t output_lines) noexcept override;
        void max(std::size_t output_lines) noexcept override;
        void sum(std::size_t output_lines) override;
        void mean(std::size_t output_lines) override;
        void median(std::size_t output_lines) override;
        void stdev(std::size_t output_lines) override;
        void max_precision(std::size_t output_lines) override;
        void freq(std::size_t output_lines) override;
        std::map<long double, std::size_t> mcv_map_;
        std::vector<std::pair<long double, std::size_t>> mcv_vec_;
    public:
        [[nodiscard]] auto const & mcv_map() const { return mcv_map_; }
        [[nodiscard]] auto const & mcv_vec() const { return mcv_vec_; }
    };

    template<class B>
    struct timedelta_class final : B {
        using B::B;
        template <class ... T>
        explicit timedelta_class(T &&... args);
        void prepare();
        struct result;
        std::shared_ptr<result> r;
    private:
        void type(std::size_t output_lines) override;
        void unique(std::size_t output_lines) override;
        void min(std::size_t output_lines) noexcept override;
        void max(std::size_t output_lines) noexcept override;
        void sum(std::size_t output_lines) override;
        void mean(std::size_t output_lines) override;
        void freq(std::size_t output_lines) override;
        std::map<long double, std::size_t> mcv_map_;
        std::vector<std::pair<long double, std::size_t>> mcv_vec_;
    public:
        [[nodiscard]] auto const & mcv_map() const { return mcv_map_; }
        [[nodiscard]] auto const & mcv_vec() const { return mcv_vec_; }
    };

    template<class B>
    struct bool_class final : B {
        using B::B;
        void prepare();
    private:
        void type(std::size_t output_lines) override;
        void unique(std::size_t output_lines) override;
        void freq(std::size_t output_lines) override;
        std::map<bool, std::size_t> mcv_map_;
        std::vector<std::pair<bool, std::size_t>> mcv_vec_;
    public:
        [[nodiscard]] auto const & mcv_map() const { return mcv_map_; }
        [[nodiscard]] auto const & mcv_vec() const { return mcv_vec_; }
    };

    template<class B>
    struct text_class final : B {
        using B::B;
        template <class ... T>
        explicit text_class(T &&... args);
        void prepare();
        struct result;
        std::shared_ptr<result> r;
    private:
        void type(std::size_t output_lines) override;
        void unique(std::size_t output_lines) override;
        void len(std::size_t output_lines) override;
        void freq(std::size_t output_lines) override;
        std::map<std::string, std::size_t> mcv_map_;
        std::vector<std::pair<std::string, std::size_t>> mcv_vec_;
    public:
        [[nodiscard]] auto const & mcv_map() const { return mcv_map_; }
        [[nodiscard]] auto const & mcv_vec() const { return mcv_vec_; }
    };

    template<class B>
    struct date_class final : B {
        using B::B;
        template <class ... T>
        explicit date_class(T &&... args);
        void prepare();
        struct result;
        std::shared_ptr<result> r;
    private:
        void type(std::size_t output_lines) override;
        void unique(std::size_t output_lines) override;
        void min(std::size_t output_lines) override;
        void max(std::size_t output_lines) override;
        void freq(std::size_t output_lines) override;
        std::map<date::sys_seconds, std::size_t> mcv_map_;
        std::vector<std::pair<date::sys_seconds, std::size_t>> mcv_vec_;
    public:
        [[nodiscard]] auto const & mcv_map() const { return mcv_map_; }
        [[nodiscard]] auto const & mcv_vec() const { return mcv_vec_; }
    };

    template<class B>
    struct datetime_class final : B {
        using B::B;
        template <class ... T>
        explicit datetime_class(T &&... args);
        void prepare();
        struct result;
        std::shared_ptr<result> r;
    private:
        void type(std::size_t output_lines) override;
        void unique(std::size_t output_lines) override;
        void min(std::size_t output_lines) override;
        void max(std::size_t output_lines) override;
        void freq(std::size_t output_lines) override;
        std::map<date::sys_seconds, std::size_t> mcv_map_;
        std::vector<std::pair<date::sys_seconds, std::size_t>> mcv_vec_;
    public:
        [[nodiscard]] auto const & mcv_map() const { return mcv_map_; }
        [[nodiscard]] auto const & mcv_vec() const { return mcv_vec_; }
    };

    struct no_calculation_class;
    class standard_print_visitor {
        struct rep;
        std::shared_ptr<rep> prep;
    public:
        explicit standard_print_visitor(auto const & args);
        template<class TypeBase>
        void operator()(bool_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(timedelta_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(number_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(text_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(date_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(datetime_class<TypeBase> const &) const;
        void operator()(no_calculation_class const &) const {}
    };

    class csv_print_visitor {
        struct rep;
        std::shared_ptr<rep> prep;
    public:
        explicit csv_print_visitor(auto const & args);
        template<class TypeBase>
        void operator()(bool_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(timedelta_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(number_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(text_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(date_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(datetime_class<TypeBase> const & o) const;
        void operator()(no_calculation_class const &) const {}
    };

    class json_print_visitor {
        struct rep;
        std::shared_ptr<rep> prep;
    public:
        json_print_visitor(auto const & args, std::size_t col_num, unsigned dec_prec, bool is_first_col);
        ~json_print_visitor();
        template<class TypeBase>
        void operator()(bool_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(timedelta_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(number_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(text_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(date_class<TypeBase> const &) const;
        template<class TypeBase>
        void operator()(datetime_class<TypeBase> const & o) const;
        void operator()(no_calculation_class const &) const {}
    };

    void stat(std::monostate &, auto const &, std::string_view = "") {}

    struct no_calculation_class final  {
        void prepare() {}
        void single_operation(std::size_t) {}
        [[nodiscard]] static std::size_t row_count() { return 0; }
        void operation_result() {}
    };

    static auto is_operation(auto const &args) -> bool;

    void stat(auto &reader, auto const &args, std::string_view default_global_locale = "") {
        throw_if_names_and_no_header(args);
        bool operation_option = is_operation(args);

        skip_lines(reader, args);
        quick_check(reader, args);

        auto const header = obtain_header_and_<skip_header>(reader, args);
        {
            max_field_size_checker size_checker(reader, args, header.size(), init_row{1});
            check_max_size(header, size_checker);
        }

        if (args.names) {
            print_header(std::cout, header, args);
            return;
        }
        
        setup_global_locale(default_global_locale);

        auto const body_rows = reader.rows();
        // We can report the number of rows ahead of time if the z option is off
        // Otherwise we have to continue.
        if (args.count and args.maxfieldsize == max_unsigned_limit) {
            std::cout << std::to_string(body_rows) + '\n';
            return;
        }

        try {
            args.columns = args.columns == "all columns" ? "" : args.columns;
            // TODO: DOCUMENT that column detection goes sooner than checking maxfieldsize
            // to escape many-pass through documents
            auto ids = parse_column_identifiers(columns{args.columns}, header, get_column_offset(args), excludes{std::string{}});
            auto const cols = ids.size();

            using cell_span_t = typename std::decay_t<decltype(reader)>::template typed_span<csv_co::unquoted>;
            using tabular_type = fixed_array_2d_replacement<cell_span_t>;

            // Filling in 2d in the transposed form, process each column a bit cache-friendly
            tabular_type transposed_2d(cols, body_rows);

            auto c_row{0u};
            auto c_col{0u};

            max_field_size_checker size_checker(reader, args, header.size(), init_row{args.no_header ? 1u : 2u});

            if (all_columns_selected(ids, header.size()))
                reader.run_rows([&](auto &row_span) { // more cache-friendly
                    check_max_size(row_span, size_checker);
                    for (auto &elem: row_span)
                        transposed_2d[c_col++][c_row] = elem;
                    c_row++;
                    c_col = 0;
                });
            else
                reader.run_rows([&](auto &row_span) { // less cache-friendly
                    check_max_size(row_span, size_checker);
                    for (auto i: ids)
                        transposed_2d[c_col++][c_row] = row_span[i];
                    c_row++;
                    c_col = 0;
                });

            if (args.count) {
                std::cout << std::to_string(body_rows) + '\n';
                return;
            }

            auto prepare_task_vector = [&](auto &reader, auto const &args, auto &transposed_2d, auto const &header, auto const &ids) {
                using Reader = std::decay_t<decltype(reader)>;
                using table_type = fixed_array_2d_replacement<typename Reader::template typed_span<csv_co::unquoted>>;
                auto detect_types_and_blanks = [&] (table_type & table) {

                    update_null_values(args.null_value);

                    std::vector<column_type> task_vec (table.rows(), column_type::unknown_t);

                    std::vector<std::size_t> column_numbers (task_vec.size());
                    std::iota(column_numbers.begin(), column_numbers.end(), 0);

                    transwarp::parallel exec(std::thread::hardware_concurrency());
                    std::vector<bool> blanks (task_vec.size(), false);

                    imbue_numeric_locale(reader, args);

                    [&args] {
                        using reader_type = std::decay_t<decltype(reader)>;

                        using unquoted_elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                        unquoted_elem_type::no_maxprecision(args.no_mdp);

                        using quoted_elem_type = typename reader_type::template typed_span<csv_co::quoted>;
                        quoted_elem_type::no_maxprecision(args.no_mdp);
                    }();

                    setup_date_parser_backend(reader, args);
                    setup_leading_zeroes_processing(reader, args);

                    //TODO: for now e.is_null() calling first is obligate. Can we do better?
                    #define SETUP_BLANKS auto const n = e.is_null(also_match_null_value_option) && !args.blanks; \
                                         if (!blanks[c] && n)                                                    \
                                             blanks[c] = true;

                    auto task = transwarp::for_each(exec, column_numbers.cbegin(), column_numbers.cend(), [&](auto c) {
                        if (std::all_of(table[c].cbegin(), table[c].cend(), [&blanks, &c, &args](auto & e) {
                            SETUP_BLANKS
                            return n || (!args.no_inference && e.is_boolean());
                        })) {
                            task_vec[c] = column_type::bool_t;
                            return;
                        }
                        if (std::all_of(table[c].cbegin(), table[c].cend(), [&args, &blanks, &c](auto &e) {
                            SETUP_BLANKS
                            return n || (!args.no_inference && std::get<0>(e.timedelta_tuple()));
                        })) {
                            task_vec[c] = column_type::timedelta_t;
                            return;
                        }
                        if (std::all_of(table[c].cbegin(), table[c].cend(), [&args, &blanks, &c](auto & e) {
                            SETUP_BLANKS
                            return n || (!args.no_inference && std::get<0>(e.datetime(args.datetime_fmt)));
                        })) {
                            task_vec[c] = column_type::datetime_t;
                            return;
                        }
                        if (std::all_of(table[c].cbegin(), table[c].cend(), [&args, &blanks, &c](auto &e) {
                            SETUP_BLANKS
                            return n || (!args.no_inference && std::get<0>(e.date(args.date_fmt)));
                        })) {
                            task_vec[c] = column_type::date_t;
                            return;
                        }
                        if (std::all_of(table[c].cbegin(), table[c].cend(), [&blanks, &c, &args](auto & e) {
                            SETUP_BLANKS
                            return n || (!args.no_inference && e.is_num());
                        })) {
                            task_vec[c] = column_type::number_t;
                            return;
                        }
                        // Text type: check ALL rows for an absent.
                        if (std::all_of(table[c].cbegin(), table[c].cend(), [&](auto &e) {
                            if (e.is_null(also_match_null_value_option) && !blanks[c] && !args.blanks)
                                blanks[c] = true;
                            return true;
                        })) {
                            task_vec[c] = column_type::text_t;
                            return;
                        }
                    });
                    task->wait();
                    #undef SETUP_BLANKS

                    for (auto & elem : task_vec) {
                        assert(elem != column_type::unknown_t);
                        if (args.no_inference and elem != column_type::text_t) {
                            assert(elem == column_type::bool_t);  // all nulls in a column otherwise boolean
                            elem = column_type::text_t;           // force setting it to text
                        }
                    }

                    return std::tuple{task_vec, blanks};
                };

                auto [types, blanks] = detect_types_and_blanks(transposed_2d);

                using tabular_t = std::decay_t<decltype(transposed_2d)>;
                using args_type = std::decay_t<decltype(args)>;

                using bool_class_type = bool_class<base<tabular_t, args_type>>;
                using timedelta_class_type = timedelta_class<base<tabular_t, args_type>>;
                using number_class_type = number_class<base<tabular_t, args_type>>;
                using datetime_class_type = datetime_class<base<tabular_t, args_type>>;
                using date_class_type = date_class<base<tabular_t, args_type>>;
                using text_class_type = text_class<base<tabular_t, args_type>>;

                using task_vector_type = std::vector<std::variant<no_calculation_class, timedelta_class_type, number_class_type, bool_class_type, text_class_type, datetime_class_type, date_class_type>>;
                task_vector_type task_vec(cols, no_calculation_class{});

                for (auto c = 0ul; c < header.size(); ++c) {
                    if (auto const it = std::find(ids.cbegin(), ids.cend(), c); it != ids.cend()) {
                        auto const index = c;
                        auto const col_name = header[c];
                        std::size_t const col = it - ids.begin();
                        assert(col < task_vec.size());
                        switch (types[col]) {
                            case column_type::bool_t:
                                task_vec[col] = bool_class_type{std::ref(transposed_2d), std::ref(args), col, col_name, index, blanks[col]};
                                break;
                            case column_type::timedelta_t:
                                task_vec[col] = timedelta_class_type{std::ref(transposed_2d), std::ref(args), col, col_name, index, blanks[col]};
                                break;
                            case column_type::number_t:
                                task_vec[col] = number_class_type{std::ref(transposed_2d), std::ref(args), col, col_name, index, blanks[col]};
                                break;
                            case column_type::datetime_t:
                                task_vec[col] = datetime_class_type{std::ref(transposed_2d), std::ref(args), col, col_name, index, blanks[col]};
                                break;
                            case column_type::date_t:
                                task_vec[col] = date_class_type{std::ref(transposed_2d), std::ref(args), col, col_name, index, blanks[col]};
                                break;
                            case column_type::text_t:
                                task_vec[col] = text_class_type{std::ref(transposed_2d), std::ref(args), col, col_name, index, blanks[col]};
                                break;
                            case column_type::unknown_t:
                            case column_type::sz:
                                assert(false && "logic error");
                        }
                    }
                }
                return task_vec;
            };

            auto tv = prepare_task_vector(reader, args, transposed_2d, header, ids);
            transwarp::parallel exec(std::thread::hardware_concurrency());
            auto task = transwarp::for_each(exec, tv.begin(), tv.end(), [&](auto &item) {
                std::visit([&](auto &&arg) {
                    operation_option ? arg.single_operation(cols) : arg.prepare();
                }, item);
            });
            task->wait();

            if (operation_option) {
                for (auto &item: tv)
                    std::visit([&](auto &&arg)  { arg.operation_result(); }, item);
                return;
            } else {
                for (auto &item: tv) {
                    if (args.csv)
                        std::visit(csv_print_visitor{args}, item);
                    else
                        if (args.json) {
                            bool const is_first = std::addressof(item) == &tv[0];
                            unsigned char json_specific_precision = 15; // TODO... or recompile with another value, if needed
                            std::visit(json_print_visitor{args, ids.size(), json_specific_precision, is_first}, item);
                        }
                        else
                            std::visit(standard_print_visitor{args}, item);
                }
            }
            if (!args.csv && !args.json)
                std::visit([](auto &&arg) { std::cout << "Row count: " << std::to_string(arg.row_count()) << '\n'; }, tv[0]);
        } catch (ColumnIdentifierError const &e) {
            std::cout << e.what() << '\n';
        }

    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)

auto mem_avail_mb()
{
#ifdef __unix__
    std::shared_ptr<FILE> cmdfile(
            popen("awk '/MemAvailable/ {printf  $2 / 1024 }' /mcv/meminfo", "r"),pclose);
    char result[256] = { 0 };
    while (fgets(result, sizeof(result), cmdfile.get())) {
        try {
            std::size_t pos;
            unsigned long const r = std::stoll(result, &pos);
            return r;
        }
        catch(std::invalid_argument const& ex) {}
        catch(std::out_of_range const& ex) {}
    }
    return 0ul;
#else
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);
#define WIDTH 7
#define DIV 1024

 #if 0
    std::cout << statex.ullAvailPhys/DIV << std::endl;
    _tprintf (TEXT("There are %*I64d free  KB of physical memory.\n"), WIDTH, statex.ullAvailPhys/DIV);
    return statex.ullAvailPhys/DIV;
 #endif
#endif
}


int main(int argc, char * argv[]) {
#if 0
    std::cout << mem_avail_mb() << std::endl;
    using namespace csvstat;
#endif
    auto args = argparse::parse<csvstat::Args>(argc, argv);
    if (args.verbose)
        args.print();

    // Fix user mistake
    args.freq_count = !args.freq_count ? 5 : args.freq_count;

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(read_standard_input, csvstat::stat)
}
#endif

namespace csvstat { 

    static auto is_operation(auto const &args) -> bool {

        unsigned operations = 0;
        operations += args.count ? 1 : 0;
        operations += args.type ? 1 : 0;
        operations += args.nulls ? 1 : 0;
        operations += args.non_nulls ? 1 : 0;
        operations += args.unique ? 1 : 0;
        operations += args.min ? 1 : 0;
        operations += args.max ? 1 : 0;
        operations += args.sum ? 1 : 0;
        operations += args.mean ? 1 : 0;
        operations += args.median ? 1 : 0;
        operations += args.stdev ? 1 : 0;
        operations += args.len ? 1 : 0;
        operations += args.max_precision ? 1 : 0;
        operations += args.freq ? 1 : 0;

        if (operations > 1 and args.count)
            throw std::runtime_error("csvstat: error: You may not specify --count and an operation (--mean, --median, etc) at the same time.");
        if (operations and args.csv and !args.count)
            throw std::runtime_error("csvstat: error: You may not specify --csv and an operation (--mean, --median, etc) at the same time.");
        if (operations and args.json and !args.count)
            throw std::runtime_error("csvstat: error: You may not specify --json and an operation (--mean, --median, etc) at the same time.");
        if (operations > 1)
            throw std::runtime_error("csvstat: error: Only one operation argument may be specified (--mean, --median, etc).");
        return operations;
    }

    template<class TabularType, class ArgsType>
    base<TabularType, ArgsType>::base(std::reference_wrapper<TabularType> _2d, std::reference_wrapper<ArgsType const> args, std::size_t col, std::string col_name, unsigned index, bool blanks)
        : _2d_(_2d), args_(args), column_(col), column_name_(std::move(col_name)), phys_index_(index), blanks_(blanks), single_op_ostream(std::make_shared<std::ostringstream>()) {
        tune_ostream(*single_op_ostream, args_.get());
    }

    template<class TabularType, class ArgsType>
    std::size_t base<TabularType, ArgsType>::row_count() const {
        return _2d_.get().cols();
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::print_column_header(std::ostream & os) const {
        assert(!column_name_.empty());
        os << std::setw(3) << phys_index_ + 1 << ". " << static_cast<const decltype(column_name_)>(column_name_) << ": ";
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::default_none_output(std::size_t output_lines) {
        compose_operation_result(output_lines, "None");
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::nulls(std::size_t output_lines) {
        auto &&slice = _2d_.get()[column_];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option)) {
                null_num++;
                break;
            }
        }
        compose_operation_result(output_lines, (null_num ? "True" : "False"));
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::non_nulls(std::size_t output_lines) {
        auto &&slice = _2d_.get()[column_];
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option))
                non_null_values_++;
        }
        compose_operation_result(output_lines, std::to_string(non_null_values_));
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::min(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::max(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::sum(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::mean(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::median(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::stdev(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::len(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::max_precision(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::freq(std::size_t output_lines) {
        default_none_output(output_lines);
    }

    template<class TabularType, class ArgsType>
    auto base<TabularType, ArgsType>::prepare_mcv_vec(auto & mcv_map) {
        using map_value_type = std::pair<typename std::decay_t<decltype(mcv_map)>::key_type, std::size_t>;
        std::vector<map_value_type> v(mcv_map.begin(), mcv_map.end());

        auto const mcv_printed = args_.get().freq_count;
        auto const min_of_2 = std::min(static_cast<unsigned long>(v.size()), mcv_printed);
        std::partial_sort(v.begin(), v.begin() + min_of_2, v.end(), [](auto &e1, auto &e2) { return e1.second > e2.second; });

        std::vector<map_value_type> arr(mcv_printed);
        std::copy_n(v.begin(), min_of_2, arr.begin());
        return arr;
    }

    template<class TabularType, class ArgsType>
    template<typename OutputType>
    inline void base<TabularType, ArgsType>::compose_operation_result(std::size_t lines, OutputType const & value) const {
        if constexpr(std::is_same_v<OutputType, std::string> or std::is_same_v<std::decay_t<OutputType>, char*> or std::is_same_v<std::decay_t<OutputType>, char const*>)
            *single_op_ostream << (lines == 1 ? (value) : (print_column_header(*single_op_ostream), value));
        else if constexpr (std::is_same_v<long double, std::remove_reference_t<std::decay_t<OutputType>>>)
            *single_op_ostream << (lines == 1 ? (spec_prec(value)) : (print_column_header(*single_op_ostream), spec_prec(value)));
        else if constexpr (std::is_same_v<unsigned char, std::remove_reference_t<std::decay_t<OutputType>>>)
            *single_op_ostream << (lines == 1 ? (static_cast<int>(value)) : (print_column_header(*single_op_ostream), static_cast<int>(value)));
        *single_op_ostream << '\n';
    }

    template<class TabularType, class ArgsType>
    inline void base<TabularType, ArgsType>::complete(std::size_t null_number, auto & mcv_map, auto & mcv_vec) {
        null_values_ = null_number;
        unique_values_ = mcv_map.size() + (null_number ? 1 : 0);
        mcv_vec = prepare_mcv_vec(mcv_map);
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::single_operation(auto output_lines) {
        if (args_.get().type)
            type(output_lines);
        else if (args_.get().nulls)
            nulls(output_lines);
        else if (args_.get().non_nulls)
            non_nulls(output_lines);
        else if (args_.get().unique)
            unique(output_lines);
        else if (args_.get().min)
            min(output_lines);
        else if (args_.get().max)
            max(output_lines);
        else if (args_.get().sum)
            sum(output_lines);
        else if (args_.get().mean)
            mean(output_lines);
        else if (args_.get().median)
            median(output_lines);
        else if (args_.get().stdev) 
            stdev(output_lines);
        else if (args_.get().len)
            len(output_lines);
        else if (args_.get().max_precision)
            max_precision(output_lines);
        else if (args_.get().freq)
            freq(output_lines);        
    }

    template<class TabularType, class ArgsType>
    void base<TabularType, ArgsType>::operation_result() const {
        std::cout << single_op_ostream->str();
    }

    template<class B>
    struct number_class<B>::result {
        long double smallest_value;
        long double largest_value;
        long double sum;
        long double mean;
        long double median;
        long double stdev;
        unsigned    mdp;
    };

    template<class B>
    template<class ... T>
    number_class<B>::number_class(T &&... args) : B(std::forward<T>(args)...), r(std::make_shared<result>()) {}

    template<class B>
    void number_class<B>::prepare() {
        auto &&slice = B::dim_2().get()[B::column()];
        long double current_rolling_mean = 0, current_rolling_var = 0, current_n = 0, sum = 0;
        long double min_ = std::numeric_limits<long double>::max(), max_ = std::numeric_limits<long double>::lowest();

        std::function<void(long double const &elem_val)> common_lambda;

        auto rest_loops_lambda = [&](auto const &elem_val) noexcept {
            auto const delta = elem_val - current_rolling_mean;
            current_rolling_mean += delta / current_n;
            auto const delta2 = elem_val - current_rolling_mean;
            current_rolling_var += delta * delta2;
        };

        auto first_loop_lambda = [&](auto const &elem_val) {
            current_rolling_mean = elem_val;
            common_lambda = rest_loops_lambda;
        };

        common_lambda = first_loop_lambda;

        std::size_t null_number = 0;
        unsigned char mdp = 0;

        using elem_t = decltype(slice[0]);
        std::function<unsigned char(elem_t&)> mdp_calc;
        if (this->args().get().no_mdp)
            mdp_calc = [&](elem_t&) { return 0; };
        else
            mdp_calc = [&](elem_t& elem) { return std::max(elem.precision(), mdp); };

        for (auto & elem : slice) {
            assert(elem.is_num() || elem.is_null(also_match_null_value_option));
            if (!elem.is_null(also_match_null_value_option)) {
                current_n++;
                auto const element_value = elem.num();
                sum += element_value;
                //TODO: probably 'if' would be faster
                max_ = std::max(element_value, max_); 
                min_ = std::min(element_value, min_);
                common_lambda(element_value);
                mcv_map_[element_value]++;
                ++B::non_nulls();
                mdp = mdp_calc(elem);
            } else 
                null_number++;
        }

        long double median;

        if (!null_number) {
            auto const mm = slice.begin() + slice.size() / 2;
            std::nth_element(slice.begin(), mm, slice.end(), [](auto &elem, auto &elem2) {
                auto const left = elem.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem.num();
                auto const right = elem2.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem2.num();
                return left < right;
            });

            if (slice.size() & 1)
                median = slice[slice.size() / 2].num();
            else {
                auto const _ = std::max_element(slice.begin(), slice.begin() + slice.size() / 2, [&](auto &elem, auto &elem2) {
                    return elem.num() < elem2.num();
                });
                median = (slice[slice.size() / 2].num() + (*_).num()) / 2;
            }
        } else {
            auto const good_size = slice.size() - null_number;

            std::sort(slice.begin(), slice.end(), [](auto &elem, auto &elem2) {
                auto const left = elem.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem.num();
                auto const right = elem2.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem2.num();
                return left < right;
            });

            median = (good_size & 1) ? slice[good_size / 2].num() : (slice[good_size / 2].num() + slice[good_size / 2 - 1].num()) / 2.0;
        }

        r->smallest_value = min_;
        r->largest_value = max_;
        r->sum = sum;
        r->mean = current_rolling_mean;
        r->median = median;
        r->stdev = std::sqrt(current_rolling_var / (current_n - 1));
        r->mdp = mdp;

        B::complete(null_number, mcv_map_, mcv_vec_);
    }

    template<class B>
    void number_class<B>::type(std::size_t output_lines) {
        B::compose_operation_result(output_lines, "Number");
    }

    template<class B>
    void number_class<B>::unique(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[elem.num()]++;
        }
        B::compose_operation_result(output_lines, std::to_string(mcv_map_.size() + (null_num ? 1 : 0)));
    }

    template<class B>
    void number_class<B>::min(std::size_t output_lines) noexcept {
        auto &&slice = B::dim_2().get()[B::column()];
        auto min_ = std::numeric_limits<long double>::max();
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option))
                min_ = std::min(elem.num(), min_);
        }
        B::compose_operation_result(output_lines, min_);
    }

    template<class B>
    void number_class<B>::max(std::size_t output_lines) noexcept {
        auto &&slice = B::dim_2().get()[B::column()];
        long double max_ = std::numeric_limits<long double>::lowest();
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option))
                max_ = std::max(elem.num(), max_);
        }
        B::compose_operation_result(output_lines, max_);
    }

    template<class B>
    void number_class<B>::sum(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        long double sum = 0;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option))
                sum += elem.num();
        }
        B::compose_operation_result(output_lines, sum);
    }

    template<class B>
    void number_class<B>::mean(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        long double current_rolling_mean = 0, current_n = 0;

        std::function<void(long double const &elem_val)> common_lambda;

        auto rest_loops_lambda = [&](auto const &elem_val) noexcept {
            auto const delta = elem_val - current_rolling_mean;
            current_rolling_mean += delta / current_n;
        };

        auto first_loop_lambda = [&](auto const &elem_val) {
            current_rolling_mean = elem_val;
            common_lambda = rest_loops_lambda;
        };

        common_lambda = first_loop_lambda;

        for (auto const & elem : slice) {
            assert(elem.is_num() || elem.is_null(also_match_null_value_option));
            if (!elem.is_null(also_match_null_value_option)) {
                current_n++;
                common_lambda(elem.num());
            }
        }
        B::compose_operation_result(output_lines, current_rolling_mean);
    }

    template<class B>
    void number_class<B>::median(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_number = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_number++;
        }

        long double median;

        if (!null_number) {
            auto const mm = slice.begin() + slice.size() / 2;
            std::nth_element(slice.begin(), mm, slice.end(), [](auto &elem, auto &elem2) {
                auto const left = elem.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem.num();
                auto const right = elem2.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem2.num();
                return left < right;
            });

            if (slice.size() & 1) {
                median = slice[slice.size() / 2].num();
            } else {
                auto const _ = std::max_element(slice.begin(), slice.begin() + slice.size() / 2, [&](auto &elem, auto &elem2) {
                    return elem.num() < elem2.num();
                });
                median = (slice[slice.size() / 2].num() + (*_).num()) / 2;
            }
        } else {
            auto const good_size = slice.size() - null_number;

            std::sort(slice.begin(), slice.end(), [](auto &elem, auto &elem2) {
                auto const left = elem.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem.num();
                auto const right = elem2.is_null(also_match_null_value_option) ? std::numeric_limits<long double>::max() : elem2.num();
                return left < right;
            });

            median = (good_size & 1) ? slice[good_size / 2].num() :
                     (slice[good_size / 2].num() + slice[good_size / 2 - 1].num()) / 2.0;
        }
        B::compose_operation_result(output_lines, median);
    }

    template<class B>
    void number_class<B>::stdev(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        long double current_rolling_mean = 0, current_rolling_var = 0, current_n = 0;

        std::function<void(long double const &elem_val)> common_lambda;

        auto rest_loops_lambda = [&](auto const &elem_val) noexcept {
            auto const delta = elem_val - current_rolling_mean;
            current_rolling_mean += delta / current_n;
            auto const delta2 = elem_val - current_rolling_mean;
            current_rolling_var += delta * delta2;
        };

        auto first_loop_lambda = [&](auto const &elem_val) noexcept {
            current_rolling_mean = elem_val;
            common_lambda = rest_loops_lambda;
        };

        common_lambda = first_loop_lambda;

        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                current_n++;
                common_lambda(elem.num());
            }
        }

        auto const stdev = std::sqrt(current_rolling_var / (current_n - 1));
        if (!std::isnan(stdev))
            B::compose_operation_result(output_lines, stdev);
        else
            B::compose_operation_result(output_lines, "None");
    }

    template<class B>
    void number_class<B>::max_precision(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        unsigned char max_prec = 0;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option))
                max_prec = std::max(elem.precision(), max_prec);
        }
        B::compose_operation_result(output_lines, max_prec);
    }

    auto freq_none_print = [](std::ostringstream & os, auto const & o) {
        os << "\"None\": " << o.nulls();
    };

    auto freq_space_print = [](std::ostringstream & os, auto & first_printed) {
        os << (first_printed++ ? ", " : "");
    };

    template<class B>
    void number_class<B>::freq(std::size_t output_lines)
    {
        static struct {
            using visitor_type = void;
            auto to_strm(std::ostringstream & oss, std::pair<long double, std::size_t> const & elem, number_class<B> const &) const {
                oss.imbue(std::locale("C"));
                oss << '"' << elem.first << '"' << ": " << elem.second;
            }
        } value_caller;

        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[elem.num()]++;
        }
        B::complete(null_num, mcv_map_, mcv_vec_);
        B::compose_operation_result(output_lines, "{ " + mcv(*this, freq_none_print, freq_space_print, &value_caller) + " }");
    }

    template<class B>
    struct timedelta_class<B>::result {
        long double smallest_value;
        long double largest_value;
        long double sum;
        long double mean;
    };

    template<class B>
    template<class ... T>
    timedelta_class<B>::timedelta_class(T &&... args) : B(std::forward<T>(args)...), r(std::make_shared<result>()) {}

    template<class B>
    void timedelta_class<B>::prepare() {

        auto &&slice = B::dim_2().get()[B::column()];
        long double current_rolling_mean = 0, current_rolling_var = 0, current_n = 0, sum = 0;
        long double min_ = std::numeric_limits<long double>::max(), max_ = std::numeric_limits<long double>::lowest();

        std::function<void(long double const &elem_val)> common_lambda;

        auto rest_loops_lambda = [&](auto const &elem_val) noexcept {
            auto const delta = elem_val - current_rolling_mean;
            current_rolling_mean += delta / current_n;
            auto const delta2 = elem_val - current_rolling_mean;
            current_rolling_var += delta * delta2;
        };

        auto first_loop_lambda = [&](auto const &elem_val) {
            current_rolling_mean = elem_val;
            common_lambda = rest_loops_lambda;
        };

        common_lambda = first_loop_lambda;

        std::size_t null_number = 0;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                current_n++;
                auto const element_value = elem.timedelta_seconds();
                sum += element_value;
                max_ = std::max(element_value, max_);
                min_ = std::min(element_value, min_);
                common_lambda(element_value);
                mcv_map_[element_value]++;
                ++B::non_nulls();
            } else
                null_number++;
        }

        r->smallest_value = min_;
        r->largest_value = max_;
        r->sum = sum;
        r->mean = current_rolling_mean;

        B::complete(null_number, mcv_map_, mcv_vec_);
    }

    template<class B>
    void timedelta_class<B>::type(std::size_t output_lines) {
        B::compose_operation_result(output_lines, "TimeDelta");
    }

    template<class B>
    void timedelta_class<B>::unique(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const &elem: slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[elem.timedelta_seconds()]++;
        }
        B::compose_operation_result(output_lines, std::to_string(mcv_map_.size() + (null_num ? 1 : 0)));
    }

    template<class B>
    void timedelta_class<B>::min(std::size_t output_lines) noexcept {
        auto &&slice = B::dim_2().get()[B::column()];
        auto min_ = std::numeric_limits<long double>::max();
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                auto const current = elem.timedelta_seconds();
                if (min_ >= current)
                    min_ = current;
            }
        }
        B::compose_operation_result(output_lines, csv_co::time_storage().str(min_));
    }

    template<class B>
    void timedelta_class<B>::max(std::size_t output_lines) noexcept {
        auto &&slice = B::dim_2().get()[B::column()];
        auto max_ = std::numeric_limits<long double>::lowest();
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                auto const current = elem.timedelta_seconds();
                if (max_ <= current)
                    max_ = current;
            }
        }
        B::compose_operation_result(output_lines, csv_co::time_storage().str(max_));
    }

    template<class B>
    void timedelta_class<B>::sum(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        long double sum = 0;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option))
                sum += elem.timedelta_seconds();
        }
        B::compose_operation_result(output_lines, csv_co::time_storage().str(sum));
    }

    template<class B>
    void timedelta_class<B>::mean(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        long double current_rolling_mean = 0, current_n = 0;

        std::function<void(long double const &elem_val)> common_lambda;

        auto rest_loops_lambda = [&](auto const &elem_val) noexcept {
            auto const delta = elem_val - current_rolling_mean;
            current_rolling_mean += delta / current_n;
        };

        auto first_loop_lambda = [&](auto const &elem_val) {
            current_rolling_mean = elem_val;
            common_lambda = rest_loops_lambda;
        };

        common_lambda = first_loop_lambda;

        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                current_n++;
                common_lambda(elem.timedelta_seconds());
            }
        }
        B::compose_operation_result(output_lines, csv_co::time_storage().str(current_rolling_mean));
    }

    template<class B>
    void timedelta_class<B>::freq(std::size_t output_lines) {
        static struct {
            using visitor_type = void;
            auto to_strm(std::ostringstream & oss, std::pair<long double, std::size_t> const & elem, timedelta_class<B> const &) const {
                oss.imbue(std::locale("C"));
                oss << '"' << csv_co::time_storage().str(elem.first) << '"' << ": " << elem.second;
            }
        } value_caller;

        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[elem.timedelta_seconds()]++;
        }
        B::complete(null_num, mcv_map_, mcv_vec_);
        B::compose_operation_result(output_lines, "{ " + mcv(*this, freq_none_print, freq_space_print, &value_caller) + " }");
    }

    template<class B>
    void bool_class<B>::prepare() {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_number = 0;
        for (auto const & e : slice) {
            if (!e.is_null(also_match_null_value_option)) {
                mcv_map_[e.unsafe_bool()]++;
                ++B::non_nulls();
            } else
                null_number++;
            assert(e.is_boolean() || e.is_null(also_match_null_value_option));
        }

        B::complete(null_number, mcv_map_, mcv_vec_);
    }

    template<class B>
    void bool_class<B>::type(std::size_t output_lines) {
        B::compose_operation_result(output_lines, "Boolean");
    }

    template<class B>
    void bool_class<B>::unique(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[elem.unsafe_bool()]++;
        }
        B::compose_operation_result(output_lines, std::to_string(mcv_map_.size() + (null_num ? 1 : 0)));
    }

    template<class B>
    void bool_class<B>::freq(std::size_t output_lines) {
        static struct {
            using visitor_type = void;
            auto to_strm(std::ostringstream & oss, std::pair<bool, std::size_t> const & elem, bool_class<B> const &) const {
                oss.imbue(std::locale(std::locale("C"), new custom_boolean_facet));
                oss << '"' << std::boolalpha << elem.first << '"' << ": " << elem.second;
            }
        } value_caller;

        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[elem.unsafe_bool()]++;
        }

        B::complete(null_num, mcv_map_, mcv_vec_);
        B::compose_operation_result(output_lines, "{ " + mcv(*this, freq_none_print, freq_space_print, &value_caller) + " }");
    }

    template<class B>
    struct text_class<B>::result {
        unsigned longest_value;
    };

    template<class B>
    template<class ... T>
    text_class<B>::text_class(T &&... args) : B(std::forward<T>(args)...), r(std::make_shared<result>()) {
    }

    template<class B>
    void text_class<B>::prepare() {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t longest_value = 0;
        std::size_t null_number = 0;
        for (auto const & e : slice) {
            if (!B::blanks() or !e.is_null(also_match_null_value_option)) {
                auto const size_in_symbols = e.unsafe_str_size_in_symbols();
                longest_value = (size_in_symbols > longest_value) ? size_in_symbols : longest_value;
                mcv_map_[e.str()]++;
                ++B::non_nulls();
            } else
                null_number++;
            assert(e.is_str() || e.is_null(also_match_null_value_option) || e.is_boolean() || e.is_num());
        }
        r->longest_value = longest_value;
        B::complete(null_number, mcv_map_, mcv_vec_);
    }

    template<class B>
    void text_class<B>::type(std::size_t output_lines) {
        B::compose_operation_result(output_lines, "Text");
    }

    template<class B>
    void text_class<B>::unique(std::size_t output_lines)  {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (!B::blanks() or !elem.is_null(also_match_null_value_option))
                mcv_map_[elem.str()]++;
            else
                null_num++;
        }
        B::compose_operation_result(output_lines, std::to_string(mcv_map_.size() + (null_num ? 1 : 0)));
    }

    template<class B>
    void text_class<B>::len(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t longest_value = 0;
        for (auto const & e : slice) {
            if (!B::blanks() or !e.is_null(also_match_null_value_option)) {
                auto const size_in_symbols = e.unsafe_str_size_in_symbols();
                longest_value = (size_in_symbols > longest_value) ? size_in_symbols : longest_value;
            }
        }
        B::compose_operation_result(output_lines, std::to_string(longest_value));
    }

    template<class B>
    void text_class<B>::freq(std::size_t output_lines)
    {
        static struct {
            using visitor_type = void;
            auto to_strm(std::ostringstream & oss, std::pair<std::string, std::size_t> const & elem, text_class<B> const &) const {
                oss.imbue(std::locale("C"));
                oss << '"' << elem.first << '"' <<  ": " << elem.second;
#if 0
                oss << std::quoted(elem.first) <<  ": " << elem.second;
#endif
            }
        } value_caller;

        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (!B::blanks() or !elem.is_null(also_match_null_value_option))
                mcv_map_[elem.str()]++;
            else
                null_num++;                
        }

        B::complete(null_num, mcv_map_, mcv_vec_);
        B::compose_operation_result(output_lines, "{ " + mcv(*this, freq_none_print, freq_space_print, &value_caller) + " }");
    }

    template<class B>
    struct date_class<B>::result {
        std::string smallest_value;
        std::string largest_value;
    };

    template<class B>
    template<class ... T>
    date_class<B>::date_class(T &&... args) : B(std::forward<T>(args)...), r(std::make_shared<result>()) {
    }

    template<class B>
    void date_class<B>::prepare() {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_number = 0;
        std::time_t min_ = std::numeric_limits<std::time_t>::max();
        date::sys_seconds min_value;
        std::time_t max_ = std::numeric_limits<std::time_t>::min();
        date::sys_seconds max_value;
        for (auto const & elem : slice) {
            assert(std::get<0>(elem.date()) || elem.is_null(also_match_null_value_option));
            if (!elem.is_null(also_match_null_value_option)) {
                auto const elem_value = date_time_point(elem); 
                auto e_time_t = std::chrono::system_clock::to_time_t(elem_value);   
                if (max_ <= e_time_t) {
                    max_ = e_time_t;
                    max_value = elem_value;
                } 
                if (min_ >= e_time_t) {
                    min_ = e_time_t;
                    min_value = elem_value;
                } 
                mcv_map_[elem_value]++;
                ++B::non_nulls();
            } else
                null_number++;
        }
        r->largest_value = date_s(max_value);
        r->smallest_value = date_s(min_value);
        B::complete(null_number, mcv_map_, mcv_vec_);
    }

    template<class B>
    void date_class<B>::type(std::size_t output_lines) {
        B::compose_operation_result(output_lines, "Date");
    }

    template<class B>
    void date_class<B>::unique(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[date_time_point(elem)]++;
        }
        B::compose_operation_result(output_lines, std::to_string(mcv_map_.size() + (null_num ? 1 : 0)));
    }

    template<class B>
    void date_class<B>::min(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        auto min_ = std::numeric_limits<std::time_t>::max();
        date::sys_seconds min_value;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                auto const elem_value = date_time_point(elem);   
                auto e_time_t = std::chrono::system_clock::to_time_t(elem_value);
                if (min_ >= e_time_t) {
                    min_ = e_time_t;
                    min_value = elem_value;
                }
            }
        }
        B::compose_operation_result(output_lines, date_s(min_value));
    }

    template<class B>
    void date_class<B>::max(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        auto max_ = std::numeric_limits<std::time_t>::min();
        date::sys_seconds max_value;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                auto const elem_value = date_time_point(elem);   
                auto e_time_t = std::chrono::system_clock::to_time_t(elem_value);
                if (max_ <= e_time_t) {
                    max_ = e_time_t;
                    max_value = elem_value;
                }
            }
        }
        B::compose_operation_result(output_lines, date_s(max_value));
    }

    template<class B>
    void date_class<B>::freq(std::size_t output_lines) {
        static struct {
            using visitor_type = void;
            auto to_strm(std::ostringstream & oss, std::pair<date::sys_seconds, std::size_t> const & elem, date_class<B> const &) const {
                oss.imbue(std::locale("C"));
                oss << std::quoted(date_s(elem.first)) << ": " << elem.second;
            }
        } value_caller;

        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[date_time_point(elem)]++;
        }

        B::complete(null_num, mcv_map_, mcv_vec_);
        B::compose_operation_result(output_lines, "{ " + mcv(*this, freq_none_print, freq_space_print, &value_caller) + " }");
    }

    template<class B>
    struct datetime_class<B>::result {
        std::string smallest_value;
        std::string largest_value;
    };

    template<class B>
    template<class ... T>
    datetime_class<B>::datetime_class(T &&... args) : B(std::forward<T>(args)...), r(std::make_shared<result>()) {
    }

    template<class B>
    void datetime_class<B>::prepare() {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_number = 0;
        std::time_t min_ = std::numeric_limits<std::time_t>::max();
        date::sys_seconds min_value;
        std::time_t max_ = std::numeric_limits<std::time_t>::min();
        date::sys_seconds max_value;
        for (auto const & elem : slice) {
            assert(std::get<0>(elem.datetime()) || elem.is_null(also_match_null_value_option));
            if (!elem.is_null(also_match_null_value_option)) {
                auto const elem_value = datetime_time_point(elem);
                auto e_time_t = std::chrono::system_clock::to_time_t(elem_value);   
                if (max_ <= e_time_t) {
                    max_ = e_time_t;
                    max_value = elem_value;
                } 
                if (min_ >= e_time_t) {
                    min_ = e_time_t;
                    min_value = elem_value;
                } 
                mcv_map_[elem_value]++;
                ++B::non_nulls();
            } else
                null_number++;
        }

        r->largest_value = this->args().get().json ? datetime_s_json(max_value) : datetime_s(max_value);
        r->smallest_value = this->args().get().json ? datetime_s_json(min_value) : datetime_s(min_value);

        B::complete(null_number, mcv_map_, mcv_vec_);
    }

    template<class B>
    void datetime_class<B>::type(std::size_t output_lines) {
        B::compose_operation_result(output_lines, "DateTime");
    }

    template<class B>
    void datetime_class<B>::unique(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[datetime_time_point(elem)]++;
        }
        B::compose_operation_result(output_lines, std::to_string(mcv_map_.size() + (null_num ? 1 : 0)));
    }

    template<class B>
    void datetime_class<B>::min(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        auto min_ = std::numeric_limits<std::time_t>::max();
        date::sys_seconds min_value;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                auto const elem_value = datetime_time_point(elem);   
                auto e_time_t = std::chrono::system_clock::to_time_t(elem_value);
                if (min_ >= e_time_t) {
                    min_ = e_time_t;
                    min_value = elem_value;
                }
            }
        }
        B::compose_operation_result(output_lines, datetime_s(min_value));
    }

    template<class B>
    void datetime_class<B>::max(std::size_t output_lines) {
        auto &&slice = B::dim_2().get()[B::column()];
        auto max_ = std::numeric_limits<std::time_t>::min();
        date::sys_seconds max_value;
        for (auto const & elem : slice) {
            if (!elem.is_null(also_match_null_value_option)) {
                auto const elem_value = datetime_time_point(elem);   
                auto e_time_t = std::chrono::system_clock::to_time_t(elem_value);
                if (max_ <= e_time_t) {
                    max_ = e_time_t;
                    max_value = elem_value;
                }
            }
        }
        B::compose_operation_result(output_lines, datetime_s(max_value));
    }

    template<class B>
    void datetime_class<B>::freq(std::size_t output_lines) {
        static struct {
            using visitor_type = void;
            auto to_strm(std::ostringstream & oss, std::pair<date::sys_seconds, std::size_t> const & elem, datetime_class<B> const &) const {    
                oss.imbue(std::locale("C"));
                oss << std::quoted(datetime_s(elem.first)) << ": " << elem.second; 
            }
        } value_caller;

        auto &&slice = B::dim_2().get()[B::column()];
        std::size_t null_num = 0;
        for (auto const & elem : slice) {
            if (elem.is_null(also_match_null_value_option))
                null_num++;
            else
                mcv_map_[datetime_time_point(elem)]++;
        }

        B::complete(null_num, mcv_map_, mcv_vec_);
        B::compose_operation_result(output_lines, "{ " + mcv(*this, freq_none_print, freq_space_print, &value_caller) + " }");
    }

    template <typename ObjectType, typename NonePrintFun, typename SpacePrintFun, class Variant>
    [[nodiscard]] auto mcv(ObjectType const& o, NonePrintFun none_print_fun, SpacePrintFun space_print_fun, Variant const & vo) {

        std::ostringstream oss;
        if (o.mcv_map().empty() && o.nulls()) {
            if constexpr (!std::is_same_v<typename std::decay_t<decltype(*vo)>::visitor_type, json_print_visitor>)
                none_print_fun(oss, o);
            else
                none_print_fun(oss, o, vo);
            return oss.str();
        }

        const auto mcv_printed = o.args().get().freq_count;
        bool none_printed = false;
        unsigned num_printed = 0, first_printed = 0;

        for (auto const &elem : o.mcv_vec()) {
            if (elem.second != 0) {
                space_print_fun(oss, first_printed);

                auto print_data = [&] {
                    vo->to_strm(oss, elem, o);
                    num_printed++;
                };

                if (elem.second >= o.nulls())
                    print_data();
                else {
                    if (!none_printed) {
                        if constexpr (!std::is_same_v<typename std::decay_t<decltype(*vo)>::visitor_type, json_print_visitor>)
                            none_print_fun(oss, o);
                        else
                            none_print_fun(oss, o, vo);
                        none_printed = true;
                        num_printed++;
                        space_print_fun(oss, first_printed);
                    }
                    if (num_printed < mcv_printed)
                        print_data();
                }
            }
        }

        if (num_printed != mcv_printed && !none_printed && o.nulls()) {
            space_print_fun(oss, first_printed);
            if constexpr (!std::is_same_v<typename std::decay_t<decltype(*vo)>::visitor_type, json_print_visitor>)
                none_print_fun(oss, o);
            else
                none_print_fun(oss, o, vo);
        }

        return oss.str();
    }

    struct standard_print_visitor::rep {
        using visitor_type = standard_print_visitor;
        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, bool_class<T> const &) const {
            oss.imbue(std::locale(std::locale(), new custom_boolean_facet));
            to_stream(oss, std::boolalpha, elem.first, " (", std::to_string(elem.second), "x)", '\n');
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, timedelta_class<T> const & o) const {
            tune_ostream(oss, o.args().get());
            to_stream(oss, csv_co::time_storage().str(elem.first), " (", std::to_string(elem.second), "x)", '\n');
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, number_class<T> const & o) const {
            tune_ostream(oss, o.args().get());
            to_stream(oss, spec_prec(elem.first), " (", std::to_string(elem.second), "x)", '\n');
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, text_class<T> const &) const {
            to_stream(oss, elem.first, " (", std::to_string(elem.second), "x)", '\n');
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, date_class<T> const &) const {
            to_stream(oss, date_s(elem.first), " (", std::to_string(elem.second), "x)", '\n');
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, datetime_class<T> const &) const {
            to_stream(oss, datetime_s(elem.first), " (", std::to_string(elem.second), "x)", '\n');
        }

        void print_col_header(auto const & o) {
            assert(!o.column_name().empty());
            to_stream(std::cout, std::setw(3), o.phys_index() + 1, ". ", std::quoted(static_cast<const decltype(o.column_name())>(o.column_name())));
        }
    };

    standard_print_visitor::standard_print_visitor(auto const & args) : prep{std::make_shared<rep>()} {
        tune_ostream(std::cout, args);
    }

    auto standard_none_print = [](std::ostringstream & os, auto const & o) {
        os << ("None (" + std::to_string(o.nulls()) + "x)\n");
    };

    auto standard_space_print = [](std::ostringstream & os, auto & first_printed) {
        os << (first_printed++ ? ("\t\t\t       ") : "");
    };

    template<class T>
    void standard_print_visitor::operator()(number_class<T> const & o) const {
        prep->print_col_header(o);
        to_stream(std::cout, "\n\n\tType of data:          Number", "\n\tContains null values:  ", std::boolalpha, o.nulls() > 0
                  , (o.nulls() ? " (excluded from calculations)\n" : "\n"), "\tNon-null values:       ", o.non_nulls()
                  , "\n\tUnique values:         ", o.uniques(), "\n\tSmallest value:        ", spec_prec(o.r->smallest_value)
                  , "\n\tLargest value:         ", spec_prec(o.r->largest_value), "\n\tSum:                   ", spec_prec(o.r->sum)
                  , "\n\tMean:                  ", spec_prec(o.r->mean), "\n\tMedian:                ", spec_prec(o.r->median));
        if (!std::isnan(o.r->stdev))
            to_stream(std::cout, "\n\tStDev:                 ", spec_prec(o.r->stdev));
        if (!o.args().get().no_mdp)
            to_stream(std::cout, "\n\tMost decimal places:   ", o.r->mdp);
        to_stream(std::cout, "\n\tMost common values:    ", mcv(o, standard_none_print, standard_space_print, prep),'\n');
    }

    template<class T>
    void standard_print_visitor::operator()(bool_class<T> const & o) const {
        prep->print_col_header(o);
        to_stream(std::cout, "\n\n\tType of data:          Boolean", "\n\tContains null values:  ", std::boolalpha, o.nulls() > 0
                  , (o.nulls() ? " (excluded from calculations)\n" : "\n"), "\tNon-null values:       ", o.non_nulls()
                  , "\n\tUnique values:         ", o.uniques(), "\n\tMost common values:    ", mcv(o, standard_none_print, standard_space_print, prep), '\n');
    }

    template<class T>
    void standard_print_visitor::operator()(timedelta_class<T> const & o) const {
        prep->print_col_header(o);
        to_stream(std::cout, "\n\n\tType of data:          TimeDelta", "\n\tContains null values:  ", std::boolalpha, o.nulls() > 0
                , (o.nulls() ? " (excluded from calculations)\n" : "\n"), "\tNon-null values:       ", o.non_nulls()
                , "\n\tUnique values:         ", o.uniques()
                , "\n\tSmallest value:        ", csv_co::time_storage().str(o.r->smallest_value)
                , "\n\tLargest value:         ", csv_co::time_storage().str(o.r->largest_value)
                , "\n\tSum:                   ", csv_co::time_storage().str(o.r->sum)
                , "\n\tMean:                  ", csv_co::time_storage().str(o.r->mean));
        to_stream(std::cout, "\n\tMost common values:    ", mcv(o, standard_none_print, standard_space_print, prep),'\n');
    }

    template<class T>
    void standard_print_visitor::operator()(text_class<T> const & o) const {
        prep->print_col_header(o);
        to_stream(std::cout, "\n\n\tType of data:          Text", "\n\tContains null values:  ", std::boolalpha, o.nulls() > 0
                  , (o.nulls() ? " (excluded from calculations)\n" : "\n"), "\tNon-null values:       ", o.non_nulls()
                  , "\n\tUnique values:         ", o.uniques(), "\n\tLongest value:         ", o.r->longest_value, " characters"
                  , "\n\tMost common values:    ", mcv(o, standard_none_print, standard_space_print, prep), '\n');
    }

    template<class T>
    void standard_print_visitor::operator()(date_class<T> const & o) const {
        prep->print_col_header(o);
        to_stream(std::cout, "\n\n\tType of data:          Date", "\n\tContains null values:  ", std::boolalpha, o.nulls() > 0
                  , (o.nulls() ? " (excluded from calculations)\n" : "\n"), "\tNon-null values:       ", o.non_nulls()
                  , "\n\tUnique values:         ", o.uniques(), "\n\tSmallest value:        ", o.r->smallest_value
                  , "\n\tLargest value:         ", o.r->largest_value, "\n\tMost common values:    ",  mcv(o, standard_none_print, standard_space_print, prep), '\n');
    }

    template<class T>
    void standard_print_visitor::operator()(datetime_class<T> const & o) const {
        prep->print_col_header(o);
        to_stream(std::cout, "\n\n\tType of data:          DateTime", "\n\tContains null values:  ", std::boolalpha, o.nulls() > 0
                  , (o.nulls() ? " (excluded from calculations)\n" : "\n"), "\tNon-null values:       ", o.non_nulls()
                  , "\n\tUnique values:         ", o.uniques(), "\n\tSmallest value:        ", o.r->smallest_value
                  , "\n\tLargest value:         ", o.r->largest_value, "\n\tMost common values:    ",  mcv(o, standard_none_print, standard_space_print, prep), '\n');
    }

    struct csv_print_visitor::rep {
        using visitor_type = csv_print_visitor;
        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, number_class<T> const & o) const {
            oss.imbue(std::locale("C"));
            tune_format(oss, o.args().get().decimal_format.c_str());
            oss << csv_mcv_prec(elem.first);
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, bool_class<T> const &) const {
            oss.imbue(std::locale(oss.getloc(), new custom_boolean_facet));
            oss << std::boolalpha << elem.first;
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, timedelta_class<T> const &) const {
            oss << csv_co::time_storage().str(elem.first);
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, text_class<T> const &) const {
            if (elem.first.find('"') != std::string::npos) {
#if 0
                oss << std::quoted(elem.first,'"','"');
#endif
                oss << std::quoted(elem.first); 
            }
            else
                oss << elem.first;
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, date_class<T> const &) const { 
            oss << date_s(elem.first);
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, datetime_class<T> const &) const { 
            oss << datetime_s(elem.first);
        }

        void print_col_header(auto const & o) {
            std::cout << o.phys_index() + 1 << ',' << o.column_name() << ',';
        }
    };

    csv_print_visitor::csv_print_visitor(auto const & args) : prep{std::make_shared<rep>()} {
        static struct call_once {
            explicit call_once(std::decay_t<decltype(args)> const & a) {
                std::cout << "column_id,column_name,type,nulls,nonnulls,unique,min,max,sum,mean,median,stdev,len,";
                if (!a.no_mdp)
                    std::cout << "maxprecision,";
                std::cout << "freq\n";
            }
        } call_once_(args);
        tune_ostream(std::cout, args);
    }

    auto csv_none_print = [](std::ostringstream & os, auto const & o) {
        os << R"(None)";
    };

    auto csv_space_print = [](std::ostringstream & os, auto & first_printed) {
        os << (first_printed++ ? ", " : "");
    };                                           

    template<class T>
    void csv_print_visitor::operator()(number_class<T> const & o) const {
        prep->print_col_header(o);
        if (std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',' or std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep() == ',') {
            to_stream(std::cout, "Number,", std::boolalpha, o.nulls() > 0, ',');
            to_stream(std::cout, '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',');
            to_stream(std::cout, '"', spec_prec(o.r->smallest_value), '"', ',');
            to_stream(std::cout, '"', spec_prec(o.r->largest_value), '"', ',');
            to_stream(std::cout, '"', spec_prec(o.r->sum), '"', ',');
            to_stream(std::cout, '"', spec_prec(o.r->mean), '"', ',');
            to_stream(std::cout, '"', spec_prec(o.r->median), '"', ',');
            if (!std::isnan(o.r->stdev))
                to_stream(std::cout, '"', spec_prec(o.r->stdev), '"');
        } else {
            to_stream(std::cout, "Number,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',');
            to_stream(std::cout, spec_prec(o.r->smallest_value), ',');
            to_stream(std::cout, spec_prec(o.r->largest_value), ',');
            to_stream(std::cout, spec_prec(o.r->sum), ',');
            to_stream(std::cout, spec_prec(o.r->mean), ',');
            to_stream(std::cout, spec_prec(o.r->median), ',');
            if (!std::isnan(o.r->stdev))
                to_stream(std::cout, spec_prec(o.r->stdev));
        }

        to_stream(std::cout, ",,");
        if (!o.args().get().no_mdp)
            to_stream(std::cout, o.r->mdp, ',');
        to_stream(std::cout, std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
    }

    template<class T>
    void csv_print_visitor::operator()(bool_class<T> const & o) const {
        prep->print_col_header(o);
        if (std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',' or std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep() == ',') {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "Boolean,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"',o.uniques(), '"', ',', ",,,,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)),'\n');
            else
                to_stream(std::cout, "Boolean,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"',o.uniques(), '"', ',', ",,,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)),'\n');
        }
        else {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "Boolean,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', ",,,,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
            else
                to_stream(std::cout, "Boolean,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', ",,,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
        }
    }

    template<class T>
    void csv_print_visitor::operator()(timedelta_class<T> const & o) const {
        prep->print_col_header(o);
        if (std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',' or std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep() == ',') {
            to_stream(std::cout, "TimeDelta,", std::boolalpha, o.nulls() > 0, ',');
            to_stream(std::cout, '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',');
            to_stream(std::cout, '"', csv_co::time_storage().str(o.r->smallest_value), '"', ',');
            to_stream(std::cout, '"', csv_co::time_storage().str(o.r->largest_value), '"', ',');
            to_stream(std::cout, '"', csv_co::time_storage().str(o.r->sum), '"', ',');
            to_stream(std::cout, '"', csv_co::time_storage().str(o.r->mean), '"', ',');
        } else {
            to_stream(std::cout, "TimeDelta,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',');
            to_stream(std::cout, csv_co::time_storage().str(o.r->smallest_value), ',');
            to_stream(std::cout, csv_co::time_storage().str(o.r->largest_value), ',');
            to_stream(std::cout, csv_co::time_storage().str(o.r->sum), ',');
            to_stream(std::cout, csv_co::time_storage().str(o.r->mean), ',');
        }
        to_stream(std::cout, ",,,");
        if (!o.args().get().no_mdp)
            to_stream(std::cout, ',');
        to_stream(std::cout, std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
    }

    template<class T>
    void csv_print_visitor::operator()(text_class<T> const & o) const {
        prep->print_col_header(o);
        if (std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',' or std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep() == ',') {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "Text,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',', ",,,,,,", o.r->longest_value, ",,", '"', mcv(o, csv_none_print, csv_space_print, prep), '"', '\n');
            else
                to_stream(std::cout, "Text,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',', ",,,,,,", o.r->longest_value, ",", '"', mcv(o, csv_none_print, csv_space_print, prep), '"', '\n');
        }
        else {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "Text,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', ",,,,,,", o.r->longest_value, ",,", '"', mcv(o, csv_none_print, csv_space_print, prep), '"', '\n');
            else
                to_stream(std::cout, "Text,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', ",,,,,,", o.r->longest_value, ",", '"', mcv(o, csv_none_print, csv_space_print, prep), '"', '\n');
        }
    }

    template<class T>
    void csv_print_visitor::operator()(date_class<T> const & o) const {
        prep->print_col_header(o);
        if (std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',' or std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep() == ',') {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "Date,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
            else
                to_stream(std::cout, "Date,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
        }
        else {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "Date,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)),'\n');
            else
                to_stream(std::cout, "Date,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)),'\n');
        }
    }

    template<class T>
    void csv_print_visitor::operator()(datetime_class<T> const & o) const {
        prep->print_col_header(o);
        if (std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',' or std::use_facet<std::numpunct<char>>(std::locale()).thousands_sep() == ',') {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "DateTime,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
            else
                to_stream(std::cout, "DateTime,", std::boolalpha, o.nulls() > 0, ',', '"', o.non_nulls(), '"', ',', '"', o.uniques(), '"', ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)), '\n');
        }
        else {
            if (!o.args().get().no_mdp)
                to_stream(std::cout, "DateTime,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)),'\n');
            else
                to_stream(std::cout, "DateTime,", std::boolalpha, o.nulls() > 0, ',', o.non_nulls(), ',', o.uniques(), ',', o.r->smallest_value, ',', o.r->largest_value, ',', ",,,,,", std::quoted(mcv(o, csv_none_print, csv_space_print, prep)),'\n');
        }
    }

    struct json_print_visitor::rep {
        using visitor_type = json_print_visitor;
        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, number_class<T> const & o) const {
            oss.imbue(std::locale("C"));
            oss.precision(decimal_precision);
            inc_indent();
            to_stream(oss, add_indent(), '{');
            inc_indent();
            to_stream(oss, add_indent(), R"("value": )", elem.first, ',');
            to_stream(oss, add_indent(), (indent() >= 0 ? R"("count": )" : R"( "count": )"), elem.second);
            dec_indent();
            to_stream(oss, add_indent(), '}');
            dec_indent();
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, bool_class<T> const &) const {
            oss.imbue(std::locale("C"));
            inc_indent();
            to_stream(oss, add_indent(), '{');
            inc_indent();
            to_stream(oss, add_indent(), R"("value": )", std::boolalpha, elem.first, ',');
            to_stream(oss, add_indent(), (indent() >= 0 ? R"("count": )" : R"( "count": )"), elem.second);
            dec_indent();
            to_stream(oss, add_indent(), '}');
            dec_indent();
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, timedelta_class<T> const &) const {
            oss.imbue(std::locale("C"));
            oss.precision(decimal_precision);
            inc_indent();
            to_stream(oss, add_indent(), '{');
            inc_indent();
            to_stream(oss, add_indent(), R"("value": )", std::quoted(csv_co::time_storage().str(elem.first)), ',');
            to_stream(oss, add_indent(), (indent() >= 0 ? R"("count": )" : R"( "count": )"), elem.second);
            dec_indent();
            to_stream(oss, add_indent(), '}');
            dec_indent();
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, text_class<T> const &) const {
            oss.imbue(std::locale("C"));
            oss.precision(decimal_precision);
            inc_indent();
            to_stream(oss, add_indent(), '{');
            inc_indent();
            to_stream(oss, add_indent(), R"("value": )", std::quoted(elem.first), ',');
            to_stream(oss, add_indent(), (indent() >= 0 ? R"("count": )" : R"( "count": )"), elem.second);
            dec_indent();
            to_stream(oss, add_indent(), '}');
            dec_indent();
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, date_class<T> const &) const {
            oss.imbue(std::locale("C"));
            oss.precision(decimal_precision);
            inc_indent();
            to_stream(oss, add_indent(), '{');
            inc_indent();
            to_stream(oss, add_indent(), R"("value": )", std::quoted(date_s(elem.first)), ',');
            to_stream(oss, add_indent(), (indent() >= 0 ? R"("count": )" : R"( "count": )"), elem.second);
            dec_indent();
            to_stream(oss, add_indent(), '}');
            dec_indent();
        }

        template <class T>
        auto to_strm(std::ostringstream & oss, auto const & elem, datetime_class<T> const &) const {
            oss.imbue(std::locale("C"));
            oss.precision(decimal_precision);
            inc_indent();
            to_stream(oss, add_indent(), '{');
            inc_indent();
            to_stream(oss, add_indent(), R"("value": )", std::quoted(datetime_s_json(elem.first)), ',');
            to_stream(oss, add_indent(), (indent() >= 0 ? R"("count": )" : R"( "count": )"), elem.second);
            dec_indent();
            to_stream(oss, add_indent(), '}');
            dec_indent();
        }

        [[nodiscard]] int indent() const {
            return indenter->get_indent();
        }
        [[nodiscard]] std::string add_indent() const {
            return indenter->add_indent();
        }

        void inc_indent() const {
            indenter->inc_indent();
        }

        void dec_indent() const {
            indenter->dec_indent();
        }

    private:
        unsigned cur_col;
        std::size_t col_num;
        unsigned decimal_precision;
        std::unique_ptr<json_indenter> indenter;
        [[nodiscard]] std::string add_lf() const {
            return indenter->add_lf();
        }
        friend json_print_visitor;
    };

    json_print_visitor::json_print_visitor(auto const & args, std::size_t col_num, unsigned dec_prec, bool is_first_col) : prep{std::make_shared<rep>()} {
        prep->col_num = col_num;
        prep->decimal_precision = dec_prec;
        prep->indenter = std::move(std::make_unique<json_indenter>(args.indent)); 
        static struct call_once {
            explicit call_once(unsigned dec_prec) {
                std::cout.imbue(std::locale("C"));
                std::cout.precision(dec_prec);
            }
        } call_once_(dec_prec);

        if (is_first_col)
            std::cout << "[";

        std::cout << prep->add_indent();
        std::cout << "{";
    }

    json_print_visitor::~json_print_visitor() {
        std::cout << prep->add_indent();
        std::cout << "}";
        if (prep->cur_col != prep->col_num-1)
            std::cout << ", ";
        else {
#if 0
            prep->dec_indent();
            std::cout << prep->add_indent() << "]";
#endif
            std::cout << prep->add_lf();
            std::cout << "]";
        }
    }

    auto json_none_print = [](std::ostringstream & os, auto const & o, auto const & json_rep) {
        json_rep->inc_indent();
        to_stream(os, json_rep->add_indent(), '{');
        json_rep->inc_indent();
        to_stream(os, json_rep->add_indent(), R"("value": null)", ',');
        to_stream(os, json_rep->add_indent(), (o.args().get().indent >= 0 ? R"("count": )" : R"( "count": )"), o.nulls());
        json_rep->dec_indent();
        to_stream(os, json_rep->add_indent(), '}');
        json_rep->dec_indent();
    };

    auto json_space_print = [](std::ostringstream & os, auto & first_printed) {
        os << (first_printed++ ? ", " : "");
    };

    template<class T>
    void json_print_visitor::operator()(number_class<T> const & o) const {
        prep->cur_col = o.column();
        prep->inc_indent();
        to_stream(std::cout
                  , prep->add_indent(), R"("column_id": )", o.phys_index() + 1, ", "
                  , prep->add_indent(), R"("column_name": )", std::quoted(o.column_name()), ", "
                  , prep->add_indent(), R"("type": "Number")", ", "
                  , prep->add_indent(), R"("nulls": )", std::boolalpha, o.nulls() > 0, ", "
                  , prep->add_indent(), R"("nonnulls": )", o.non_nulls(), ", "
                  , prep->add_indent(), R"("unique": )", o.uniques(), ", "
                  , prep->add_indent(), R"("min": )", o.r->smallest_value, ", "
                  , prep->add_indent(), R"("max": )", o.r->largest_value, ", "
                  , prep->add_indent(), R"("sum": )", o.r->sum, ", "
                  , prep->add_indent(), R"("mean": )", o.r->mean, ", "
                  , prep->add_indent(), R"("median": )", o.r->median, ", ");
        if (!std::isnan(o.r->stdev))
            to_stream(std::cout, prep->add_indent(), R"("stdev": )", o.r->stdev, ", ");
        if (!o.args().get().no_mdp)
            to_stream(std::cout, prep->add_indent(), R"("maxprecision": )", o.r->mdp, ", ");

        to_stream(std::cout, prep->add_indent(), R"("freq": )", '[', mcv(o, json_none_print, json_space_print, prep), prep->add_indent(), ']');
        prep->dec_indent();
    }

    template<class T>
    void json_print_visitor::operator()(bool_class<T> const & o) const {
        prep->cur_col = o.column();
        prep->inc_indent();
        to_stream(std::cout
                  , prep->add_indent(), R"("column_id": )", o.phys_index() + 1, ", "
                  , prep->add_indent(), R"("column_name": )", std::quoted(o.column_name()), ", "
                  , prep->add_indent(), R"("type": "Boolean")", ", "
                  , prep->add_indent(), R"("nulls": )", std::boolalpha, o.nulls() > 0, ", "
                  , prep->add_indent(), R"("nonnulls": )", o.non_nulls(), ", "
                  , prep->add_indent(), R"("unique": )", o.uniques(), ", "
                  , prep->add_indent(), R"("freq": )", '[', mcv(o, json_none_print, json_space_print, prep), prep->add_indent(), ']');
        prep->dec_indent();
    }

    template<class T>
    void json_print_visitor::operator()(timedelta_class<T> const & o) const {
        prep->cur_col = o.column();
        prep->inc_indent();
        to_stream(std::cout
                , prep->add_indent(), R"("column_id": )", o.phys_index() + 1, ", "
                , prep->add_indent(), R"("column_name": )", std::quoted(o.column_name()), ", "
                , prep->add_indent(), R"("type": "TimeDelta")", ", "
                , prep->add_indent(), R"("nulls": )", std::boolalpha, o.nulls() > 0, ", "
                , prep->add_indent(), R"("nonnulls": )", o.non_nulls(), ", "
                , prep->add_indent(), R"("unique": )", o.uniques(), ", "
                , prep->add_indent(), R"("min": )", std::quoted(csv_co::time_storage().str(o.r->smallest_value)), ", "
                , prep->add_indent(), R"("max": )", std::quoted(csv_co::time_storage().str(o.r->largest_value)), ", "
                , prep->add_indent(), R"("sum": )", std::quoted(csv_co::time_storage().str(o.r->sum)), ", "
                , prep->add_indent(), R"("mean": )", std::quoted(csv_co::time_storage().str(o.r->mean)), ", "
                , prep->add_indent(), R"("freq": )", '[', mcv(o, json_none_print, json_space_print, prep), prep->add_indent(), ']');
        prep->dec_indent();
    }

    template<class T>
    void json_print_visitor::operator()(text_class<T> const & o) const {
        prep->cur_col = o.column();
        prep->inc_indent();
        to_stream(std::cout
                  , prep->add_indent(), R"("column_id": )", o.phys_index() + 1, ", "
                  , prep->add_indent(), R"("column_name": )", std::quoted(o.column_name()), ", "
                  , prep->add_indent(), R"("type": "Text")", ", "
                  , prep->add_indent(), R"("nulls": )", std::boolalpha, o.nulls() > 0, ", "
                  , prep->add_indent(), R"("nonnulls": )", o.non_nulls(), ", "
                  , prep->add_indent(), R"("unique": )", o.uniques(), ", "
                  , prep->add_indent(), R"("len": )", o.r->longest_value, ", "
                  , prep->add_indent(), R"("freq": )", '[', mcv(o, json_none_print, json_space_print, prep), prep->add_indent(), ']');
        prep->dec_indent();
    }

    template<class T>
    void json_print_visitor::operator()(date_class<T> const & o) const {
        prep->cur_col = o.column();
        prep->inc_indent();
        to_stream(std::cout
                  , prep->add_indent(), R"("column_id": )", o.phys_index() + 1, ", "
                  , prep->add_indent(), R"("column_name": )", std::quoted(o.column_name()), ", "
                  , prep->add_indent(), R"("type": "Date")", ", "
                  , prep->add_indent(), R"("nulls": )", std::boolalpha, o.nulls() > 0, ", "
                  , prep->add_indent(), R"("nonnulls": )", o.non_nulls(), ", "
                  , prep->add_indent(), R"("unique": )", o.uniques(), ", "
                  , prep->add_indent(), R"("min": )", std::quoted(o.r->smallest_value), ", "
                  , prep->add_indent(), R"("max": )", std::quoted(o.r->largest_value), ", "
                  , prep->add_indent(), R"("freq": )", '[', mcv(o, json_none_print, json_space_print, prep), prep->add_indent(), ']');
        prep->dec_indent();
    }

    template<class T>
    void json_print_visitor::operator()(datetime_class<T> const & o) const {
        prep->cur_col = o.column();
        prep->inc_indent();
        to_stream(std::cout
                , prep->add_indent(), R"("column_id": )", o.phys_index() + 1, ", "
                , prep->add_indent(), R"("column_name": )", std::quoted(o.column_name()), ", "
                , prep->add_indent(), R"("type": "DateTime")", ", "
                , prep->add_indent(), R"("nulls": )", std::boolalpha, o.nulls() > 0, ", "
                , prep->add_indent(), R"("nonnulls": )", o.non_nulls(), ", "
                , prep->add_indent(), R"("unique": )", o.uniques(), ", "
                , prep->add_indent(), R"("min": )", std::quoted(o.r->smallest_value), ", "
                , prep->add_indent(), R"("max": )", std::quoted(o.r->largest_value), ", "
                , prep->add_indent(), R"("freq": )", '[', mcv(o, json_none_print, json_space_print, prep), prep->add_indent(), ']');
        prep->dec_indent();
    }
} ///namespace
