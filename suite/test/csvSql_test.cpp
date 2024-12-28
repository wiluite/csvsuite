///
/// \file   test/csvSql_test.cpp
/// \author wiluite
/// \brief  Tests for the csvSql utility.

#define BOOST_UT_DISABLE_MODULE
#include <utility>

#include "ut.hpp"

#include "../csvSql.cpp"
#include "../Sql2csv.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "stdin_subst.h"
#include "test_max_field_size_macros.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call)    \
    std::stringstream cout_buffer;              \
    {                                           \
        redirect(cout)                          \
        redirect_cout cr(cout_buffer.rdbuf());  \
        call;                                   \
    }


using namespace boost::ut;
namespace tf = csvsuite::test_facilities;

class SQL2CSV {
public:
    SQL2CSV(std::string const & db, std::string const & query) {
        sql2csvargs.db = db;
        sql2csvargs.query = query;
    }
    SQL2CSV& call() {
        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(sql2csvargs))
            output = cout_buffer.str();
        }));
        return *this;
    }
    [[nodiscard]] std::string cout_buffer() const {
        return output;
    }
private:
    struct sql2csv_specific_args {
        std::filesystem::path query_file;
        std::string db;
        std::string query;
        bool linenumbers {false};
        std::string encoding {"UTF-8"};
        bool no_header {false};
    } sql2csvargs;

    std::string output;
};

enum class dbms_client{
    SOCI,
    OCILIB
};

template<dbms_client client = dbms_client::SOCI>
struct table_dropper {
    table_dropper(std::string db, char const * const table_name) : db(std::move(db)), table_name(table_name) {}

    ~table_dropper() {
        try {
            if constexpr(client == dbms_client::SOCI) {
                soci::session sql (db);
                sql << "DROP TABLE " << table_name;
            } else {
                struct env_init_cleanup {
                    env_init_cleanup() { Environment::Initialize(); }
                    ~env_init_cleanup() { Environment::Cleanup(); }
                } eic;
                csvsql::detail::OracleConnection con(db);
                Statement stmt(con);
                stmt.Execute("DROP TABLE " + table_name);
            }
        } catch (...) {
            // there was no corresponding backend at all earlier. Or, no Oracle runtime found (oci.dll etc.) (in case of ocilib)
        }
    }
private:
    std::string const db;
    std::string table_name;
};

std::string get_db_conn(char const * const env_var_name) {
    std::string db_conn;
    if (char const * env_p = getenv(env_var_name)) {
        db_conn = env_p;
        if (!db_conn.empty()) {
            // Well, further: we must unquote the quoted env var, because the SOCI framework imply DB connection
            // string can not be quoted (otherwise, SOCI does not evaluate the backend to load, properly).
            csv_co::string_functions::unquote(db_conn, '"');
            // So, why an ENV VAR can be quoted? This is because there can be complicated things like:
            // "oracle://service=//127.0.0.1:1521/xepdb1 user=usr password=pswd", and getenv can't extract it as is
            // from the environment.
        }
    }
    return db_conn;
}

