///
/// \file   utils/csvkit/include/enconding.h
/// \author wiluite
/// \brief  Means for converting text from one encoding to another.

#pragma once

#include <string>
#include "../../../external/simdutf/simdutf.h"
#include <cppp/reiconv.hpp>

using namespace simdutf;
using namespace cppp::base::reiconv;

namespace csvkit::cli::encoding::detail {
    static std::string& acp_string() {
        static std::string acp;
        return acp;
    }

    static bool& e_name_found() {
        static bool found = false;
        return found;
    }

    static int check_encoding_impl (unsigned int namescount, const char * const * names, void* data) {
        std::string const & e_name = *static_cast<std::string*>(data);
        for (auto i = 0u; i < namescount; i++) {
            if (!strcmp(names[i], e_name.c_str())) {
                e_name_found() = true;
                return 1;
            }
        }
        return 0;
    }

    static int active_code_page_name_impl (unsigned int namescount, const char * const * names, void* data) {
#ifndef unix
        static auto const acp = GetACP();
        (void)data;
        for (auto i = 0u; i < namescount; i++) {
            if (!strcmp(names[i], (std::string("CP")+std::to_string(acp)).c_str())
                || !strcmp(names[i], (std::string("CP-")+std::to_string(acp)).c_str())) {
                    acp_string() = names[i];
                    return 1; // stop iteration
            }
        }
        return 0; // continue
#else
        return 1;
#endif
    }
}

#define recode(len_func, conv_func, divisor, type_name, source)                                     \
    auto from_words = source.size()/divisor;                                                        \
    auto const * const src_start = reinterpret_cast<type_name const*>(source.data());               \
    auto l = len_func(src_start, from_words);                                                       \
    std::string new_rep (l,' ');                                                                    \
    size_t to_words = conv_func(src_start, from_words, new_rep.data());                             \
    assert(l == to_words);                                                                          \
    return new_rep;

namespace csvkit::cli::encoding {
    auto is_source_utf8(auto & source) {
        // NOTE: ISO-2022-JP detected as UTF-8 (otherwise impossible (python csvkit as well))
        return validate_utf8_with_errors(source.data(), source.size());
    }

    static auto lookup_encoding(std::string & e_name) {
        using namespace detail;
        iconvlist(check_encoding_impl, static_cast<void*>(&e_name));
        if (!e_name_found()) {
            char * p_end{};
            auto const i = std::strtol(e_name.c_str(), &p_end, 10);
            const bool range_error = errno == ERANGE;
            if (range_error)
                throw std::runtime_error("Unexpected conversion error in lookup_encoding()");
            if (i) {
                auto cp_name = "CP" + e_name;
                iconvlist(check_encoding_impl, static_cast<void*>(&cp_name));
                if (e_name_found())
                    e_name = std::move(cp_name);
            }
        }
        return e_name_found();
    }

    template <class T>
    struct inval_tracer {
        void accumulate(size_t portion) const {
            trace_invalid_pos += portion;
        }
        std::string print() const {
            return std::string(" at position ") + std::to_string(trace_invalid_pos);
        }
    private:
        mutable size_t trace_invalid_pos = 0;
    };

    /// Encoding class based on iconv library
    template <std::size_t BufSize=1'000'000>
    class iconv_converter final : public inval_tracer<iconv_converter<>> {

        std::shared_ptr<std::remove_pointer_t<iconv_t>> cd;

    public:
        static_assert(BufSize >= 6 && BufSize <= 1'000'000);
        iconv_converter(iconv_converter &&) noexcept = delete;
        explicit iconv_converter(std::string const & from, std::string const & to = "UTF-8") :
            cd ([&from, &to] {
                    auto const desc = iconv_open (to.c_str(), from.c_str());
                    if (desc == reinterpret_cast<iconv_t>(-1))
                        throw std::runtime_error(std::string("iconv_open(): not supported transform from ") + from);
                    else
                        return desc;
                }()
                ,   [&](iconv_t desc) {
                        auto const result = iconv_close(desc); assert(!result);
                    }
                )
            {}

