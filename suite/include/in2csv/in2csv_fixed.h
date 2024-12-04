#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>

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
            impl(impl_args a) : a(std::move(a)) {}
            void convert();
        private:
            impl_args a;
        };
        static std::shared_ptr<impl> pimpl;
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
