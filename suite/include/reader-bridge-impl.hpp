///
/// \file   utils/csvsuite/include/reader-bridge-impl.h
/// \author wiluite
/// \brief  Definitions of the typed_span template methods.

#pragma once

#if 0
#define USE_BOOST_MULTIPRECISION_FOR_DECIMAL_PLACES_CALCULATIONS
#endif

#include <csv_co/reader.hpp>
#include <../external/vince-csv-parser/data_type.h>
#include <unordered_set>
#include <deque>
#include "../external/date/date.h"
#include <codecvt>
#include "../external/Alphabet-AB/UtfConv.c"

#if defined(USE_BOOST_MULTIPRECISION_FOR_DECIMAL_PLACES_CALCULATIONS)
    #include <boost/multiprecision/cpp_dec_float.hpp>
#endif

namespace csv_co::csvsuite {
    constexpr auto to_basic_string_32(auto && str) {
        return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().from_bytes(str);
    }
    constexpr auto from_basic_string_32(auto && str) {
        return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().to_bytes(str);
    }
    /// Counts the number of characters in a guaranteed utf-8 string.
    constexpr auto str_symbols(auto && str) {
        std::size_t len = 0;
        for (auto && e : str)
            len += (e & 0xc0) != 0x80;
        return len;
    }
    /// Counts the number of characters in a utf-8 string with checking for utf-8
    constexpr auto str_symbols_deep_check(auto && str) {
        return to_basic_string_32(str).size();
    }
}