int main() {
#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    OutputCodePage65001 ocp65001;

    struct csvsql_specific_args {
        std::vector<std::string> files;
        std::string dialect;
        std::string db;
        std::string query;
        bool insert{false};
        std::string prefix;
        std::string before_insert;
        std::string after_insert;
        std::string sql_delimiter=";";
        std::string tables;
        bool no_constraints{false};
        std::string unique_constraint;
        bool no_create{false};
        bool create_if_not_exists{false};
        bool overwrite{false};
        std::string schema{false};
        unsigned chunk_size{0};
        bool check_integrity = {true};
    };

    "create table"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                tables = "foo";
                date_lib_parser = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 text VARCHAR NOT NULL,
 date DATE,
 integer DECIMAL,
 boolean BOOLEAN,
 float DECIMAL,
 time DATETIME,
 datetime TIMESTAMP,
 empty_column BOOLEAN
);
)");
    };

    "no blanks"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/blanks.csv"};
                tables = "foo";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a BOOLEAN,
 b BOOLEAN,
 c BOOLEAN,
 d BOOLEAN,
 e BOOLEAN,
 f BOOLEAN
);
)");
    };

    "blanks"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/blanks.csv"};
                tables = "foo";
                blanks = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a VARCHAR NOT NULL,
 b VARCHAR NOT NULL,
 c VARCHAR NOT NULL,
 d VARCHAR NOT NULL,
 e VARCHAR NOT NULL,
 f VARCHAR NOT NULL
);
)");
    };

    "no inference"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                tables = "foo";
                date_lib_parser = true;
                no_inference = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 text VARCHAR NOT NULL,
 date VARCHAR,
 integer VARCHAR,
 boolean VARCHAR,
 float VARCHAR,
 time VARCHAR,
 datetime VARCHAR,
 empty_column VARCHAR
);
)");
    };

    "no header row"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/no_header_row.csv"};
                tables = "foo";
                no_header = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a BOOLEAN NOT NULL,
 b DECIMAL NOT NULL,
 c DECIMAL NOT NULL
);
)");
    };

    "linenumbers"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/dummy.csv"};
                tables = "foo";
                linenumbers = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a BOOLEAN NOT NULL,
 b DECIMAL NOT NULL,
 c DECIMAL NOT NULL
);
)");
    };

    "stdin"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                tables = "foo";
                files = {"_"};
            }
        } args;

        std::istringstream iss("a,b,c\n4,2,3\n");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a DECIMAL NOT NULL,
 b DECIMAL NOT NULL,
 c DECIMAL NOT NULL
);
)");
    };

    "piped stdin"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() = default; // NOTE: now we do not use '_' placeholder to help. We read a csv file and pipe it with us.
        } args;

        stdin_redir sr("examples/piped_stdin");

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE stdin (
 a DATETIME NOT NULL
);
)");
    };

    "stdin and filename"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_", "examples/dummy.csv"};
            }
        } args;

        std::istringstream iss("a,b,c\n1,2,3\n");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str().find(R"(CREATE TABLE stdin)") != std::string::npos);
        expect(cout_buffer.str().find(R"(CREATE TABLE dummy)") != std::string::npos);
    };

    "query"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/iris.csv", "examples/irismeta.csv"};
                query = "SELECT m.usda_id, avg(i.sepal_length) AS mean_sepal_length FROM iris "
                        "AS i JOIN irismeta AS m ON (i.species = m.species) GROUP BY m.species";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        expect(cout_buffer.str().find("usda_id,mean_sepal_length") != std::string::npos);
        expect(cout_buffer.str().find("IRSE,5.00") != std::string::npos);
        expect(cout_buffer.str().find("IRVE2,5.936") != std::string::npos);
        expect(cout_buffer.str().find("IRVI,6.58") != std::string::npos);
    };

    "query empty"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                query = "SELECT 1";
            }
        } args;

        std::istringstream iss(" ");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == "1\n1\n");
    };

    "query text"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                query = "SELECT text FROM testfixed_converted WHERE text LIKE \"Chicago%\"";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == "text\nChicago Reader\nChicago Sun-Times\nChicago Tribune\n");
    };

    "query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                query = "examples/test_query.sql";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

//      question,text
//      36,©
        expect(cout_buffer.str() == "question,text\n36,©\n");
    };

    "no UTF-8 query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                query = "examples/test_query_1252.sql";
            }
        } args;

        expect(throws([&]{ CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) ) }));
    };

    "CP1252 query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                query = "examples/test_query_1252.sql";
                encoding = "CP1252";
            }
        } args;

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )

