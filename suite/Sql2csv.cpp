///
/// \file   utils/csvsuite/Sql2csv.cpp
/// \author wiluite
/// \brief  Execute an SQL query on a database and output the result to a CSV file.

#include <soci/soci.h>

#include <ocilib.hpp>

using namespace ocilib;

#include <cli.h>
#include <sql_utils/rowset-query-impl.h>
#include <sql_utils/local-sqlite3-dep.h>

using namespace ::csvsuite::cli;

namespace sql2csv::detail {
    struct Args : argparse::Args {
        std::filesystem::path & query_file = arg("The FILE to use as SQL query. If it and --query are omitted, the query is piped data via STDIN.").set_default("");
        bool & verbose = flag("v,verbose", "A flag to toggle verbose.");
        bool & linenumbers = flag("l,linenumbers", "Insert a column of line numbers at the front of the output. Useful when piping to grep or as a simple primary key.");
        std::string & db = kwarg("db","If present, a 'soci' connection string to use to directly execute generated SQL on a database.").set_default(std::string(""));
        std::string & query = kwarg("query","The SQL query to execute. Overrides FILE and STDIN.").set_default(std::string(""));
        std::string & encoding = kwarg("e,encoding","Specify the encoding of the input query file.").set_default("UTF-8");
        bool & no_header = flag("H,no-header-row","Do not output column names.");

        void welcome() final {
            std::cout << "\nExecute an SQL query on a database and output the result to a CSV file.\n\n";
        }
    };

    std::string & query_stdin() {
        static std::string query_stdin_;
        return query_stdin_;
    }

    auto sql_split = [](std::stringstream && strm, char by = ';') {
        bool only_one = false;
        std::string result_phrase;
        for (std::string phrase; std::getline(strm, phrase, by);) {
            if (!only_one) {
                only_one = true;
                result_phrase = std::move(phrase);
            } else
                throw std::runtime_error("Warning: You can only execute one statement at a time.");
        }
        return result_phrase;
    };

    static auto query_impl(auto const & args) {
        std::stringstream queries;
        if (!(args.query.empty())) {
            queries << args.query;
        } else if (!args.query_file.empty()) {
            if (std::filesystem::exists(std::filesystem::path{args.query_file})) {
                std::filesystem::path path{args.query_file};
                auto const length = std::filesystem::file_size(path);
                if (length == 0)
                    throw std::runtime_error("Query file '" + args.query_file.string() +"' exists, but it is empty.");
                std::ifstream f(args.query_file);
                if (!(f.is_open()))
                    throw std::runtime_error("Error opening the query file: '" + args.query_file.string() + "'.");
                std::string queries_s;
                for (std::string line; std::getline(f, line, '\n');)
                    queries_s += line;
                queries << recode_source(std::move(queries_s), args);
            } else
                throw std::runtime_error("No such file or directory: '" + args.query_file.string() + "'.");
        } else {
            assert(!query_stdin().empty());
            queries << query_stdin();
        }
        return sql_split(std::move(queries));
    }

#if !defined(__unix__)
    static local_sqlite3_dependency lsd;
#endif

    struct dbms_client
    {
        virtual ~dbms_client() = default;
        virtual void querying() = 0;
    };

    template <class Args2>
    struct soci_client : dbms_client {
        explicit soci_client(Args2 & args) : args(args) {
            if (args.db.empty())
                this->args.db = "sqlite3://db=:memory:";
            session = std::make_unique<soci::session>(this->args.db);
        }
        void querying() override {
            csvsuite::cli::sql::rowset_query(*session, args, query_impl(args));
        }
        static std::shared_ptr<dbms_client> create(Args2 & args) {
            return std::make_shared<soci_client>(args);
        }
    private:
        Args2 & args;
        std::unique_ptr<soci::session> session;
    };

    template <class Args2>
    struct ocilib_client : dbms_client {
        struct env_init_cleanup {
            env_init_cleanup() { Environment::Initialize(); }
            ~env_init_cleanup() { Environment::Cleanup(); }
        };

        explicit ocilib_client(Args2 & args) : args(args) {
            using namespace std::literals;
            std::string_view sv = "oracle://service="sv;
            assert(args.db.starts_with(sv));

            char service[128];
            char usr[128];
            char pwd[128];

            auto count = sscanf(args.db.c_str() + sv.size(), "%s user=%s password=%s", service, usr, pwd);
            if (count != 3)
                throw std::runtime_error("Error parsing " + args.db + " for ocilib!");

            con = std::make_unique<Connection>(service, usr, pwd);
            con->SetAutoCommit(true);
        }

        void querying() override {
            csvsuite::cli::sql::rowset_query(*con, args, query_impl(args));
        }
        static std::shared_ptr<dbms_client> create(Args2 & args) {
            return std::make_shared<ocilib_client>(args);
        }
    private:
        Args2 & args;
        env_init_cleanup e_i_c;
        std::unique_ptr<Connection> con;
    };

    template <class Args2>
    class dbms_client_factory
    {
    public:
        using CreateCallback = std::function<std::shared_ptr<dbms_client>(Args2 &)>;

        static void register_client(const std::string &type, CreateCallback cb) {
            clients[type] = cb;
        }
        static void unregister_client(const std::string &type) {
            clients.erase(type);
        }
        static std::shared_ptr<dbms_client> create_client(const std::string &type, Args2 & args) {
            auto it = clients.find(type);
            if (it != clients.end())
                return (it->second)(args);
            return nullptr;
        }

    private:
        using CallbackMap = std::map<std::string, CreateCallback>;
        static inline CallbackMap clients;
    };


} /// detail

namespace sql2csv {
    void sql2csv(auto &args) {
        using namespace detail;
        if (args.query.empty() and args.query_file.empty() and isatty(STDIN_FILENO))
            throw std::runtime_error("sql2csv: error: You must provide an input file or piped data.");

        if (args.query.empty()) {
            if ((args.query_file.empty() and !isatty(STDIN_FILENO)) or args.query_file == "_") {
                query_stdin() = read_standard_input(args);
                if (args.query_file == "_")
                    args.query_file.clear();
            }
        }

        using args_type = std::decay_t<decltype(args)>;
        using dbms_client_factory_type = dbms_client_factory<args_type>;
        dbms_client_factory_type::register_client("soci", soci_client<args_type>::create);
        dbms_client_factory_type::register_client("ocilib", ocilib_client<args_type>::create);

        std::string which_one_to_create = args.db.find("oracle://service=") == std::string::npos ? "soci" : "ocilib";
        auto client = dbms_client_factory_type::create_client(which_one_to_create, args);
        client->querying();
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {
    auto args = argparse::parse<sql2csv::detail::Args>(argc, argv);

    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        sql2csv::sql2csv(args);
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
