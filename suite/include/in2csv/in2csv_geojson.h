///
/// \file   suite/include/in2csv/in2csv_geojson.h
/// \author wiluite
/// \brief  Header of the geojson-to-csv converter.

#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>

namespace in2csv::detail {

    namespace geojson {
        struct impl_args {
            unsigned maxfieldsize;
            std::string encoding; // always UTF-8
            bool skip_init_space;
            bool no_inference;

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
    struct geojson_client final : converter_client {
        explicit geojson_client(Args2 & args) {
            geojson::pimpl = std::make_shared<geojson::impl> (geojson::impl_args{
                args.maxfieldsize
                , args.encoding
                , args.skip_init_space
                , args.no_inference

                , args.file});
        }
        void convert() override {
            geojson::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<geojson_client>(args);
        }
    };

}