//          question,text
//          36,©
            expect(cout_buffer.str() == "question,text\n36,©\n");

        }));
    };

    "empty query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/testfixed_converted.csv"};
                query = "examples/test_query_empty.sql";
            }
        } args;

        try {
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
        } catch (std::exception const & e) {
            expect(std::string(e.what()).find("Query file 'examples/test_query_empty.sql' exists, but it is empty.") != std::string::npos);
        }
    };

    "query update"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"examples/dummy.csv"};
                no_inference = true;
                query = "UPDATE dummy SET a=10 WHERE a=1";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str().empty());
    };

    class db_file {
        std::string file;
    public:
        explicit db_file(char const * const name = "foo.db") : file(name) {}
        ~db_file() {
            if (std::filesystem::exists(std::filesystem::path(file)))
                std::remove(file.c_str());
        }
        std::string operator()() {
            return file;
        }
        std::string operator()() const {
            return file;
        }
    };

    "before and after insert"_test = [] {
        // Longer test
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"examples/dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                before_insert = "SELECT 1; CREATE TABLE foobar (date DATE)";
                after_insert = "INSERT INTO dummy VALUES (0, 5, 6.1)";
            }
            std::string str1;
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }

        expect(SQL2CSV{"sqlite3://db=" + args.dbfile(), "SELECT * FROM foobar"}.call().cout_buffer() == "date\n");
        expect(SQL2CSV{"sqlite3://db=" + args.dbfile(), "SELECT * from dummy"}.call().cout_buffer() == "a,b,c\ntrue,2,3\nfalse,5,6.1\n");
    };

    "no prefix unique constraint"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"examples/dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                unique_constraint = "a";
           }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }
        {   // now "unique constraint" exception
            args.no_create = true;
            bool catched = false;
            try {
                CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
            } catch(soci::soci_error const & e) {
                expect(std::string(e.what()).find("UNIQUE constraint failed: dummy.a") != std::string::npos);
                catched = true;
            }
            expect(catched);
        }
    };

    "prefix unique constraint"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"examples/dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                unique_constraint = "a";
           }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }

        args.no_create = true;
        args.prefix = "OR IGNORE";

        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args)) }));
    };

    "no create-if-not-exists"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"examples/foo1.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                tables = "foo";
            }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }

        args.files = {"examples/foo2.csv"};
        {
            bool catched = false;
            try {
                CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
            } catch(soci::soci_error const & e) {
                expect(std::string(e.what()).find("table foo already exists") != std::string::npos);
                catched = true;
            }
            expect(catched);
        }
    };

    "create-if-not-exists"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"examples/foo1.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                tables = "foo";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsql::sql<notrimming_reader_type>(args))

        args.files = {"examples/foo2.csv"};
        args.create_if_not_exists = true;
        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args)) }));
    };

    "batch_bulk_inserter"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 3;
            }
        } args;
        std::istringstream iss("a,b\n1,1972-04-14\n3,1973-05-15\n5,1974-06-16\n7, \n9,04/18/1976\n");
        stdin_subst new_cin(iss);
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        expect(cout_buffer.str() == "a,b\n1,1972-04-14\n3,1973-05-15\n5,1974-06-16\n7,\n9,1976-04-18\n");
    };

    "comma containing string cells"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
            }
        } args;
        std::istringstream iss("a,b\n1,\"normal string\"\n2,\"complex, string\"\n");
        stdin_subst new_cin(iss);
        CALL_TEST_AND_REDIRECT_TO_COUT (csvsql::sql<notrimming_reader_type>(args))
        expect(cout_buffer.str() == "a,b\n1,normal string\n2,\"complex, string\"\n");
    };

try {
#if defined(SOCI_HAVE_SQLITE3)
    "Sqlite3 date, datetime, timedelta"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 1; //bulk insert mode OFF.
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_SQLITE3");

        if (!db_conn.empty()) {
            args.db = db_conn;
            auto bs_pos = args.db.find(' ');
            if (bs_pos == std::string::npos)
                bs_pos = args.db.length();

            std::string filename = std::string(args.db.c_str() + args.db.find('=') + 1, args.db.c_str() + bs_pos);
            struct db_file_remover {
                explicit db_file_remover(std::string name) : name(std::move(name)){}

                ~db_file_remover() {
                    std::remove(name.c_str());
                }
                std::string name;
            } db_file_remover_ (filename);

            std::istringstream iss("a,b,c,d\n1971-01-01,1971-01-01T04:14:00,2 days 01:14:47.123,n/a\n");
            stdin_subst new_cin(iss);
            table_dropper td {db_conn, "stdin"};
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          a,b,c,d
//          1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.123000,
            expect(
                cout_buffer.str() == "a,b,c,d\n1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.123000,\n"
            or
                cout_buffer.str() == "a,b,c,d\n1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.122999,\n"
            );
        }
    };
