#include "../../include/in2csv/in2csv_xls.h"
#include <cli.h>
#include "../../external/libxls/include/xls.h"
#include <iostream>
#include "common_datetime_excel.h"
#include "common_time_point.h"
#include "common_excel.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::xls {

    static char stringSeparator = '\"';

    static void OutputHeaderString(std::ostringstream & oss, const char *string) {
        // now we have native header, and so "1" does not influence on the nature of this column
        can_be_number.push_back(1);
#if 0
        std::cerr << "OutputHeaderString\n";
#endif
        std::ostringstream header_cell;
        tune_format(header_cell, "%.16g");
#if 0
        header_cell << stringSeparator;
#endif
        for (const char * str = string; *str; str++) {
            if (*str == stringSeparator)
                header_cell << stringSeparator << stringSeparator;
            else
            if (*str == '\\')
                header_cell << "\\\\";
            else
                header_cell << *str;
        }
#if 0
        header_cell << stringSeparator;
#endif
        if (header_cell.str().empty()) {
            std::cerr << "UnnamedColumnWarning: Column " << header_cell_index << " has no name. Using " << '"' << letter_name(header_cell_index) << "\".\n"; 
            get_header().push_back(letter_name(header_cell_index));
        }
        else
            get_header().push_back(header_cell.str());
        oss << get_header().back();
        ++header_cell_index;
    }

    inline static void OutputString(std::ostringstream & oss, const char *string) {
        // now we have first line of the body, and so "0" really influence on the nature of this column
        if (can_be_number.size() < get_header().size())
            can_be_number.push_back(0);

        oss << stringSeparator;
        for (const char *str = string; *str; str++) {
            if (*str == stringSeparator)
                oss << stringSeparator << stringSeparator;
            else
            if (*str == '\\')
                oss << "\\\\";
            else
                oss << *str;
        }
        oss << stringSeparator;
    }

    void convert_impl(impl_args & a) {
        using namespace ::xls;
        xls_error_t error = LIBXLS_OK;

        class pwb_holder {
            std::shared_ptr<xlsWorkBook> ptr;
        public:
            pwb_holder(impl_args const & a, xls_error_t & err) : ptr (
                [&a, &err] {
                    if (a.file.empty() or a.file == "_") {
                        static std::string WB;
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                WB += static_cast<char>(r);
                            else
                                break;
                        }
                        return xls_open_buffer(reinterpret_cast<unsigned char const *>(&*WB.cbegin()), WB.length(), a.encoding_xls.c_str(), &err);
                    } else
                        return xls_open_file(a.file.string().c_str(), a.encoding_xls.c_str(), &err);
                }(),
                [&](xlsWorkBook* descriptor) { xls_close_WB(descriptor); }
            ) {}
            operator xlsWorkBook* () {
                return ptr.get();
            }
            xlsWorkBook* operator->() {
                return ptr.get();
            }
        } pwb(a, error);

        if (xls_parseWorkBook(pwb) != 0)
            throw std::runtime_error("Error parsing the XLS workbook source.");
        is1904 = pwb->is1904;

        if (!pwb)
            throw std::runtime_error(std::string(xls_getError(error)));

        auto fill_sheets = [&] {
            std::vector<std::string> result;
            for (auto i = 0u; i < pwb->sheets.count; i++) {
                if (!pwb->sheets.sheet[i].name)
                    continue;
                result.push_back(pwb->sheets.sheet[i].name);
            }
            return result;
        };

        if (a.names) {
            auto v = fill_sheets();
            for (auto const & e : v)
                std::cout << e << '\n';
            return;
        }

        if (a.sheet.empty())
            a.sheet = pwb->sheets.sheet[0].name;

        auto sheet_index_by_name = [&pwb](std::string const & name) {
            for (auto i = 0u; i < pwb->sheets.count; i++) {
                if (!pwb->sheets.sheet[i].name)
                    continue;
                if (strcmp(name.c_str(), (char *)pwb->sheets.sheet[i].name) == 0)
                    return static_cast<int>(i);
            }
            throw std::runtime_error(std::string("No sheet named ") + "'" + name + "'");
        };

        auto sheet_index = sheet_index_by_name(a.sheet);

        auto print_sheet = [&pwb](int sheet_idx, std::ostream & os, impl_args arguments, use_date_datetime_excel use_d_dt) {
            auto args (std::move(arguments));
            get_header().clear();
            header_cell_index = 0;
            can_be_number.clear();
            dates_ids.clear();
            datetimes_ids.clear();
            class pws_holder {
                std::shared_ptr<xlsWorkSheet> ptr;
            public:
                pws_holder(pwb_holder & pwb, int sheet_index) : ptr (
                        [&pwb, &sheet_index] {
                            return xls_getWorkSheet(pwb, sheet_index);
                        }(),
                        [&](xlsWorkSheet* descriptor) { xls_close_WS(descriptor); }
                ) {}
                operator xlsWorkSheet* () {
                    return ptr.get();
                }
                xlsWorkSheet* operator->() {
                    return ptr.get();
                }
            } pws(pwb, sheet_idx);

            // open and parse the sheet
            if (xls_parseWorkSheet(pws) != LIBXLS_OK)
                throw std::runtime_error("Error parsing the sheet. Index: " + std::to_string(sheet_idx));

            std::ostringstream oss;

            generate_and_print_header(oss, args, pws->rows.lastcol + 1, use_d_dt);

            tune_format(oss, "%.16g");

            for (auto j = args.skip_lines; j <= (unsigned int)pws->rows.lastrow; ++j) {
                auto cellRow = (unsigned int)j;
                if (j != args.skip_lines)
                    oss << '\n';

                if (j == args.skip_lines + 1 and !args.no_header) // now we have really the native header
                    get_date_and_datetime_columns(args, get_header(), use_d_dt);

                static void (*output_string_func)(std::ostringstream &, const char *) = OutputString;
                static void (*output_number_func)(std::ostringstream &, const double, unsigned) = OutputNumber<true>;

                output_string_func = (!args.no_header and j == args.skip_lines) ? OutputHeaderString : OutputString;
                output_number_func = (!args.no_header and j == args.skip_lines) ? OutputHeaderNumber : OutputNumber<true>;

                for (WORD cellCol = 0; cellCol <= pws->rows.lastcol; cellCol++) {
                    xlsCell *cell = xls_cell(pws, cellRow, cellCol);
                    if (!cell || cell->isHidden)
                        continue;

                    if (cellCol)
                        oss << ',';

                    // display the value of the cell (either numeric or string)
                    if (cell->id == XLS_RECORD_RK || cell->id == XLS_RECORD_MULRK || cell->id == XLS_RECORD_NUMBER)
                        output_number_func(oss, cell->d, cellCol);
                    else if (cell->id == XLS_RECORD_FORMULA || cell->id == XLS_RECORD_FORMULA_ALT) {
                        // formula
                        if (cell->l == 0)
                            output_number_func(oss, cell->d, cellCol);
                        else if (cell->str) {
                            if (!strcmp((char *)cell->str, "bool")) // its boolean, and test cell->d
                                output_string_func(oss, (int) cell->d ? "True" : "False");
                            else
                            if (!strcmp((char *)cell->str, "error")) // formula is in error
                                output_string_func(oss, "*error*");
                            else // ... cell->str is valid as the result of a string formula.
                                output_string_func(oss, (char *)cell->str);
                        }
                    } else if (cell->str)
                        output_string_func(oss, (char *)cell->str);
                    else
                        output_string_func(oss, "");
                }
            }

            transform_csv(args, oss, os);
        };

        print_sheet(sheet_index, std::cout, a, use_date_datetime_excel::yes);

        auto sheet_name_by_index = [&pwb](std::string const & index) {
            unsigned idx = std::atoi(index.c_str());
            if (idx >= pwb->sheets.count)
                throw std::runtime_error("List index out of range");
            return pwb->sheets.sheet[idx].name;
        };

        print_sheets(a, sheet_name_by_index, sheet_index_by_name, fill_sheets, print_sheet);
    }

    void impl::convert() {
        try {
            convert_impl(a);
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }
}