        std::string convert(char const * const src_begin, char const * const src_end) const {
            char * src_ptr = const_cast<char*>(src_begin);
            size_t src_size = src_end - src_begin;

            std::vector<char> buf(BufSize);

            std::string dst;
            while (src_size) {
                char * dst_ptr = &buf.front();
                size_t dst_size = buf.size();
                size_t res = ::iconv(cd.get(), &src_ptr, &src_size, &dst_ptr, &dst_size);
                if (res == static_cast<size_t>(-1)) {
                    if (errno != E2BIG)  {
                        inval_tracer::accumulate(buf.size() - dst_size);
                        throw_convert_error();
                    }
                }

                auto const accumulated = buf.size() - dst_size;
                inval_tracer::accumulate(accumulated);
                if ((adjacent_zeros = (accumulated ? 0 : adjacent_zeros + 1)) > 1 )
                    throw std::runtime_error("iconv_converter: BufSize is still too small");

                dst.append(&buf.front(), accumulated);
            }
            return dst;
        }

        std::string convert(auto & src) const {
            return convert(src.data(), src.data() + src.size());
        }

    private:
        void throw_convert_error() const {
            switch (errno) {
                case EILSEQ: 
                    throw std::runtime_error("iconv_converter: Invalid multibyte sequence" + inval_tracer::print()); 

                case EINVAL: 
                    throw std::runtime_error("iconv_converter: Incomplete multibyte sequence");

                default: 
                    throw std::runtime_error("iconv_converter: unknown error");
            }
        }

        mutable unsigned adjacent_zeros = 0;
    };

    static_assert(!std::is_move_constructible_v<iconv_converter<>>);
    static_assert(!std::is_move_assignable_v<iconv_converter<>>);
    static_assert(!std::is_copy_constructible_v<iconv_converter<>>);
    static_assert(!std::is_copy_assignable_v<iconv_converter<>>);

    /// Recodes test from given encoding to UTF-8
    auto recode_source_from(auto & source, std::string from) {

        std::transform(from.begin(), from.end(), from.begin(), ::toupper);

        if (from == "UTF-16LE" or from == "UTF-16-LE") {
            //TODO: validate
            recode(utf8_length_from_utf16le, convert_utf16le_to_utf8, 2, char16_t, source)
        } else if (from == "UTF-16BE" or from == "UTF-16-BE") {
            recode(utf8_length_from_utf16be, convert_utf16be_to_utf8, 2, char16_t, source)
        } else if (from == "UTF-32LE" or from == "UTF-32-LE") {
            recode(utf8_length_from_utf32, convert_utf32_to_utf8, 4, char32_t, source)
        } else {

            if (!lookup_encoding(from))
                throw std::runtime_error("LookupError: unknown encoding: " + from);

            return iconv_converter(from).convert(source);
        }
    }

    static auto active_code_page_name() {
        iconvlist(detail::active_code_page_name_impl, nullptr);
        return detail::acp_string().empty() ? std::tuple{false, "UTF-8"} : std::tuple{true, detail::acp_string().c_str()};
    }

    static std::string iconv(const std::string& to, const std::string& from, std::string const & what) {
        return iconv_converter(from, to).convert(const_cast<char*>(what.data()), what.data()+what.size());
    }

    class acp_transformer {

        char const * active_cp;
        bool disabled_ = false;
        std::function<void(std::string &)> fun;

    public:
        acp_transformer() {
            auto reverse_conversion = active_code_page_name();
            if (std::get<0>(reverse_conversion))
                fun = [&](std::string & src) {
                    if (!disabled_)
                        src = iconv(active_cp, "UTF-8", src);
                };
            else
                fun = [&](std::string & src) {};
            active_cp = std::get<1>(reverse_conversion);
        }

        void transform(std::string & str) const {
            fun(str);
        }

        [[nodiscard]] char const * acp() const noexcept {
            return active_cp;
        }

        void disable() {
            disabled_ = true;
        }
        acp_transformer(acp_transformer &&) noexcept = delete;
        acp_transformer& operator =(acp_transformer &&) noexcept = delete;
    };
    static_assert(!std::is_copy_constructible_v<acp_transformer>);
    static_assert(!std::is_move_constructible_v<acp_transformer>);
    static_assert(!std::is_copy_assignable_v<acp_transformer>);
    static_assert(!std::is_move_assignable_v<acp_transformer>);

}
