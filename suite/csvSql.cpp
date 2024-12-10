//
// Created by wiluite on 7/30/24.
//

#include <soci/soci.h>

#include <ocilib.hpp>

using namespace ocilib;

#include <cli.h>
#include <sql_utils/rowset-query-impl.h>
#include <sql_utils/local-sqlite3-dep.h>

// TODO:
//  2. get rid of blanks calculations in typify() for csvsql  (--no-constraints mode)
//  3. implement null_value (while printing results?) (see how this is done in original utility)

using namespace ::csvsuite::cli;

namespace csvsql::detail {
    struct Args final : ARGS_positional_files {
        std::string & num_locale = kwarg("L,locale","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool &blanks = flag("blanks", R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> &null_value = kwarg("null-value","Convert these values to NULL.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        std::string & dialect = kwarg("i,dialect","Dialect of SQL {mysql,postgresql,sqlite,firebird,oracle} to generate. Cannot be used with --db or --query. ").set_default(std::string(""));
        std::string & db = kwarg("db","If present, a 'soci' connection string to use to directly execute generated SQL on a database.").set_default(std::string(""));
        std::string & query = kwarg("query","Execute one or more SQL queries delimited by --sql-delimiter, and output the result of the last query as CSV. QUERY may be a filename. --query may be specified multiple times.").set_default(std::string(""));
        bool &insert = flag("insert", "Insert the data into the table. Requires --db.");
        std::string & prefix = kwarg("prefix","Add an expression following the INSERT keyword, like OR IGNORE or OR REPLACE.").set_default(std::string(""));
        std::string & before_insert = kwarg("before-insert","Before the INSERT command, execute one or more SQL queries delimited by --sql-delimiter. Requires --insert.").set_default(std::string(""));
        std::string & after_insert = kwarg("after-insert","After the INSERT command, execute one or more SQL queries delimited by --sql-delimiter. Requires --insert.").set_default(std::string(""));
        std::string & sql_delimiter = kwarg("sql-delimiter","Delimiter separating SQL queries in --query, --before-insert, and --after-insert.").set_default(std::string(";"));
        std::string & tables = kwarg("tables","A comma-separated list of names of tables to be created. By default, the tables will be named after the filenames without extensions or \"stdin\".").set_default(std::string(""));
        bool &no_constraints = flag("no-constraints", "Generate a schema without length limits or null checks. Useful when sampling big tables.");
        std::string & unique_constraint = kwarg("unique-constraint","A comma-separated list of names of columns to include in a UNIQUE constraint").set_default(std::string(""));

        bool &no_create = flag("no-create", "Skip creating the table. Requires --insert.");
        bool &create_if_not_exists = flag("create-if-not-exists", "Create the table if it does not exist, otherwise keep going. Requires --insert.");
        bool &overwrite = flag("overwrite", "Drop the table if it already exists. Requires --insert. Cannot be used with --no-create.");
#if 0
        std::string & db_schema = kwarg("db-schema","Optional name of database schema to create table(s) in.").set_default(std::string(""));
#endif
        bool &no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        unsigned & chunk_size = kwarg("chunk-size","Chunk size for batch insert into the table. Requires --insert.").set_default(0);
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);

        void welcome() final {
            std::cout << "\nGenerate SQL statements for one or more CSV files, or execute those statements directly on a database, and execute one or more SQL queries.\n\n";
        }
    };

    auto sql_split = [](std::stringstream && strm, char by = ';') {
        std::vector<std::string> result;
        for (std::string phrase; std::getline(strm, phrase, by);)
            result.push_back(phrase);
        return result;
    };

