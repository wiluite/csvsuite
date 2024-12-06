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

        template <typename RT, typename AT>
        struct schema_decoder {
            schema_decoder(RT & reader, AT & args) : reader(reader), args(args) {
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
#if 0
                auto & names_ = all["column"];
                auto & starts_ = all["start"];
                auto & lengths_ = all["length"];
                assert(names_.size() == starts_.size());
                assert(names_.size() == lengths_.size());
#endif
            }
            auto & names() {return all["column"]; }
            auto & starts() {return all["start"]; }
            auto & lengths() {return all["length"]; }
        private:
            RT & reader;
            AT & args;

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
