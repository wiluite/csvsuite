///
/// \file   suite/include/in2csv/in2csv_fixed.h
/// \author wiluite
/// \brief  Header of the fixed-to-csv converter.

#pragma once

#include "converter_client.h"
#include "csv_co/reader.hpp"
#include "reader-bridge-impl.hpp"

namespace in2csv::detail {

    namespace fixed {
        struct impl_args {
            bool skip_init_space;
            std::string schema;
            std::string encoding;
            unsigned skip_lines;
            bool linenumbers;
            bool check_integrity;
            std::filesystem::path file;
        };

        struct impl {
            explicit impl(impl_args a) : a(std::move(a)) {}

            void convert();

        private:
            impl_args a;
        };

        static std::shared_ptr<impl> pimpl;

        template <typename RT>
        struct schema_decoder {
            explicit schema_decoder(RT & reader) : reader(reader) {
                static std::locale loc("C");
                elem_type::imbue_num_locale(loc);
                reader.run_rows(
                    [&](auto rowspan) {
                        for (auto e : rowspan)
                            columns.push_back(e.operator csv_co::unquoted_cell_string());
                    }
                    ,[&](auto rowspan) {
                        unsigned col = 0;
                        for (auto e : rowspan) {
                            auto et {elem_type{e}};
                            if (et.is_num()) {
                                auto const value = columns[col];
                                if (value == "column")
                                    all[value].emplace_back(et.operator csv_co::unquoted_cell_string());
                                else
                                    all[value].emplace_back(static_cast<unsigned>(et.num()));
                            }
                            else if (et.is_str()) {
                                auto const value = columns[col];
                                if (value != "start" and value != "length" )
                                    all[value].push_back(et.operator csv_co::unquoted_cell_string());
                                else
                                    throw std::runtime_error("A value of unsupported type '"
                                        + et.operator csv_co::unquoted_cell_string() + "' for " + value + '.');
                            }
                            else
                                throw std::runtime_error("A value of unsupported type or a null value is in the schema file.");

                            ++col;
                        }
                    }
                );

                if (all.find("column") == all.end())
                    throw std::runtime_error("ValueError: A column named \"column\" must exist in the schema file.");
                if (all.find("start") == all.end())
                    throw std::runtime_error("ValueError: A column named \"start\" must exist in the schema file.");
                if (all.find("length") == all.end())
                    throw std::runtime_error("ValueError: A column named \"length\" must exist in the schema file.");

                auto & nms = all["column"];
                for (auto & nm : nms)
                    names_.push_back(std::get<0>(nm));

                auto & strts = all["start"];
                for (auto & e : strts)
                    starts_.push_back(std::get<1>(e));

                if (!starts_.empty() and starts_[0] == 1)
                    for (auto & e : starts_)
                        --e;

                auto & lngths = all["length"];
                for (auto & e : lngths)
                    lengths_.push_back(std::get<1>(e));

            }
            auto & names() const {
                return names_;
            }
            auto & starts() const {
                return starts_;
            }
            auto & lengths() const {
                return lengths_;
            }
        private:
            RT & reader;
            std::vector<std::string> names_;
            std::vector<unsigned> starts_;
            std::vector<unsigned> lengths_;
            using elem_type = typename std::decay_t<RT>::template typed_span<csv_co::unquoted>;
            std::vector<std::string> columns;
            std::unordered_map<std::string, std::vector<std::variant<std::string, unsigned>>> all;
        };
    }

    template <class Args2>
    struct fixed_client final : converter_client {
        explicit fixed_client(Args2 & args) {
            fixed::pimpl = std::make_shared<fixed::impl>
                (fixed::impl_args(args.skip_init_space, args.schema, args.encoding, args.skip_lines, args.linenumbers, true, args.file));
        }
        void convert() override {
            fixed::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<fixed_client>(args);
        }
    };

}