    template <typename ... r_types>
    struct readers_manager {
        auto & get_readers() {
            return readers;
        }
        template<typename ReaderType>
        auto set_readers(auto & args) {
            if (args.files.empty())
                args.files = std::vector<std::string>{"_"};
            for (auto & elem : args.files) {
                auto reader {elem != "_" ? ReaderType{std::filesystem::path{elem}} : (elem = "stdin", ReaderType{read_standard_input(args)})};
                recode_source(reader, args);
                skip_lines(reader, args);
                quick_check(reader, args);
                get_readers().push_back(std::move(reader));
            }
        }
    private:
        std::deque<r_types...> readers;
    };

    auto create_table_phrase(auto const & args) {
        return !args.create_if_not_exists ? std::string_view("CREATE TABLE ") : std::string_view("CREATE TABLE IF NOT EXISTS ");
    }

    class create_table_composer {
        friend void reset_environment();
        static inline std::size_t file_no;
        class print_director {
            static inline std::stringstream stream;
            struct bool_column_tag {};
            struct number_column_tag {};
            struct datetime_column_tag {};
            struct date_column_tag {};
            struct timedelta_column_tag {};
            struct text_column_tag {};

            class printer {
            public:
                virtual void print_name(std::string const & name) = 0;
                virtual void print(bool_column_tag) = 0;
                virtual void print(bool_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(number_column_tag) = 0;
                virtual void print(number_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(datetime_column_tag) = 0;
                virtual void print(datetime_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(date_column_tag) = 0;
                virtual void print(date_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(timedelta_column_tag) = 0;
                virtual void print(timedelta_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(text_column_tag) = 0;
                virtual void print(text_column_tag, bool blanks, unsigned prec) = 0;
                virtual ~printer() = default;
            protected:
                static void print_name_or_quoted_name(std::string const & name, char qch) {
                    if (std::all_of(name.cbegin(), name.cend(), [&](auto & ch) {
                        return ((std::isalpha(ch) and std::islower(ch))
                                or (std::isdigit(ch) and std::addressof(ch) != std::addressof(name.front()))
                                or ch == '_');
                    }))
                        stream << name;
                    else
                        stream << std::quoted(name, qch, qch);
                    stream << " ";
                }
            }; // printer

            struct varchar_precision {};
            struct generic_printer : printer {
                void print_name(std::string const & name) override { print_name_or_quoted_name(name, '"'); }
                void print(bool_column_tag) override { to_stream(stream, "BOOLEAN"); }
                void print(bool_column_tag, bool blanks, unsigned) override { to_stream(stream, "BOOLEAN", (blanks ? "" : " NOT NULL")); }
                void print(number_column_tag) override { to_stream(stream, "DECIMAL"); }
                void print(number_column_tag, bool blanks, unsigned) override { to_stream(stream, "DECIMAL", (blanks ? "" : " NOT NULL")); }
                void print(datetime_column_tag) override { to_stream(stream, "TIMESTAMP"); }
                void print(datetime_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP"); }
                void print(date_column_tag) override { to_stream(stream, "DATE"); }
                void print(date_column_tag, bool blanks, unsigned) override { to_stream(stream, "DATE", (blanks ? "" : " NOT NULL")); }
                void print(timedelta_column_tag) override { to_stream(stream, "DATETIME"); }
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "DATETIME", (blanks ? "" : " NOT NULL")); }
                void print(text_column_tag) override { to_stream(stream, "VARCHAR"); }
                void print(text_column_tag, bool blanks, unsigned) override { to_stream(stream, "VARCHAR", (blanks ? "" : " NOT NULL")); }
            };

            struct mysql_printer : generic_printer, varchar_precision {
                void print_name(std::string const & name) override {
                    print_name_or_quoted_name(name, '`');
                }
                void print(datetime_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP", (blanks ? " NULL DEFAULT NULL" : " NOT NULL")); }
                void print(bool_column_tag) override { to_stream(stream, "BOOL"); }
                void print(bool_column_tag, bool blanks, unsigned) override { to_stream(stream, "BOOL", (blanks ? "" : " NOT NULL")); }
                void print(number_column_tag, bool blanks, unsigned prec) override { to_stream(stream, "DECIMAL(38, ", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL")); }
                void print(text_column_tag) override { throw std::runtime_error("VARCHAR requires a length on dialect mysql"); }
                void print(text_column_tag, bool blanks, unsigned prec) override { to_stream(stream, "VARCHAR(", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL")); }
            };

            struct mariadb_printer : mysql_printer {
            };

            struct postgresql_printer : generic_printer {
                void print(datetime_column_tag) override { to_stream(stream, "TIMESTAMP WITHOUT TIME ZONE"); }
                void print(datetime_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP WITHOUT TIME ZONE"); }
                void print(timedelta_column_tag) override { to_stream(stream, "INTERVAL"); }
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "INTERVAL", (blanks ? "" : " NOT NULL")); }
            };

            struct sqlite_printer : generic_printer {
                void print(number_column_tag) override { to_stream(stream, "FLOAT"); }
                void print(number_column_tag, bool blanks, unsigned) override { to_stream(stream, "FLOAT", (blanks ? "" : " NOT NULL")); }
            };

            struct firebird_printer : generic_printer {
                void print(datetime_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP", (blanks ? "" : " NOT NULL")); }
                void print(timedelta_column_tag) override { to_stream(stream, "TIMESTAMP");}
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP", (blanks ? "" : " NOT NULL"));}
#if 0
                void print(text_column_tag) override { throw std::runtime_error("VARCHAR requires a length on dialect firebird");}
                void print(text_column_tag, bool blanks, unsigned) override { throw std::runtime_error("VARCHAR requires a length on dialect firebird");}
#endif
                void print(text_column_tag) override { to_stream(stream, "VARCHAR(200)"); }
                void print(text_column_tag, bool blanks, unsigned) override { to_stream(stream, "VARCHAR(200)", (blanks ? "" : " NOT NULL")); }
            };

            struct oracle_printer : generic_printer {
                void print_name(std::string const & name) override {
                    if (name == "date" or name == "integer" or name == "float")
                        stream << std::quoted(name, '"', '"');
                    else
                        generic_printer::print_name_or_quoted_name(name, '"');
                }
                void print(number_column_tag, bool blanks, unsigned prec) override { to_stream(stream, "DECIMAL(38, ", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL")); }
                void print(timedelta_column_tag) override { to_stream(stream, "INTERVAL DAY TO SECOND"); }
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "INTERVAL DAY TO SECOND", (blanks ? "" : " NOT NULL")); }
                void print(text_column_tag) override { to_stream(stream, "VARCHAR(200)"); }
                void print(text_column_tag, bool blanks, unsigned) override { to_stream(stream, "VARCHAR(200)", (blanks ? "" : " NOT NULL")); }
            };

            using printer_dialect = std::string;
            static inline std::unordered_map<printer_dialect, std::shared_ptr<printer>> printer_map {
                  {"mysql", std::make_shared<mysql_printer>()}
                , {"postgresql", std::make_shared<postgresql_printer>()}
                , {"sqlite", std::make_shared<sqlite_printer>()}
                , {"firebird", std::make_shared<firebird_printer>()}
                , {"oracle", std::make_shared<oracle_printer>()}
                , {"generic", std::make_shared<generic_printer>()}
                , {"mariadb", std::make_shared<mariadb_printer>()}
            };

            static inline std::unordered_map<column_type
                    , std::variant<bool_column_tag, number_column_tag, datetime_column_tag, date_column_tag
                    , timedelta_column_tag, text_column_tag>> tag_map {
                  {column_type::bool_t, bool_column_tag{}}
                , {column_type::bool_t, bool_column_tag()}
                , {column_type::number_t, number_column_tag()}
                , {column_type::datetime_t, datetime_column_tag()}
                , {column_type::date_t, date_column_tag()}
                , {column_type::timedelta_t, timedelta_column_tag()}
                , {column_type::text_t, text_column_tag()}
            };

        public:
            void direct(auto & reader, typify_with_precisions_result const & value, std::string const & dialect, auto const & header) {
                auto [types, blanks, precisions] = value;
                auto index = 0ull;

                // re-fill precisions with varchar lengths if needed

                if (dynamic_cast<varchar_precision*>(printer_map[dialect].get())) {
                    using reader_type = std::decay_t<decltype(reader)>;
                    using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                    reader.run_rows([&](auto & row_span) {
                        auto col = 0ull;
                        for (auto & elem : row_span) {
                            if (types[col] == column_type::text_t) {
                                auto const candidate_size = elem_type{elem}.str_size_in_symbols();
                                if (candidate_size > precisions[col])
                                    precisions[col] = candidate_size;
                            }
                            col++;
                        }
                    });
                }
                for (auto e : types) {
                    printer_map[dialect]->print_name(header[index].operator csv_co::cell_string());
                    std::visit([&](auto & arg) {
                        printer_map[dialect]->print(arg, blanks[index], precisions[index]);
                    }, tag_map[e]);
                    ++index;
                    stream << (index != types.size() ? ",\n\t" :(unique_constraint.empty() ? "\n" : (",\n\tUNIQUE (" + unique_constraint + ")\n")));
                }
            }

            void direct(auto&, typify_without_precisions_and_blanks_result const & value, std::string const & dialect, auto const & header) {
                auto [types] = value;
                auto index = 0ull;
                for (auto e : types) {
                    printer_map[dialect]->print_name(header[index].operator csv_co::cell_string());
                    std::visit([&](auto & arg) {
                        printer_map[dialect]->print(arg);
                    }, tag_map[e]);
                    ++index;
                    stream << (index != types.size() ? ",\n\t" : (unique_constraint.empty() ? "\n" : (",\n\tUNIQUE (" + unique_constraint + ")\n")));
                }
            }

            void direct(auto&, typify_without_precisions_result const &, std::string const &, auto const &) {}

            static std::string table() {
                return stream.str();
            }

            struct open_close {
                open_close(auto const & args, std::vector<std::string> const & table_names) {
                    stream.str({});
                    stream.clear();
                    stream << create_table_phrase(args);
                    std::string tn;
                    if (table_names.size() > file_no)
                        tn = table_names[file_no];
                    else
                        tn = std::filesystem::path{args.files[file_no]}.stem().string();
                    csv_co::string_functions::unquote(tn, '"');
                    stream << tn;
                    stream << " (\n\t";
                }
                ~open_close() {
                    stream << ");\n";
                }
            } wrapper_;
            std::string const & unique_constraint;
            print_director(auto const & args, std::vector<std::string> const & table_names)
                : wrapper_(args, table_names),  unique_constraint(args.unique_constraint) {}
        };

    private:
        [[nodiscard]] std::string dialect(auto const & args) const {
            if (args.db.empty() and args.dialect.empty())
                 return "generic";
            if (!args.dialect.empty())
                 return args.dialect;
            std::vector<std::string> dialects {"mysql", "postgresql", "sqlite", "firebird", "oracle", "mariadb"};
            for (auto elem : dialects) {
                if (args.db.find(elem) != std::string::npos)
                    return elem;
            }
            return "generic";
        }
        std::vector<std::string> header_;
        std::vector<column_type> types_;
        std::vector<unsigned char> blanks_;
    public:
        create_table_composer(auto & reader, auto const & args, std::vector<std::string> const & table_names) {

            auto const info = typify(reader, args, !args.no_constraints ? typify_option::typify_with_precisions : typify_option::typify_without_precisions_and_blanks);
            skip_lines(reader, args);
            auto const header = obtain_header_and_<skip_header>(reader, args);
            header_ = header_to_strings<csv_co::unquoted>(header);

            print_director print_director_(args, table_names);
            std::visit([&](auto & arg) {
                types_ = std::get<0>(arg);
                if constexpr(std::is_same_v<std::decay_t<decltype(arg)>, typify_with_precisions_result>)
                    blanks_ = std::get<1>(arg);
                print_director_.direct(reader, arg, dialect(args), header);
            }, info);

            ++file_no;
        }

        [[nodiscard]] static std::string table() {
            return print_director::table();
        }

        [[nodiscard]] auto const & header() const { return header_; }
        [[nodiscard]] auto const & types() const { return types_; }
        [[nodiscard]] auto const & blanks() const { return blanks_; }
    };

    [[nodiscard]] std::string insert_expr(std::string const & insert_prefix_, auto const & args) {
        constexpr const std::array<std::string_view, 1> sql_keywords {{"UNIQUE"}};
        auto const t = create_table_composer::table();
        std::string expr = "insert" + (insert_prefix_.empty() ? "": " " + insert_prefix_) + " into ";
        auto const expr_prim_size = create_table_phrase(args).size();

        auto pos = t.find('(', expr_prim_size);
        expr += std::string(t.begin() + expr_prim_size, t.begin() + static_cast<std::ptrdiff_t>(pos) + 1);
        std::string values;
        unsigned counter = 0;
        while (pos = t.find('\t', pos + 1), pos != std::string::npos) {
            using diff_t = std::ptrdiff_t;
            auto find_bs = [&t](auto from_pos) {
                bool return_flag = true;
                for(;;) {
                    auto const next_char = *(t.begin() + from_pos);
                    if (next_char == '"')
                        return_flag = !return_flag;
                    if (next_char == ' ' and return_flag)
                        return from_pos;
                    from_pos++;
                }
            };
            std::string next = {t.begin() + static_cast<diff_t>(pos) + 1, t.begin() + static_cast<diff_t>(find_bs(pos + 1))};
            bool keyword = false;
            for (auto e : sql_keywords) {
                if (e == next) {
                    keyword = true;
                    break;
                }
            }
            if (keyword)
                continue;
            expr += next + ",";
            values += ":v" + std::to_string(counter++) + ',';
        }
        expr.back() = ')';
        expr += " values (" + values;
        expr.back() = ')';
        return expr;
    }

    static auto queries(auto const & args) {
        std::stringstream queries;
        if (std::filesystem::exists(std::filesystem::path{args.query})) {
            std::filesystem::path path{args.query};
            auto const length = std::filesystem::file_size(path);
            if (length == 0)
                throw std::runtime_error("Query file '" + args.query +"' exists, but it is empty.");
            std::ifstream f(args.query);
            if (!(f.is_open()))
                throw std::runtime_error("Error opening the query file: '" + args.query + "'.");
            std::string queries_s;
            for (std::string line; std::getline(f, line, '\n');)
                queries_s += line;
            queries << recode_source(std::move(queries_s), args);
        } else
            queries << args.query;
        return sql_split(std::move(queries), args.sql_delimiter[0]);
    }

    // quote or unquote a cell while storing it in DB.
    std::string clarify_text(auto const & e) {
        return (e.raw_string_view().find(',') == std::string_view::npos) ? std::string(e.str()) : e;
    }

    #include "src/csvsql/csvsql_soci.cpp"
    #include "src/csvsql/csvsql_ocilib.cpp"

    void reset_environment() {
        create_table_composer::file_no = 0;
    }

#if !defined(__unix__)
    static local_sqlite3_dependency lsd;
#endif

    struct dbms_client
    {
        virtual ~dbms_client() = default;
        virtual void task() = 0;
        virtual void querying() = 0;
    };

    template <class ReaderType2, class Args2>
    struct soci_client : dbms_client {
        soci_client(readers_manager<ReaderType2> & r_man, Args2 & args, std::vector<std::string> const & table_names)
        : r_man(r_man), args(args), table_names(table_names) {
            if (args.db.empty())
                this->args.db = "sqlite3://db=:memory:";
            session = std::make_unique<soci::session>(this->args.db);
        }
        void task() override {
            using namespace soci_client_ns;
            for (auto & reader : r_man.get_readers()) {
                try {
                    create_table_composer composer(reader, args, table_names);
                    table_creator{args, *session};
                    if (args.insert or (args.db == "sqlite3://db=:memory:" and !args.query.empty()))
                        table_inserter(args, *session, composer).insert(args, reader);
                } catch(std::exception & e) {
                    if (std::string(e.what()).find("Vain to do next actions") != std::string::npos)
                        continue;
                    throw;
                }
            }
        }
        void querying() override {
            using namespace soci_client_ns;
            query {args, *session};
        }
        static std::shared_ptr<dbms_client> create(readers_manager<ReaderType2> & r_man, Args2 & args, std::vector<std::string> const & table_names) {
            return std::make_shared<soci_client>(r_man, args, table_names);
        }
    private:
        readers_manager<ReaderType2> & r_man;
        Args2 & args;
        std::vector<std::string> const & table_names;
        std::unique_ptr<soci::session> session;
    };

    struct OracleConnection : Connection {
        explicit OracleConnection(std::string const & args_db) : Connection() {
            using namespace std::literals;
            std::string_view sv = "oracle://service="sv;
            assert(args_db.starts_with(sv));

            char service[128];
            char usr[128];
            char pwd[128];
            auto count = sscanf(args_db.c_str() + sv.size(), "%s user=%s password=%s", service, usr, pwd);
            if (count != 3)
                throw std::runtime_error("Error parsing " + args_db + " for ocilib!");
            Connection::Open(service, usr, pwd);
            Connection::SetAutoCommit(true);
        }
    };
    template <class ReaderType2, class Args2>
    struct ocilib_client : dbms_client {
        struct env_init_cleanup {
            env_init_cleanup() { Environment::Initialize(); }
            ~env_init_cleanup() { Environment::Cleanup(); }
        };

        ocilib_client(readers_manager<ReaderType2> & r_man, Args2 & args, std::vector<std::string> const & table_names)
        : r_man(r_man), args(args), table_names(table_names) {
            con = std::make_unique<OracleConnection>(args.db);
        }
        void task() override {
            using namespace ocilib_client_ns;
            for (auto & reader : r_man.get_readers()) {
                try {
                    create_table_composer composer(reader, args, table_names);
                    table_creator{args, *con};
                    if (args.insert)
                        table_inserter(args, *con, composer).insert(args, reader);
                } catch(std::exception & e) {
                    if (std::string(e.what()).find("Vain to do next actions") != std::string::npos)
                        continue;
                    throw;
                }
            }
        }

        void querying() override {
            using namespace ocilib_client_ns;
            query {args, *con};
        }
        static std::shared_ptr<dbms_client> create(readers_manager<ReaderType2> & r_man, Args2 & args, std::vector<std::string> const & table_names) {
            return std::make_shared<ocilib_client>(r_man, args, table_names);
        }
    private:
        readers_manager<ReaderType2> & r_man;
        Args2 & args;
        std::vector<std::string> const & table_names;
        env_init_cleanup e_i_c;
        std::unique_ptr<Connection> con;
    };

    template <class ReaderType2, class Args2>
    class dbms_client_factory
    {
    public:
        using CreateCallback = std::function<std::shared_ptr<dbms_client>(readers_manager<ReaderType2> &, Args2 &, std::vector<std::string> const &)>;

        static void register_client(const std::string &type, CreateCallback cb) {
            clients[type] = cb;
        }
        static void unregister_client(const std::string &type) {
            clients.erase(type);
        }
        static std::shared_ptr<dbms_client> create_client(const std::string &type, readers_manager<ReaderType2> & r_man, Args2 & args, std::vector<std::string> const & table_names) {
            auto it = clients.find(type);
            if (it != clients.end())
                return (it->second)(r_man, args, table_names);
            return nullptr;
        }

    private:
        using CallbackMap = std::map<std::string, CreateCallback>;
        static inline CallbackMap clients;
    };

} ///detail

namespace csvsql {
    template <typename ReaderType>
    void sql(auto & args) {

        using namespace detail;
        if (args.files.empty() and isatty(STDIN_FILENO))
            throw std::runtime_error("csvsql: error: You must provide an input file or piped data.");

        std::vector<std::string> table_names;
        if (!args.tables.empty())
            table_names = [&] {
                std::istringstream stream(args.tables);
                std::vector<std::string> result;
                for (std::string word; std::getline(stream, word, ',');)
                    result.push_back(word != "_" ? word : "stdin");
                return result;
            }();

        if (!args.dialect.empty()) {
            std::vector<std::string_view> svv{"mysql", "postgresql", "sqlite", "firebird", "oracle"};
            if (!std::any_of(svv.cbegin(), svv.cend(), [&](auto elem){ return elem == args.dialect;}))
                throw std::runtime_error("csvsql: error: argument -i/--dialect: invalid choice: '" + args.dialect + "' (choose from 'mysql', 'postgresql', 'sqlite', 'firebird', 'oracle').");
            if (!args.db.empty() or !args.query.empty())
                throw std::runtime_error("csvsql: error: The --dialect option is only valid when neither --db nor --query are specified.");
        }

        if (args.insert and args.db.empty() and args.query.empty())
            throw std::runtime_error("csvsql: error: The --insert option is only valid when either --db or --query is specified.");

        auto need_insert = [](char const * const option) {
            return std::string("csvsql: error: The ") + option + " option is only valid if --insert is also specified.";
        };

        if (!args.before_insert.empty() and !args.insert)
            throw std::runtime_error(need_insert("--before-insert"));
        if (!args.after_insert.empty() and !args.insert)
            throw std::runtime_error(need_insert("--after-insert"));
        if (args.no_create and !args.insert)
            throw std::runtime_error(need_insert("--no-create option"));
        if (args.create_if_not_exists and !args.insert)
            throw std::runtime_error(need_insert("--create-if-not-exists option"));
        if (args.overwrite and !args.insert)
            throw std::runtime_error(need_insert("--overwrite"));
        if (args.chunk_size and !args.insert and args.query.empty())
            throw std::runtime_error(need_insert("--chunk-size"));

        // TODO: FixMe in the future.
        if (args.create_if_not_exists and args.db.find("firebird") != std::string::npos)
            throw std::runtime_error("Sorry, we do not support the --create-if-not-exists option for Firebird DBMS client.");

        reset_environment();
        readers_manager<ReaderType> r_man;
        r_man.template set_readers<ReaderType>(args);

        if (args.db.empty() and args.query.empty()) {
            for (auto & r : r_man.get_readers()) {
                create_table_composer composer (r, args, table_names);
                std::cout << create_table_composer::table();
            }
            return;
        }

        using args_type = std::decay_t<decltype(args)>;
        using dbms_client_factory_type = dbms_client_factory<ReaderType, args_type>;
        dbms_client_factory_type::register_client("soci", soci_client<ReaderType, args_type>::create);
        dbms_client_factory_type::register_client("ocilib", ocilib_client<ReaderType, args_type>::create);

        std::string which_one_to_create = args.db.find("oracle://service=") == std::string::npos ? "soci" : "ocilib";
        auto client = dbms_client_factory_type::create_client(which_one_to_create, r_man, args, table_names);
        client->task();
        client->querying();
    }

}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {
    auto args = argparse::parse<csvsql::detail::Args>(argc, argv);

    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        if (!args.skip_init_space)
            csvsql::sql<notrimming_reader_type>(args);
        else
            csvsql::sql<skipinitspace_reader_type>(args);
    }
    catch (soci::soci_error const & e) {
        std::cout << e.get_error_message() << std::endl;
    }
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
}

#endif
