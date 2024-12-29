///
/// \file   suite/include/in2csv/in2csv_xlsx.h
/// \author wiluite
/// \brief  Header of the xlsx-to-csv converter.

#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>
#include <vector>

namespace in2csv::detail {

    namespace xlsx {
        struct impl_args {
            unsigned maxfieldsize;
            std::string encoding; // always UTF-8
            bool skip_init_space;
            bool no_header;
            unsigned skip_lines;
            bool linenumbers;
            bool zero;
            std::string num_locale;
            bool blanks;
            mutable std::vector<std::string> null_value;
            std::string date_fmt;
            std::string datetime_fmt;
            bool no_leading_zeroes;
            bool no_inference;
            bool date_lib_parser;
            bool asap;

            bool names; // "Display sheet names from the input Excel file."
            std::string sheet; // "The name of the Excel sheet to operate on."
            std::string write_sheets; // "The names of the Excel sheets to write to files, or \"-\" to write all sheets.
            bool use_sheet_names; // "Use the sheet names as file names when --write-sheets is set."
            std::string d_excel;
            std::string dt_excel;
            bool is1904;
            std::filesystem::path file;
        };

        struct impl {
            explicit impl(impl_args a) : a(std::move(a)) {}
            void convert();
        private:
            impl_args a;
        };
        static std::shared_ptr<impl> pimpl;
    }

    template <class Args2>
    struct xlsx_client final : converter_client {
        explicit xlsx_client(Args2 & args) {
            xlsx::pimpl = std::make_shared<xlsx::impl> (xlsx::impl_args(
                args.maxfieldsize
                , args.encoding
                , args.skip_init_space
                , args.no_header
                , args.skip_lines
                , args.linenumbers
                , args.zero
                , args.num_locale
                , args.blanks
                , args.null_value
                , args.date_fmt
                , args.datetime_fmt
                , args.no_leading_zeroes
                , args.no_inference
                , args.date_lib_parser
                , args.asap

                , args.names
                , args.sheet
                , args.write_sheets
                , args.use_sheet_names
                , args.d_excel
                , args.dt_excel
                , args.is1904
                , args.file));
        }
        void convert() override {
            xlsx::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<xlsx_client>(args);
        }
    };

}
