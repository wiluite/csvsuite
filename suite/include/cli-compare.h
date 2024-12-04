///
/// \file   utils/csvkit/include/cli-compare.h
/// \author wiluite
/// \brief  All possible comparison operations, type-aware and type-independent.

#ifndef CSV_CO_CLI_COMPARE_H
#define CSV_CO_CLI_COMPARE_H

namespace csvkit::cli::compare::detail {

    template <class EType, class Native>
    class common_compare_impl : Native {
    protected:
        int operator()(EType const & e1, EType const & e2) const {
            return std_fun(e1, e2);
        }

    private:
        template <class ElemType>
        static inline int compare_blanks_no_I_when_blanks (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            return elem1.operator UElemType const& ().compare(elem2.operator UElemType const& ());
        }

        template <class ElemType>
        static inline int compare_no_blanks_no_I_when_blanks (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1_is_null = elem1.operator UElemType const &().is_null(csv_co::also_match_null_value_option);
            auto const e2_is_null = elem2.operator UElemType const &().is_null(csv_co::also_match_null_value_option);
            return (e1_is_null && e2_is_null) ? 0 : (e1_is_null ? 1 : (e2_is_null ? -1 : Native::native_compare(elem1, elem2)));
        }

        template <class ElemType>
        static int compare_no_blanks_I_when_blanks (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1_is_null = elem1.operator UElemType const &().is_null(csv_co::also_match_null_value_option);
            auto const e2_is_null = elem2.operator UElemType const &().is_null(csv_co::also_match_null_value_option);
            return e1_is_null && e2_is_null ? 0 : (e1_is_null ? 1 : (e2_is_null ? -1 : compare_blanks_no_I_when_blanks(elem1, elem2)));
        }

    protected:
        using class_type = common_compare_impl<EType, Native>;

        common_compare_impl(auto & args, bool has_blanks) : Native(args) {
            if (!args.no_inference && !has_blanks) {
                std_fun = class_type::template native_compare<EType>;
                return;
            }
            if (!args.blanks && !args.no_inference && has_blanks) {
                std_fun = class_type::template compare_no_blanks_no_I_when_blanks<EType>;
                return;
            }
            if (!args.blanks && args.no_inference && has_blanks) {
                std_fun = class_type::template compare_no_blanks_I_when_blanks<EType>;
                return;
            }
            std_fun = class_type::template compare_blanks_no_I_when_blanks<EType>;
        }
        common_compare_impl() = default;

    private:
        std::function<int(EType const &, EType const &)> std_fun;
    };

