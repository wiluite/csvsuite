///
/// \file   suite/include/in2csv/in2csv_json.h
/// \author wiluite
/// \brief  Header of the json-to-csv converter.

#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>
#include <vector>

namespace in2csv::detail {

    namespace json {
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

            std::string key;
            bool non_flat;
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
    struct json_client final : converter_client {
        explicit json_client(Args2 & args) {
            json::pimpl = std::make_shared<json::impl> (json::impl_args{
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

                , args.key
                , args.non_flat
                , args.file});
        }
        void convert() override {
            json::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<json_client>(args);
        }
    };

}
