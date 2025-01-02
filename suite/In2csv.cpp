///
/// \file   suite/In2csv.cpp
/// \author wiluite
/// \brief  Convert common, but less awesome, tabular data formats to CSV.

#include <cli.h>
#include "include/in2csv/in2csv_fixed.h"
#include "include/in2csv/in2csv_dbf.h"
#include "include/in2csv/in2csv_xls.h"
#include "include/in2csv/in2csv_geojson.h"
#include "include/in2csv/in2csv_csv.h"
#include "include/in2csv/in2csv_xlsx.h"
#include "include/in2csv/in2csv_json.h"
#include "include/in2csv/in2csv_ndjson.h"

using namespace ::csvsuite::cli;

namespace in2csv::detail {

    struct Args final : ARGS_positional_1_any_format {
        std::string & num_locale = kwarg("L,locale","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool & blanks = flag("blanks",R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> & null_value = kwarg("null-value", "Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format","Specify a strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format","Specify a strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        std::string & format = kwarg("f,format","The format {csv,dbf,fixed,geojson,json,ndjson,xls,xlsx} of the input file. If not specified will be inferred from the file type.").set_default(std::string{});
        std::string & schema = kwarg("s,schema","Specify a CSV-formatted schema file for converting fixed-width files. See In2csv_test as example.").set_default(std::string{});
        std::string & key = kwarg("k,key","Specify a top-level key to look within for a list of objects to be converted when processing JSON.").set_default(std::string{});
        bool & names = flag ("n,names","Display sheet names from the input Excel file.");
        std::string & sheet = kwarg("sheet","The name of the Excel sheet to operate on.").set_default(std::string{});
        std::string & write_sheets = kwarg("write-sheets","The names of the Excel sheets to write to files, or \"-\" to write all sheets.").set_default(std::string{});
        bool & use_sheet_names = flag ("use-sheet-names","Use the sheet names as file names when --write-sheets is set.");
#if 0
        bool reset_dimensions //Ignore the sheet dimensions provided by the XLSX file.
#endif
        std::string & encoding_xls = kwarg("encoding-xls","Specify the encoding of the input XLS file.").set_default(std::string{"UTF-8"});
        std::string & d_excel = kwarg("d-excel","A comma-separated list of numeric columns of the input XLS/XLSX/CSV source, considered as dates, e.g. \"1,id,3-5\".").set_default("none");
        std::string & dt_excel = kwarg("dt-excel","A comma-separated list of numeric columns of the input XLS/XLSX/CSV source, considered as datetimes, e.g. \"1,id,3-5\".").set_default("none");

        bool & is1904 = flag("is1904", "Epoch based on the 1900/1904 datemode for input XLSX source, or for the input CSV source, converted from XLS/XLSX.").set_default(true);

        bool & no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format, --no-leading-zeroes) when parsing the input.");
        bool & date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);
        bool & asap = flag("ASAP","Print result output stream as soon as possible.").set_default(true);

