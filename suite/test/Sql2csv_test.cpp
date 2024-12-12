///
/// \file   test/Sql2csv_test.cpp
/// \author wiluite
/// \brief  Tests for the Sql2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../Sql2csv.cpp"
#include "../csvSql.cpp"
#include "common_args.h"
#include "strm_redir.h"
#include "stdin_subst.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call)    \
    std::stringstream cout_buffer;              \
    {                                           \
        redirect(cout)                          \
        redirect_cout cr(cout_buffer.rdbuf());  \
        call;                                   \
    }

int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    struct sql2csv_specific_args {
        std::filesystem::path query_file;
        std::string db;
        std::string query;
        bool linenumbers {false};
        std::string encoding {"UTF-8"};
        bool no_header {false};
    };

    "encoding"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                encoding = "latin1";
                query_file = "test.sql";
            }
        } args;

        expect(nothrow([&] {CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))}));
    };

    "query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query = "select 6*9 as question";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))

        expect(cout_buffer.str().find("question") != std::string::npos);
        expect(cout_buffer.str().find("54") != std::string::npos);
    };

    "file"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))

        expect(cout_buffer.str().find("question") != std::string::npos);
        expect(cout_buffer.str().find("36") != std::string::npos);
    };

    "file with query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
                query = "select 6*9 as question";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))

        expect(cout_buffer.str().find("question") != std::string::npos);
        expect(cout_buffer.str().find("54") != std::string::npos);
    };

    "stdin"_test = [] {
        struct Args : sql2csv_specific_args {
            //NOTE: we do not use any option, only piped text: "select cast(3.1415 * 13.37 as integer) as answer"
            Args() = default;
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
            expect(cout_buffer.str().find("answer") != std::string::npos);
            expect(cout_buffer.str().find("42") != std::string::npos);
        }));
    };

    "stdin with query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query = "select 6*9 as question";
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
            expect(cout_buffer.str().find("question") != std::string::npos);
            expect(cout_buffer.str().find("54") != std::string::npos);
        }));
    };

    "stdin with file"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
            expect(cout_buffer.str().find("question") != std::string::npos);
            expect(cout_buffer.str().find("36") != std::string::npos);
        }));
    };

    "stdin with file and query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
                query = "select 6*9 as question";
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
            expect(cout_buffer.str().find("question") != std::string::npos);
            expect(cout_buffer.str().find("54") != std::string::npos);
        }));
    };

    // Load test data into the DB and return the CSV's file contents as a string for comparison.
    class db_file {
        std::string file;
    public:
        explicit db_file(char const * const name = "foo2.db") : file(name) {}
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

    auto csvsql = [&](db_file & dbfile, char const * const csv_file) {
        namespace tf = csvsuite::test_facilities;
        struct csvsql_specific_args {
            std::vector<std::string> files;
            std::string dialect;
            std::string db;
            std::string query;
            bool insert {false};
            std::string prefix;
            std::string before_insert;
            std::string after_insert;
            std::string sql_delimiter=";";
            std::string tables;
            bool no_constraints {false};
            std::string unique_constraint;
            bool no_create {false};
            bool create_if_not_exists {false};
            bool overwrite {false};
            std::string schema {false};
            unsigned chunk_size {0};
            bool check_integrity = {true};
        };
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args(db_file const & dbfile, char const * const csv_file) {
                db = "sqlite3://db=" + dbfile();
                files = std::vector<std::string>{csv_file};
                tables = "foo";
                insert = true;
                no_inference = true;
            }
        } args(dbfile, csv_file);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        std::ifstream f(csv_file);
        if (!f)
            throw std::runtime_error("Error opening the file: '" + std::string(csv_file) + "'.");
        std::ostringstream oss;
        oss << f.rdbuf();
        return oss.str();
    };

    "unicode"_test = [&csvsql] {
        db_file dbfile;
        auto expected = csvsql(dbfile, "test_utf8.csv");
        struct Args : sql2csv_specific_args {
            explicit Args(db_file & dbfile) {
                db = "sqlite3://db=" + dbfile();
                query = "select * from foo";
            }
        } args(dbfile);
        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
        expect(std::equal(expected.cbegin(), expected.cend(), cout_buffer.str().cbegin()));
    };

    "no header row"_test = [&csvsql] {
        db_file dbfile;
        auto expected = csvsql(dbfile, "dummy.csv");
        struct Args : sql2csv_specific_args {
            explicit Args(db_file & dbfile) {
                db = "sqlite3://db=" + dbfile();
                query = "select * from foo";
                no_header = true;
            }
        } args(dbfile);
        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
        expect(cout_buffer.str().find("a,b,c") == std::string::npos);
        expect(cout_buffer.str().find("1,2,3") != std::string::npos);
    };

    "line numbers"_test = [&csvsql] {
        db_file dbfile;
        auto expected = csvsql(dbfile, "dummy.csv");
        struct Args : sql2csv_specific_args {
            explicit Args(db_file & dbfile) {
                db = "sqlite3://db=" + dbfile();
                query = "select * from foo";
                linenumbers = true;
            }
        } args(dbfile);
        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
        expect(cout_buffer.str().find("line_number,a,b,c") != std::string::npos);
        expect(cout_buffer.str().find("1,1,2,3") != std::string::npos);
    };

    "wildcard on sqlite"_test = [&csvsql] {
        db_file dbfile;
        auto expected = csvsql(dbfile, "iris.csv");
        struct Args : sql2csv_specific_args {
            explicit Args(db_file & dbfile) {
                db = "sqlite3://db=" + dbfile();
                query = "select * from foo where species LIKE '%'";
            }
        } args(dbfile);
        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql2csv(args))
        expect(cout_buffer.str().find("sepal_length,sepal_width,petal_length,petal_width,species") != std::string::npos);
        expect(cout_buffer.str().find("5.1,3.5,1.4,0.2,Iris-setosa") != std::string::npos);
    };
}