    class bool_compare_impl {
    protected:
        template <class ElemType>
        static int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            UElemType const & ue1 = elem1;
            UElemType const & ue2 = elem2;
            auto const e1 = (ue1.is_boolean(), ue1.unsafe_bool());
            auto const e2 = (ue2.is_boolean(), ue2.unsafe_bool());
            return e1 == e2 ? 0 : (e1 < e2 ? -1 : 1);
        }
        explicit bool_compare_impl(auto const & ) {}
        bool_compare_impl() = default;
    };

    class timedelta_compare_impl {
    protected:
        template <class ElemType>
        static int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1 = elem1.operator UElemType const&().timedelta_seconds();
            auto const e2 = elem2.operator UElemType const&().timedelta_seconds();
            return e1 == e2 ? 0 : (e1 < e2 ? -1 : 1);
        }
        explicit timedelta_compare_impl (auto const & ) {}
        timedelta_compare_impl() = default;
    };

    class num_compare_impl {
    protected:
        template <class ElemType>
        static int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1 = elem1.operator UElemType const&().num();
            auto const e2 = elem2.operator UElemType const&().num();
            return e1 == e2 ? 0 : (e1 < e2 ? -1 : 1);
        }
        explicit num_compare_impl (auto const &) {}
        num_compare_impl() = default;
    };

    class datetime_compare_impl {
    protected:  
        template <class ElemType>
        static inline int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1 = std::get<1>(elem1.operator UElemType const&().datetime(datetime_compare_impl::datetime_fmt));
            auto const e2 = std::get<1>(elem2.operator UElemType const&().datetime(datetime_compare_impl::datetime_fmt));
            return e1 == e2 ? 0 : (e1 < e2 ? -1 : 1);
        }
        explicit datetime_compare_impl(auto & args) {
            locale = args.num_locale;
            datetime_fmt = args.datetime_fmt;
        };
        datetime_compare_impl() = default;
    private:
        static std::string locale;
        static std::string datetime_fmt;
    };
    inline std::string datetime_compare_impl::locale;
    inline std::string datetime_compare_impl::datetime_fmt;

    class date_compare_impl {
    protected:
        template <class ElemType>
        static inline int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1 = std::get<1>(elem1.operator UElemType const&().date(date_compare_impl::date_fmt));
            auto const e2 = std::get<1>(elem2.operator UElemType const&().date(date_compare_impl::date_fmt));
            return e1 == e2 ? 0 : (e1 < e2 ? -1 : 1);
        }
        explicit date_compare_impl(auto & args) {
            locale = args.num_locale;
            date_fmt = args.date_fmt;
        };
        date_compare_impl() = default;
    private:
        static std::string locale;
        static std::string date_fmt;
    };
    inline std::string date_compare_impl::locale;
    inline std::string date_compare_impl::date_fmt;

    class text_compare_impl {
    protected:
        template <class ElemType>
        static int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            return elem1.operator UElemType const& ().compare(elem2.operator UElemType const& ());
        }
        explicit text_compare_impl (auto &) {}
        text_compare_impl() = default;
    };

    // ^^ !args.I && !has_blanks
    // ^^^ !args.blanks && !args.I && has_blanks
    // ^^^^ !args.blanks && args.I && has_blanks
    // * ...

    // blanks  I  has_blanks
    //    0    0   0          bool_compare_no_blanks_no_I_when_no_blanks ** (native compare)
    //    0    0   1          compare_no_blanks_no_I_when_blanks         ^^^  
    //    0    1   0          bool_compare_no_blanks_I_when_no_blanks    *
    //    0    1   1          compare_no_blanks_I_when_blanks            ^^^^
    //    1    0   0          bool_compare_blanks_no_I_when_no_blanks    ** (native compare)
    //    1    0   1          compare_blanks_no_I_when_blanks            *
    //    1    1   0          bool_compare_blanks_I_when_no_blanks       *
    //    1    1   1          bool_compare_blanks_I_when_blanks          *

    template <class ElemType, class Impl>
    struct compare_host_class : common_compare_impl<ElemType, Impl> {
        using common_compare_class_type = common_compare_impl<ElemType, Impl>;
        using common_compare_class_type::operator();
        compare_host_class() = default;
        compare_host_class(auto const & args, bool has_blanks) : common_compare_class_type(args, has_blanks) {}
        auto clone (auto const & args, bool has_blanks) {
            return compare_host_class(args, has_blanks);
        }
    };

    template <class ElemType>
    auto obtain_compare_functionality (auto const & ids, auto const & types_blanks, auto const & args ) {
        using bc_fun  = compare_host_class<ElemType, bool_compare_impl>;
        using tdc_fun = compare_host_class<ElemType, timedelta_compare_impl>;
        using nc_fun  = compare_host_class<ElemType, num_compare_impl>;
        using dtc_fun = compare_host_class<ElemType, datetime_compare_impl>;
        using dc_fun  = compare_host_class<ElemType, date_compare_impl>;
        using tc_fun  = compare_host_class<ElemType, text_compare_impl>;

        using compare_fun = std::variant<bc_fun, tdc_fun, nc_fun, dtc_fun, dc_fun, tc_fun>;
        
        using col_t = column_type; 
        std::unordered_map<col_t, compare_fun> hash {{col_t::bool_t, bc_fun()}, {col_t::number_t, nc_fun()}, {col_t::datetime_t, dtc_fun()}, {col_t::date_t, dc_fun()}, {col_t::timedelta_t, tdc_fun()}, {col_t::text_t, tc_fun()} };

        std::vector<std::tuple<unsigned, compare_fun>> result;
        auto const [types, blanks] = types_blanks;

        for (auto elem : ids) {
            std::visit([&](auto & arg) {
                result.push_back({elem, arg.clone(args, std::get<1>(types_blanks)[elem])});
            }, hash[types[elem]]);
        }

        return result;
    }

}

#endif