namespace csv_co {
    using DataType = vince_csv::DataType;
    constexpr const bool unquoted = true;
    constexpr const bool quoted = false;

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::to_C_locale(std::string & s) const {
        std::erase_if(s, [&](char x) {
            return x == std::use_facet<std::numpunct<char>>(num_locale()).thousands_sep();
        });

        auto const decimal_point = std::use_facet<std::numpunct<char>>(num_locale()).decimal_point();

        if (decimal_point != '.' and s.find('.') != std::string::npos)
            return false;

        if (auto const pos = s.find(decimal_point); pos != std::string::npos)
            s[pos] = '.';

        return true;
    }

#if defined(USE_BOOST_MULTIPRECISION_FOR_DECIMAL_PLACES_CALCULATIONS)
    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline unsigned char reader<T, Q, D, L, M, E>::typed_span<Unquoted>::get_precision(std::string & rep) const {
        rep.erase(0, rep.find_first_not_of(' '));
        rep.erase(rep.find_last_not_of(" \t\r") + 1);
        boost::multiprecision::cpp_dec_float_50 val(abs(boost::multiprecision::cpp_dec_float_50(rep)));
        if (boost::multiprecision::trunc(val) == val )
            return 0;
        auto const ss_prec = std::numeric_limits<boost::multiprecision::cpp_dec_float_50>::digits10;
        boost::multiprecision::cpp_dec_float_50 tmp;
        auto const zero_pos = boost::multiprecision::modf(val, &tmp).str(ss_prec, std::ios::fixed).find_last_not_of('0') + 1;
        assert(zero_pos > 1);
        return zero_pos == 2 ? ss_prec : zero_pos - 2;
    }
#else
    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline unsigned char reader<T, Q, D, L, M, E>::typed_span<Unquoted>::get_precision() const {
        // https://stackoverflow.com/a/3019027
#if defined(_MSC_VER)
        static_assert(std::numeric_limits<decltype(this->value)>::digits10 == 15);
        static auto max_digits = std::numeric_limits<long double>::digits10 - 1;
#else
        static_assert(std::numeric_limits<decltype(this->value)>::digits10 == 18);
        static auto max_digits = 14;
#endif
        auto int_part = std::trunc(std::abs(double(this->value)));
        unsigned long long magnitude = (int_part == 0) ? 1 : static_cast<unsigned long long>(std::trunc(std::log10(int_part)) + 1);
        if (magnitude >= (unsigned)max_digits)
            return 0;
        auto frac_part = std::abs(double(this->value)) - int_part;
        auto multiplier = pow(10.0, static_cast<double>(max_digits) - static_cast<double>(magnitude));
        unsigned long long frac_digits = multiplier + std::trunc(multiplier * frac_part + 0.5);
        while (frac_digits % 10 == 0)
            frac_digits /= 10;
        auto scale = static_cast<int>(std::log10(frac_digits));
        return scale;
    }
#endif

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::can_be_money(std::string const & s) const {
        // if no money thing is present in this locale, just exit;
        if (std::use_facet<std::moneypunct<char>>(num_locale()).curr_symbol().empty())
            return false;
        auto const fix_possibly_wrong_money_locale = [&] {
            unsigned char min_cnt = 0;
            return std::any_of(s.cbegin(), s.cend(), [&min_cnt](char x) {
                min_cnt = (x == '-') ? min_cnt + 1 : min_cnt;
                return (x=='/' or x==':' or min_cnt > 2);
            });
        };
        if (fix_possibly_wrong_money_locale())
            return false;
        return true;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::get_num_from_money() const {
        std::stringstream in(str());
        in.imbue(num_locale());
        std::string double_rep;
        in >> std::get_money(double_rep, false);
        if (in) {
            auto const money_decimal_point = std::use_facet<std::moneypunct<char>>(num_locale()).decimal_point();
            if (auto const pos = in.str().find(money_decimal_point); pos != std::string::npos)
                double_rep.insert(double_rep.size()-std::use_facet<std::moneypunct<char>>(num_locale()).frac_digits(), ".");
#ifdef _MSC_VER
            std::from_chars_result r{};
            r = std::from_chars(double_rep.data(), double_rep.data()+double_rep.size(), value, std::chars_format::general);
            if (static_cast<int>(r.ec) == 0) {
#if defined(USE_BOOST_MULTIPRECISION_FOR_DECIMAL_PLACES_CALCULATIONS)
                prec = !no_maxprec_ ? get_precision(double_rep) : 0;
#else
                prec = !no_maxprec_ ? get_precision() : 0;
#endif
                type_ = static_cast<signed char>(DataType::CSV_DOUBLE);
            }
#else //_MSC_VER
            std::stringstream in2(double_rep);
            in2.imbue(std::locale("C"));
            in2 >> value;
#if defined(USE_BOOST_MULTIPRECISION_FOR_DECIMAL_PLACES_CALCULATIONS)
            prec = !no_maxprec_ ? get_precision(double_rep) : 0;
#else
            prec = !no_maxprec_ ? get_precision() : 0;
#endif
            type_ = static_cast<signed char>(DataType::CSV_DOUBLE);
#endif
        }
    }

    // --
    // In order to correctly detect type we should trim cell string in all cases where trim policy is not alltrim;-)
    // --
    template<TrimPolicyConcept T, bool Unquoted>
    inline auto toupper_cell_string(auto & cell_span) {
        cell_string str;
        if constexpr (!Unquoted && std::is_same_v<T, trim_policy::alltrim>)
            str = cell_span.operator cell_string();
        else if constexpr (Unquoted && std::is_same_v<T, trim_policy::alltrim>)
            str = cell_span.operator unquoted_cell_string();
        else if constexpr (Unquoted && !std::is_same_v<T, trim_policy::alltrim>)
            str = trim_policy::alltrim::ret_trim(cell_span.operator unquoted_cell_string());
        else if constexpr (!Unquoted && !std::is_same_v<T, trim_policy::alltrim>)
            str = trim_policy::alltrim::ret_trim(cell_span.operator cell_string());
        else
            assert(0);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    inline auto & get_inf_nan_set() {
        static std::unordered_set<std::string_view> inf_nan_set {"NAN", "INF", "INFINITY", "+INF", "-INF", "+INFINITY", "-INFINITY"};
        return inf_nan_set;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::get_value() const {
        using namespace vince_csv::internals;
        // Check to see if value has been cached previously, if not evaluate it
        if (static_cast<DataType>(type_) != DataType::UNKNOWN)
            return;

        auto s_ = str();

        auto postprocess_num = [&] {
            if (no_leading_zeroes_) {
                assert(static_cast<DataType>(type_) > DataType::CSV_STRING);
                auto const pos = s_.find_first_not_of(' ');
                if (pos != std::string::npos and s_[pos] == '0')
                    type_ = static_cast<signed char>(DataType::CSV_STRING);
            }
        };

        auto detect_num_type = [&] {
            if ((type_ = static_cast<signed char>(data_type(s_, &value))) > static_cast<signed char>(DataType::CSV_STRING)) {
#if defined(USE_BOOST_MULTIPRECISION_FOR_DECIMAL_PLACES_CALCULATIONS)
                prec = static_cast<DataType>(type_) == DataType::CSV_DOUBLE and !no_maxprec_ ? get_precision(s_) : 0;
#else
                prec = static_cast<DataType>(type_) == DataType::CSV_DOUBLE and !no_maxprec_ ? get_precision() : 0;
#endif
                postprocess_num();
                return true;
            }
            return false;
        };

        if (num_locale().name() == "C") {
            if (detect_num_type())
                return;
        } else {
            if (to_C_locale(s_)) {
                if (detect_num_type())
                    return;
                else 
                if (static_cast<DataType>(type_) == DataType::CSV_STRING and can_be_money(s_))
                    get_num_from_money();
            } else
                type_ = static_cast<signed char>(DataType::CSV_STRING);
        }

        if (static_cast<DataType>(type_) == DataType::CSV_STRING) {
            auto const iter = get_inf_nan_set().find(toupper_cell_string<T, Unquoted>(*this));
            if (iter != get_inf_nan_set().end()) {
                value = *iter == "NAN" ? std::nanl("nan") : ((*iter)[0] == '-' ? -INFINITY : INFINITY);
                type_ = static_cast<signed char>(DataType::CSV_DOUBLE);
                prec = 0;
            }
        }
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    reader<T, Q, D, L, M, E>::typed_span<Unquoted>::typed_span(cell_span const &cs)
        : cell_span(cs.b, cs.e), type_{static_cast<signed char>(DataType::UNKNOWN)} {}

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    reader<T, Q, D, L, M, E>::typed_span<Unquoted>::typed_span(std::string const & s)
        : cell_span(s.begin(), s.end()), type_{static_cast<signed char>(DataType::UNKNOWN)} {}

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    typename reader<T, Q, D, L, M, E>::template typed_span<Unquoted> &
    reader<T, Q, D, L, M, E>::typed_span<Unquoted>::operator=(cell_span const &cs) noexcept {
        b = cs.b;
        e = cs.e;
        type_ = static_cast<signed char>(DataType::UNKNOWN);
        return *this;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    [[nodiscard]] auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::text_compare(typed_span const &other) const -> int {
        static_assert(Unquoted == csv_co::unquoted);
        return string_comparison_func(unquoted_cell_string(*this).c_str(), unquoted_cell_string(other).c_str());
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    long double reader<T, Q, D, L, M, E>::typed_span<Unquoted>::num() const {
        auto maybe_exception_l = [&] {
            if (static_cast<DataType>(type_) < DataType::CSV_INT8) {
                if constexpr (!Unquoted)
                    throw exception(std::string("Cell ") + cell_string(*this) + " is not numeric");
                else
                    throw exception(std::string("Cell ") + unquoted_cell_string(*this) + " is not numeric");
            }
        };
        if (static_cast<DataType>(type_) == DataType::UNKNOWN) {
            type_ = type();
            assert(DataType::UNKNOWN != static_cast<DataType>(type_));
        }
        maybe_exception_l();
        return value;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::unsafe() const noexcept {
        assert(static_cast<DataType>(type_) != DataType::UNKNOWN);
        return value;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_nil() const {
        return type() == static_cast<signed char>(DataType::CSV_NULL);
    }

    inline auto & get_none_set() {
        static std::unordered_set<std::string_view> default_none_set {"NA", "N/A", "NONE", "NULL", "."};
        return default_none_set;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_null() const {
        if (is_nil())
            return true;
        return static_cast<DataType>(type_) == DataType::CSV_STRING and get_none_set().find(toupper_cell_string<T, Unquoted>(*this)) != get_none_set().end();
    }

    inline auto & get_null_value_set() {
        static std::unordered_set<std::string_view> null_value_set {};
        return null_value_set;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_null_value() const {
        if (is_nil())
            return false;
        return static_cast<DataType>(type_) == DataType::CSV_STRING and get_null_value_set().find(toupper_cell_string<T, Unquoted>(*this)) != get_null_value_set().end();
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_null_or_null_value() const {
        if (is_nil())
            return true;
        if (static_cast<DataType>(type_) != DataType::CSV_STRING)
            return false;
        return get_none_set().find(toupper_cell_string<T, Unquoted>(*this)) != get_none_set().end() or
               get_null_value_set().find(toupper_cell_string<T, Unquoted>(*this)) != get_null_value_set().end();
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_str() const {
        return type() == static_cast<signed char>(DataType::CSV_STRING);
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_num() const {
        return type() >= static_cast<signed char>(DataType::CSV_INT8);
    }

    inline auto & get_bool_set() {
        static std::unordered_set<std::string_view> bool_set {"T", "F", "TRUE", "FALSE", "\"1\"", "\"0\"", "Y", "N", "YES", "NO"};
        return bool_set;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_boolean() const {
        type();
        if (static_cast<DataType>(type_) == DataType::CSV_INT8 && (value == 0 or value == 1))
            return true;
        else if (static_cast<DataType>(type_) == DataType::CSV_STRING) {
            auto const str = toupper_cell_string<T,Unquoted>(*this);
            auto const result = get_bool_set().find(str) != get_bool_set().end();
            if (result) {
                // update value for get_bool() function use.
                value = (str == "F" || str == "FALSE" || str == "\"0\"" || str == "N" || str == "NO") ? 0 : 1;
                // we now are able to change inner type for better caching.
                type_ = static_cast<signed char>(DataType::CSV_INT8);
            }
            return result;
        } else
            return false;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_int() const {
        return (type() >= static_cast<signed char>(DataType::CSV_INT8)) && (type() <= static_cast<signed char>(DataType::CSV_INT64));
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr bool reader<T, Q, D, L, M, E>::typed_span<Unquoted>::is_float() const {
        return type() == static_cast<signed char>(DataType::CSV_DOUBLE);
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr signed char reader<T, Q, D, L, M, E>::typed_span<Unquoted>::type() const {
        get_value();
        return type_;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr unsigned reader<T, Q, D, L, M, E>::typed_span<Unquoted>::str_size_in_symbols() const {
        get_value();
        return unsafe_str_size_in_symbols();
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr unsigned reader<T, Q, D, L, M, E>::typed_span<Unquoted>::unsafe_str_size_in_symbols() const {
        assert(static_cast<DataType>(type_) != DataType::UNKNOWN);
        return csvsuite::str_symbols(str());
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    constexpr auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::str() const {
        if constexpr (!Unquoted)
            return this->operator cell_string();
        else
            return this->operator unquoted_cell_string();
    }

    enum class DateTimeType : signed char {
        UNKNOWN = -1,
        NO,
        DATE,
        DATETIME
    };

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::datetime(std::string const &extra_dt_fmt) const {
        static datetime_format_proc datetime_fmt_proc(extra_dt_fmt);
        if (static_cast<DateTimeType>(datetime_type) == DateTimeType::UNKNOWN || !extra_dt_fmt.empty()) {
            auto [is, v] = datetime_fmt_proc(*this);
            if (is) {
                datetime_type = static_cast<signed char>(DateTimeType::DATETIME);
                static_assert(sizeof(std::chrono::system_clock::to_time_t(v)) <= sizeof(decltype(value)));
                value = std::chrono::system_clock::to_time_t(v);
                date::sys_seconds tp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::from_time_t(value));
                return std::tuple{true, v};
            } else {
                datetime_type = static_cast<signed char>(DateTimeType::NO);
                return std::tuple{false, date::sys_seconds{}};
            }
        } else {
            if (static_cast<DateTimeType>(datetime_type) == DateTimeType::DATETIME) {
                date::sys_seconds tp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::from_time_t(value));
                return std::tuple{true, tp};
            }
            else
                return std::tuple{false, date::sys_seconds{}};
        }
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::date(std::string const &extra_date_fmt) const {
        static date_format_proc date_fmt_proc(extra_date_fmt);
         
        if (static_cast<DateTimeType>(datetime_type) == DateTimeType::UNKNOWN || !extra_date_fmt.empty()) {
            auto [is, v] = date_fmt_proc(*this);
            if (is) {
                datetime_type = static_cast<signed char>(DateTimeType::DATE);
                value = std::chrono::system_clock::to_time_t(v);
                return std::tuple{true, v};
            } else {
                datetime_type = static_cast<signed char>(DateTimeType::NO);
                return std::tuple{false, date::sys_seconds{}};
            }
        } else {
            if (static_cast<DateTimeType>(datetime_type) == DateTimeType::DATE) {
                date::sys_seconds tp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::from_time_t(value));
                return std::tuple{true, tp};
            }
            else
                return std::tuple{false, date::sys_seconds{}};
        }
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::imbue_num_locale(std::locale & num_loc) noexcept {
        num_locale_ = &num_loc;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::no_maxprecision(bool flag) noexcept {
        no_maxprec_ = flag;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    std::locale& reader<T, Q, D, L, M, E>::typed_span<Unquoted>::num_locale() {
        if (!num_locale_)
            throw exception(std::string{"Locale for "} + std::string(type_name<class_type>()) + " is not set.");
        return *num_locale_;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    unsigned char reader<T, Q, D, L, M, E>::typed_span<Unquoted>::precision() const noexcept {
        return prec;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    inline auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::datetime_call_operator_impl(auto const & span, auto & formats) {
        static class parser {
            std::function<std::tuple<bool, date::sys_seconds>(typed_span const &, std::vector<std::string> &)> fun_impl;
        public:
            parser() {
                using namespace std::chrono; 
                if (date_parser_backend == date_parser_backend_t::date_lib_supported) {
                    fun_impl = [&](typed_span const & span, std::vector<std::string> & formats) -> std::tuple<bool, date::sys_seconds> {

                        date::sys_seconds tp;
                        std::string trimmed = span;
                        trimmed.erase(0, trimmed.find_first_not_of(' '));
                        for (auto const &fmt : formats) {
                            std::istringstream in {trimmed};
                            in >> date::parse(fmt, tp);
                            if (in.fail() or in.bad())
                                continue;
                            return std::tuple{true, tp};
                        }
                        return std::tuple{false, date::sys_seconds{}};
                    };
                } else {
                    fun_impl = [&](typed_span const & span, std::vector<std::string> & formats) -> std::tuple<bool, date::sys_seconds> {
                        std::tm calendar = {};
                        std::stringstream ss;
                        ss.imbue(num_locale());
                        for (auto const &fmt : formats) {
                            ss.clear();
                            ss.str({});
                            calendar = {};
                            ss << span << ' ';
                            ss >> std::get_time(&calendar, fmt.c_str());
                            if (!ss.fail()) {
                                if (std::mktime(&calendar) == -1)
                                    return std::tuple{false, date::sys_seconds{}};
                                date::sys_seconds tp = date::sys_days{date::year{calendar.tm_year + 1900}/(calendar.tm_mon+1)/calendar.tm_mday} +
                                    hours{calendar.tm_hour} + minutes{calendar.tm_min} + seconds{calendar.tm_sec};
                                return std::tuple{true, tp};
                            }
                        }
                        return std::tuple{false, date::sys_seconds{}};
                    };
                }
            }
            auto parse(typed_span const & span, std::vector<std::string> & formats) {
                return fun_impl(span, formats);
            }
        } p;

        return p.parse(span, formats);
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    reader<T, Q, D, L, M, E>::typed_span<Unquoted>::datetime_format_proc::datetime_format_proc(std::string const & extra_dt_fmt) {
        if (!extra_dt_fmt.empty()) {
            formats.emplace_back(extra_dt_fmt);
            formats.emplace_back("%Y-%m-%d %H:%M:%S");
            formats.emplace_back("%Y-%m-%dT%H:%M:%S");
            formats.emplace_back("%Y-%m-%dT%H:%M:%SZ");
            formats.emplace_back("%Y-%b-%d %H:%M:%S");
        } else
            assert(!formats.empty());  
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::datetime_format_proc::operator()(typed_span const & span) const {
        return datetime_call_operator_impl(span, formats);
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    reader<T, Q, D, L, M, E>::typed_span<Unquoted>::date_format_proc::date_format_proc(std::string const & extra_date_fmt) {
        if (!extra_date_fmt.empty()) {
            formats.emplace_back(extra_date_fmt);
            formats.emplace_back("%Y-%m-%d");
        } else
            assert(!formats.empty());  
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::date_format_proc::operator()(typed_span const & ccs) const {
        return datetime_call_operator_impl(ccs, formats);
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::setup_date_parser_backend(date_parser_backend_t value) noexcept {
        date_parser_backend = value;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::no_leading_zeroes(bool flag) noexcept {
        no_leading_zeroes_ = flag;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    void reader<T, Q, D, L, M, E>::typed_span<Unquoted>::case_insensitivity(bool flag) noexcept {
        if (flag)
            string_comparison_func = reinterpret_cast<int (*)(void const*, void const*)>(StrCiCmpUtf8);
        else
            string_comparison_func = reinterpret_cast<int (*)(void const*, void const*)>(strcmp);
    }

    /// Time storage class for time parser class right underneath
    class time_storage {
        friend class time_parser;

        enum class output_resolution {
            MICRO,
            NANO
        };

        template<class SS, class... Ts>
        void time_to_stream(SS & ss, Ts&&... ts) const {
            (ss << ... << std::forward<Ts>(ts));
        }

        [[nodiscard]] std::string str(output_resolution r = output_resolution::MICRO) const {
            if (r == output_resolution::MICRO) {
                microseconds_ = static_cast<decltype(microseconds_)>(round<std::chrono::microseconds>(rest_).count());
                nanoseconds_ = 0;
            } else {
                microseconds_ = static_cast<decltype(microseconds_)>(floor<std::chrono::microseconds>(rest_).count());
                nanoseconds_ = static_cast<decltype(microseconds_)>(round<std::chrono::nanoseconds>(rest_ - floor<std::chrono::microseconds>(rest_)).count());
            }

            std::stringstream ss;
            ss.imbue(std::locale("C"));
            ss.fill('0');
            if (days_)
                time_to_stream(ss, days_, (days_ == 1 ? " day, " : " days, "));
            time_to_stream(ss,  std::setw(clock_hours_ > 9 ? 2 : 1), clock_hours_, ":", std::setw(2), clock_minutes_, ":", std::setw(2), clock_seconds_);
            if (microseconds_)
                time_to_stream(ss, '.', microseconds_);
            if (nanoseconds_)
                time_to_stream(ss, ',', nanoseconds_, "ns");
            return ss.str();
        }

        std::size_t seconds_ {};
        mutable std::chrono::duration<long double, std::ratio<1>> rest_ {};
        mutable unsigned days_{};
        mutable unsigned clock_hours_{};
        mutable unsigned clock_minutes_{};
        mutable unsigned clock_seconds_{};
        mutable unsigned microseconds_{};
        mutable unsigned nanoseconds_{};

        void fill_parts(std::chrono::duration<long double, std::ratio<1>> data) const {
            days_ = static_cast<decltype(days_)>(floor<std::chrono::days>(data).count());
            data -= floor<std::chrono::days>(data);
            clock_hours_ = static_cast<decltype(clock_hours_)>(floor<std::chrono::hours>(data).count());
            data -= floor<std::chrono::hours>(data);
            clock_minutes_ = static_cast<decltype(clock_minutes_)>(floor<std::chrono::minutes>(data).count());
            data -= floor<std::chrono::minutes>(data);
            clock_seconds_ = static_cast<decltype(clock_seconds_)>(floor<std::chrono::seconds>(data).count());
            data -= floor<std::chrono::seconds>(data);
            rest_ = data;
        }

    public:
        [[nodiscard]] std::string str(long double seconds, output_resolution r = output_resolution::MICRO) const {
            std::chrono::duration<double, std::ratio<1>> secs {seconds};
            fill_parts(secs);
            return str(r);
        }
    };

    /// Time parser for timedelta type cells
    class time_parser {
    private:
        time_storage tm_storage;
    public:
        operator long double() const {
            auto const microsecs = static_cast<double>(floor<std::chrono::microseconds>(tm_storage.rest_).count());
            auto const nanosecs = static_cast<double>(round<std::chrono::nanoseconds>(tm_storage.rest_ - floor<std::chrono::microseconds>(tm_storage.rest_)).count());
            return static_cast<long double>(tm_storage.seconds_) + microsecs/1000000.0 + nanosecs/1000000000.0;
        }
        time_parser() = default;
        time_parser(time_parser && other) = delete;
        time_parser& operator=(time_parser && other) = delete;

        bool parse(std::string_view sv) {
            auto const end = sv.find_last_not_of(" \r\t\n");
            auto const beg = sv.find_first_not_of(" \r\t\n");
            if (end - beg == 0)
                return false;

            if (sv.find_first_of('h') != std::string_view::npos and std::count(sv.begin(), sv.end(), ':'))
                return false;

            auto cursor = sv.begin() + static_cast<long long>(beg);
            auto const e = sv.begin() + static_cast<long long>(end) + 1;

            constexpr int max_possible_semicolons = 3;
            auto const semicolons = static_cast<int>(std::count_if(cursor, e, [](auto elem) {return elem == ':';}));

            if (semicolons > max_possible_semicolons)
                return false;

            auto duplicated_essences = [&] {
                return std::min(semicolons, max_possible_semicolons) == max_possible_semicolons and sv.find_first_of("wd") != std::string_view::npos;
            };
            if (duplicated_essences())
                return false;

            std::deque<std::vector<std::string>> sym_deq {
                      {"w", "wk", "week", "weeks"}
                    , {"d", "day", "days"}
                    , {"h", "hr", "hrs", "hour", "hours"}
                    , {"m", "min", "mins", "minute", "minutes"}
                    , {"s", "sec", "secs", "second", "seconds"}
            };

            std::unordered_map<std::string, long double> sym_to_sec {
                      {"w", 604800}, {"wk", 604800}, {"week", 604800}, {"weeks", 604800}
                    , {"d", 86400}, {"day", 86400}, {"days", 86400}
                    , {"h", 3600}, {"hr", 3600}, {"hrs", 3600}, {"hour", 3600}, {"hours", 3600}
                    , {"m", 60}, {"min", 60}, {"mins", 60}, {"minute", 60}, {"minutes", 60}
                    , {"s", 1}, {"sec", 1}, {"secs", 1}, {"second", 1}, {"seconds", 1}
            };

            std::deque<long double> sec_deq;

            if (semicolons) {
                sec_deq.push_back(60);
                sec_deq.push_back(1);
            }

            if (semicolons > 1)
                sec_deq.push_front(3600);

            if (semicolons > 2)
                sec_deq.push_front(86400);

            enum class fun_type {digit, complete_time, word};
            fun_type next_fun = fun_type::digit;
            char c;

            auto initial_space_func = [&c, &cursor] {
                c = *cursor++;
                return true;
            };

            std::function<bool()> space_fn = initial_space_func;
            std::chrono::duration<long double, std::ratio<1>> total_sec {0};

            unsigned char commas = 0;

            std::string recent_num;
            std::locale loc {"C"};

            while (cursor != e) {
                auto space_fun = [&c, &cursor, &commas, &e, &loc] {
                    if (commas > 1)
                        return false;
                    for(;;) {
                        if (cursor == e or (c == ',' and ++commas > 1))
                            return false;
                        if (!(std::isspace(c = *cursor++, loc)) and c != ',')
                            return true;
                    }
                };

                auto compose_num = [&c, &cursor, &e, &loc] {
                    unsigned dots = 0;
                    auto num_char = [&c, &cursor, &dots, &loc] {
                        c = *cursor++;
                        dots += c == '.' ? 1 : 0;
                        return (dots < 2 and (std::isdigit(c, loc) or c == '.'));
                    };
                    std::string s;
                    do s += c; while (cursor != e and num_char());
                    return s;
                };

                auto inc_sec = [&recent_num, &total_sec] (auto const & num) {
                    struct sstringstream : std::stringstream {
                        sstringstream() : std::stringstream() { imbue(std::locale("C")); }
                    };
                    sstringstream ss;
                    ss << recent_num;
                    long double val;
                    ss >> val;
                    total_sec += std::chrono::duration<long double, std::ratio<1>>(val * num);
                };

                auto digit_fun = [&space_fn, &c, &space_fun, &compose_num, &cursor, &recent_num, &inc_sec, &sec_deq, &next_fun, &commas, &loc] {
                    if (!space_fn() or !std::isdigit(c, loc))
                        return false;
                    if (space_fn.target<decltype(initial_space_func)>())
                        space_fn = space_fun;
                    recent_num = compose_num();
                    --cursor;
                    if (c == ':') {
                        if (recent_num.find('.') != std::string::npos)
                            return false;
                        inc_sec(sec_deq.front());
                        sec_deq.pop_front();
                        next_fun = fun_type::complete_time;
                    } else {
                        commas = 0;
                        next_fun = fun_type::word;
                    }
                    return true;
                };

                auto complete_time_fun = [&cursor, &e, &c, &recent_num, &compose_num, &inc_sec, &sec_deq, &loc] {
                    if (++cursor == e or !(std::isdigit(c = *cursor++, loc)))
                        return false;
                    recent_num = compose_num();
                    if (cursor == e and std::isdigit(*(cursor - 1), loc)) {
                        auto const dot_pos = recent_num.find_first_of('.');
                        if ((dot_pos != std::string::npos and dot_pos != 2) or (dot_pos == std::string::npos and recent_num.size() != 2))
                            return false;
                        inc_sec(sec_deq.front());
                        sec_deq.pop_front();
                        return true;
                    }
                    --cursor;
                    if (c == ':') {
                        if (recent_num.find('.') != std::string::npos or recent_num.size() != 2)
                            return false;
                    } else {
                        while(cursor != e) {
                            if (!std::isspace(*cursor++, loc))
                                return false;
                        }
                    }
                    inc_sec(sec_deq.front());
                    sec_deq.pop_front();
                    return true;
                };

                auto word_fun = [&space_fun, &c, &cursor, &e, &commas, &sym_deq, &inc_sec, &sym_to_sec, &next_fun, &loc] {
                    if (!space_fun() or !(std::isalpha(c, loc)))
                        return false;

                    auto isalpha_or_comma = [&] {
                        return std::isalpha(c = *cursor++, loc) || c == ',';
                    };
                    std::string word;
                    do {
                        if (c != ',')
                            word += c;
                        else
                            ++commas;
                    }
                    while (cursor != e and isalpha_or_comma());
                    for (;;) {
                        if (sym_deq.empty())
                            return false;
                        auto const & front = sym_deq.front();
                        bool found = false;
                        for (auto & elem : front) {
                            if (elem == word) {
                                inc_sec(sym_to_sec[elem]);
                                found = true;
                                break;
                            }
                        }
                        sym_deq.pop_front();
                        if (found)
                            break;
                    }

                    if (cursor == e and std::isalpha(*(cursor - 1), loc))
                        return true;

                    --cursor;
                    next_fun = fun_type::digit;
                    return true;
                };

                std::unordered_map<fun_type, std::function<bool()>>
                    fun_map = {{fun_type::digit, digit_fun}, {fun_type::complete_time, complete_time_fun}, {fun_type::word, word_fun}};

                if (!fun_map[next_fun]())
                    return false;
            }

            tm_storage.seconds_ = static_cast<decltype(tm_storage.seconds_)>(floor<std::chrono::seconds>(total_sec).count());
            tm_storage.fill_parts(total_sec);
            return true;
        }

        [[nodiscard]] std::string str() const {
            return tm_storage.str();
        }
    };
    static_assert(!std::is_move_constructible_v<time_parser>);
    static_assert(!std::is_copy_constructible_v<time_parser>);
    static_assert(!std::is_move_assignable_v<time_parser>);
    static_assert(!std::is_copy_assignable_v<time_parser>);

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    auto reader<T, Q, D, L, M, E>::typed_span<Unquoted>::timedelta_tuple() const {
        time_parser t_parser;
        return t_parser.parse((*this).str()) ? std::tuple{true, t_parser.str()} : std::tuple{false, std::string{}};
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    long double reader<T, Q, D, L, M, E>::typed_span<Unquoted>::timedelta_seconds() const {
        time_parser t_parser;
        t_parser.parse((*this).str());
        return t_parser;
    }

    template<TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<bool Unquoted>
    reader<T, Q, D, L, M, E>::typed_span<Unquoted>::operator typed_span<!Unquoted> const & () const {
        if (!rebind_conversion) {
            type_ = static_cast<signed char>(DataType::UNKNOWN);
            rebind_conversion = true;
        }
        return reinterpret_cast<typed_span<!Unquoted> const &>(*this);
    }

} /// namespace