#endif

//TODO: datetime type's NULLs MUST BE URGENTLY FIXED!
#if defined(SOCI_HAVE_MYSQL)
    "MySQL all types, bulk insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2; //bulk insert mode ON.
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_MYSQL");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss(
                    "a,b,c,d,e\n,1984-01-01T04:14:00,2 days 01:14:47.123,3.1415,T\n1972-01-01,1980-01-01T04:15:00,,,F\nN/A,N/A,3 days 01:14:47.123,,\n");
            stdin_subst new_cin(iss);
            table_dropper td{db_conn, "stdin"};
            CALL_TEST_AND_REDIRECT_TO_COUT(
                    csvsql::sql<notrimming_reader_type>(args)
            )

//          a,b,c,d,e
//          ,1984-01-01 04:14:00.000000,1970-01-03 01:14:47.000000,3.142,true
//          1972-01-01,1980-01-01 04:15:00,,,false
//          ,,1970-01-04 01:14:47.000000,,
            expect(cout_buffer.str() ==
                   "a,b,c,d,e\n,1984-01-01 04:14:00.000000,1970-01-03 01:14:47.000000,3.142,true\n1972-01-01,1980-01-01 04:15:00.000000,,,false\n,,1970-01-04 01:14:47.000000,,\n");
        }
    };
#endif

#if defined(SOCI_HAVE_MARIADB)
    "MariaDB all types, bulk insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_MARIADB");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss(
                    "a,b,c,d,e,f\n,1984-01-01T04:14:00,2 days 01:14:47.123,3.1415,T,str 1\n1972-01-01,1980-01-01T04:15:00,,,F,str 2\nN/A,N/A,3 days 01:14:47.123,,,\n");
            stdin_subst new_cin(iss);
            table_dropper td{db_conn, "stdin"};
            CALL_TEST_AND_REDIRECT_TO_COUT(
                    csvsql::sql<notrimming_reader_type>(args)
            )

//          a,b,c,d,e,f
//          ,1984-01-01 04:14:00.000000,1970-01-03 01:14:47.000000,3.142,true,str 1
//          1972-01-01,1980-01-01 04:15:00,,,false,str 2
//          ,,1970-01-04 01:14:47.000000,,,
            expect(cout_buffer.str() ==
                   "a,b,c,d,e,f\n,1984-01-01 04:14:00.000000,1970-01-03 01:14:47.000000,3.142,true,str 1\n1972-01-01,1980-01-01 04:15:00.000000,,,false,str 2\n,,1970-01-04 01:14:47.000000,,,\n");
        }
    };
#endif

#if defined(SOCI_HAVE_POSTGRESQL)
    "PostgreSQL date, datetime, timedelta, bulk insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_POSTGRESQL");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c,d,e,f\n ,1971-01-01T04:14:00,2 days 01:14:10.737,1,A,\n1972-01-01,n/a,3 days 01:14:10.737,,B,F\n1971-01-01,1920-01-01T04:14:00,,3,,T\n");
            stdin_subst new_cin(iss);
            table_dropper td {db_conn, "stdin"};

            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          a         ,        b                 ,   c           ,d,e,f
//                    ,1971-01-01 04:14:00.000000,49:14:10.736999,1,A,
//          1972-01-01,                          ,73:14:10.737   , ,B,false
//          1971-01-01,1920-01-01 04:14:00.000000,               ,3, ,true
            expect(cout_buffer.str() == "a,b,c,d,e,f\n,1971-01-01 04:14:00.000000,49:14:10.736999,1,A,\n1972-01-01,,73:14:10.737,,B,false\n1971-01-01,1920-01-01 04:14:00.000000,,3,,true\n");
        }
    };