        void welcome() final {
            std::cout << "\nConvert common, but less awesome, tabular data formats to CSV.\n\n";
        }
    };
    struct empty_file_and_no_piping_now : std::runtime_error {
        empty_file_and_no_piping_now() : std::runtime_error("in2csv: error: You must provide an input file or piped data.") {}
    };
    struct no_format_specified_on_piping : std::runtime_error {
        no_format_specified_on_piping() : std::runtime_error("in2csv: error: You must specify a format when providing input as piped data via STDIN.") {}
    };
    struct no_schema_when_no_extension : std::runtime_error {
        no_schema_when_no_extension() : std::runtime_error("ValueError: schema must not be null when format is \"fixed\".") {}
    };
    struct unable_to_automatically_determine_format : std::runtime_error {
        unable_to_automatically_determine_format() : std::runtime_error("in2csv: error: Unable to automatically determine the format of the input file. Try specifying a format with --format.") {}
    };
    struct invalid_input_format : std::runtime_error {
        explicit invalid_input_format(std::string const & f)
        : std::runtime_error(std::string("in2csv: error: argument -f/--format: invalid choice: ") + '\''
            + f + '\'' + " (choose from 'csv', 'dbf', 'fixed', 'geojson', 'json', 'ndjson', 'xls', 'xlsx')") {}
    };
    struct schema_file_not_found : std::runtime_error {
        explicit schema_file_not_found(std::string const & f) : std::runtime_error(std::string("FileNotFoundError: No such file or directory: ") + '\'' + f +'\'') {}
    };
    struct file_not_found : schema_file_not_found {
        using schema_file_not_found::schema_file_not_found;
    };
    struct cannot_use_names_for_non_excels : std::runtime_error {
        cannot_use_names_for_non_excels() : std::runtime_error("in2csv: error: You cannot use the -n or --names options with non-Excel files.") {}
    };

    std::array<std::string, 8> formats {"csv", "dbf", "fixed", "geojson", "json", "ndjson", "xls", "xlsx" };

    std::string guess_format(std::string const & extension) {
        auto ext = extension.empty() ? "fixed" : std::string{extension.cbegin() + 1, extension.cend()};
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        if (std::find(formats.cbegin(), formats.cend(), ext) != formats.cend()) {
            if (ext == "csv" or ext == "dbf" or ext == "fixed" or ext == "xls" or ext == "xlsx")
                return ext;
            if (ext == "json" or ext == "js")
                return "json";
        }
        return {};
    }

    template <class Args2>
    class dbms_client_factory
    {
    public:
        using CreateCallback = std::function<std::shared_ptr<converter_client>(Args2 &)>;

        static void register_client(const std::string &type, CreateCallback cb) {
            clients[type] = cb;
        }
        static void unregister_client(const std::string &type) {
            clients.erase(type);
        }
        static std::shared_ptr<converter_client> create_client(const std::string &type, Args2 & args) {
            auto it = clients.find(type);
            if (it != clients.end())
                return (it->second)(args);
            return nullptr;
        }

    private:
        using CallbackMap = std::map<std::string, CreateCallback>;
        static inline CallbackMap clients;
    };

}
namespace in2csv {
    void in2csv(auto &args) {
        using namespace detail;
        namespace fs = std::filesystem;

        // no file, no piped data
        if (args.file.empty() and isatty(STDIN_FILENO) and args.format.empty())
            throw empty_file_and_no_piping_now();

        std::string filetype;
        if (!args.format.empty()) {
            if (std::find(formats.cbegin(), formats.cend(), args.format) == formats.cend())
                throw invalid_input_format(args.format);
            filetype = args.format;
        } else {
            if (!args.schema.empty())
                filetype = "fixed";
            else if (!args.key.empty())
                filetype = "json";
            else {
                if (args.file.empty() or args.file == "_") { // piped data, but no format specified
                    assert (args.format.empty());
                    assert(!isatty(STDIN_FILENO));
                    throw no_format_specified_on_piping();
                }
                filetype = guess_format(fs::path(args.file).extension().string());
                if (filetype.empty())
                    throw unable_to_automatically_determine_format();
            }
        }
        assert(!filetype.empty());
        if (args.names) {
            if (filetype != "xls" and filetype != "xlsx")
                throw cannot_use_names_for_non_excels();
        }
        if (filetype == "fixed") {
            if (args.schema.empty())
                throw no_schema_when_no_extension();
            if (!std::filesystem::exists(fs::path{args.schema}))
                throw schema_file_not_found(args.schema);
        }
        if (filetype == "dbf" and (args.file.empty() or args.file == "_"))
            throw dbf_cannot_be_converted_from_stdin();

        if (!args.file.empty() and args.file != "_") {
            if (!std::filesystem::exists(fs::path{args.file}))
                throw file_not_found(args.file.string());
        }

        using args_type = std::decay_t<decltype(args)>;
        using dbms_client_factory_type = dbms_client_factory<args_type>;
        dbms_client_factory_type::register_client("csv", csv_client<args_type>::create);
        dbms_client_factory_type::register_client("dbf", dbf_client<args_type>::create);
        dbms_client_factory_type::register_client("fixed", fixed_client<args_type>::create);
        dbms_client_factory_type::register_client("geojson", geojson_client<args_type>::create);
        dbms_client_factory_type::register_client("json", json_client<args_type>::create);
        dbms_client_factory_type::register_client("ndjson", ndjson_client<args_type>::create);
        dbms_client_factory_type::register_client("xls", xls_client<args_type>::create);
        dbms_client_factory_type::register_client("xlsx", xlsx_client<args_type>::create);

        auto client = dbms_client_factory_type::create_client(filetype, args);
        client->convert();
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {
    auto args = argparse::parse<in2csv::detail::Args>(argc, argv);

    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        in2csv::in2csv(args);
    }
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
}

#endif