#endif

#if defined(SOCI_HAVE_FIREBIRD)
    "Firebird date, datetime, timedelta, bulk insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_FIREBIRD");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c\n1971-01-01,1971-01-01T04:14:00,2 days 01:14:47.123\nn/a,n/a,n/a\n1972-01-01,1972-01-01T04:14:00,01:14:47.123\n");
            stdin_subst new_cin(iss);
            table_dropper td {db_conn, "stdin"};
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          A,B,C
//          1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.000000
//          ,,
//          1972-01-01,1971-01-01 04:14:00.000000,1972-01-01 01:14:47.000000
            expect(cout_buffer.str() == "A,B,C\n1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.000000\n,,\n1972-01-01,1972-01-01 04:14:00.000000,1970-01-01 01:14:47.000000\n");
        }
    };
#endif

#if defined(SOCI_HAVE_ORACLE)
    "Oracle all types and null values, non-bulk insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_ORACLE");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c,d,e\nstr 1,1071-01-01,,,3.1415\nstr 2,1072-02-01,,,\nstr 3,1073-03-01,1072-02-01 14:09:53,25 h 3.534sec,\n");
            stdin_subst new_cin(iss);
            table_dropper<dbms_client::OCILIB> td {db_conn, "stdin"};
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )

//          A,B,C,D,E
//          str 1,1071-01-01,,,3.142
//          str 2,1072-02-01,,,
//          str 3,1073-03-01,1072-02-01 14:09:53.000000,"1 day, 01:00:03.534000",
            expect(cout_buffer.str() == "A,B,C,D,E\nstr 1,1071-01-01,,,3.142\nstr 2,1072-02-01,,,\nstr 3,1073-03-01,1072-02-01 14:09:53.000000,\"1 day, 01:00:03.534000\",\n");
        }
    };
    "Oracle all types and null values, bulk insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
                datetime_fmt = R"(%m/%d/%Y %H:%M:%S)";
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_ORACLE");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c,d,e\n04/14/1955 08:08:08,,,,\n,1072-01-01,,,\n,,3.14,,\n,,,str 1,\n1075-01-01T09:09:09,,,,\n,,,,1h 1.5sec\n");

            stdin_subst new_cin(iss);
            table_dropper<dbms_client::OCILIB> td {db_conn, "stdin"};
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<skipinitspace_reader_type>(args) )

//          A,B,C,D,E
//          1955-04-14 08:08:08.000000,          ,    ,     ,
//                                    ,1072-01-01,    ,     ,
//                                    ,          ,3.14,     ,
//                                    ,          ,    ,str 1,
//          1075-01-01 09:09:09.000000,          ,    ,     ,
//                                    ,          ,    ,     ,01:00:01.500000
            expect(cout_buffer.str() == "A,B,C,D,E\n1955-04-14 08:08:08.000000,,,,\n,1072-01-01,,,\n,,3.14,,\n,,,str 1,\n1075-01-01 09:09:09.000000,,,,\n,,,,01:00:01.500000\n");
        }
    };
#endif

    "max field size"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() { files = {"examples/test_field_size_limit.csv"}; maxfieldsize = 100;}
        } args;

        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT(csvsql::sql<notrimming_reader_type>(args))}));

        using namespace z_test;
        Z_CHECK1(csvsql::sql<notrimming_reader_type>(args), skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK1(csvsql::sql<notrimming_reader_type>(args), skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

        Z_CHECK1(csvsql::sql<notrimming_reader_type>(args), skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK1(csvsql::sql<notrimming_reader_type>(args), skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

        Z_CHECK1(csvsql::sql<notrimming_reader_type>(args), skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        Z_CHECK1(csvsql::sql<notrimming_reader_type>(args), skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
    };

} catch (std::exception const & e) {
    std::cerr << e.what() << std::endl;
}
}
