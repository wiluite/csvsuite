///
/// \file   suite/include/cli-hash.h
/// \author wiluite
/// \brief  All possible hash classes, operations: type-aware and type-independent.

#pragma once

namespace csvsuite::cli::hash::detail {

    template <class E, class Native>
    class common_hash_impl : Native {
    protected:
        std::size_t operator()(E const & e) const {
            return std_fun(e);
        }

    private:
        template <class E>
        static inline std::size_t hash_blanks_no_I_when_blanks (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            return std::hash<std::string>{}(e.operator UElemType const& ().str());
        }

        template <class E>
        static inline std::size_t hash_no_blanks_no_I_when_blanks (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            auto const e_is_null = e.operator UElemType const &().is_null_or_null_value();
            return e_is_null ? 0 : Native::native_hash(e);
        }

        template <class E>
        static std::size_t hash_no_blanks_I_when_blanks (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            auto const e_is_null = e.operator UElemType const &().is_null_or_null_value();
            return e_is_null ? 0 : hash_blanks_no_I_when_blanks(e);
        }

    protected:
        using class_type = common_hash_impl<E, Native>;

        common_hash_impl(auto & args, bool has_blanks) : Native(args) {
            if (!args.no_inference && !has_blanks) {
                std_fun = class_type::template native_hash<E>;
                return;
            }
            if (!args.blanks && !args.no_inference && has_blanks) {
                std_fun = class_type::template hash_no_blanks_no_I_when_blanks<E>;
                return;
            }
            if (!args.blanks && args.no_inference && has_blanks) {
                std_fun = class_type::template hash_no_blanks_I_when_blanks<E>;
                return;
            }
            std_fun = class_type::template hash_blanks_no_I_when_blanks<E>;
        }
        common_hash_impl() = default;

    private:
        std::function<std::size_t(E const &)> std_fun;
    };

    class bool_hash_impl {
    protected:
        template <class E>
        static std::size_t native_hash (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            UElemType const & ue = e;
            auto const value = (ue.is_boolean(), static_cast<bool>(ue.unsafe()));
            return std::hash<bool>{}(value);
        }
        explicit bool_hash_impl(auto const & ) {}
        bool_hash_impl() = default;
    };

    class timedelta_hash_impl {
    protected:
        template <class E>
        static std::size_t native_hash (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            auto const value = e.operator UElemType const&().timedelta_seconds();
            return std::hash<long double>{}(value);
        }
        explicit timedelta_hash_impl (auto const & ) {}
        timedelta_hash_impl() = default;
    };

    class num_hash_impl {
    protected:
        template <class E>
        static std::size_t native_hash (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            auto const value = e.operator UElemType const&().num();
            return std::hash<long double>{}(value);
        }
        explicit num_hash_impl (auto const &) {}
        num_hash_impl() = default;
    };

    class datetime_hash_impl {
    protected:
        template <class E>
        static inline std::size_t native_hash (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            auto const value = std::get<1>(e.operator UElemType const&().datetime(datetime_hash_impl::datetime_fmt));
            return std::hash<std::size_t>{}(value.time_since_epoch().count());
        }
        explicit datetime_hash_impl(auto & args) {
            datetime_fmt = args.datetime_fmt;
        };
        datetime_hash_impl() = default;
    private:
        static std::string datetime_fmt;
    };
    inline std::string datetime_hash_impl::datetime_fmt;

    class date_hash_impl {
    protected:
        template <class E>
        static inline std::size_t native_hash (E const & elem1) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const value = std::get<1>(e.operator UElemType const&().date(date_hash_impl::date_fmt));
            return std::hash<std::size_t>{}(value.time_since_epoch().count());
        }
        explicit date_hash_impl(auto & args) {
            date_fmt = args.date_fmt;
        };
        date_hash_impl() = default;
    private:
        static std::string date_fmt;
    };
    inline std::string date_hash_impl::date_fmt;

    class text_hash_impl {
    protected:
        template <class E>
        static std::size_t native_hash (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
            return std::hash<std::string>{}(e.operator UElemType const& ().str());
        }
        explicit text_hash_impl (auto &) {}
        text_hash_impl() = default;
    };

    template <class E, class Impl>
    struct hash_host_class : common_hash_impl<E, Impl> {
        using common_hash_class_type = common_hash_impl<E, Impl>;
        using common_hash_class_type::operator();
        hash_host_class() = default;
        hash_host_class(auto const & args, bool has_blanks) : common_hash_class_type(args, has_blanks) {}
        auto clone (auto const & args, bool has_blanks) {
            return hash_host_class(args, has_blanks);
        }
    };
}

namespace csvsuite::cli::hash {

    using namespace detail;

    template<class T>
    using bh_fun = hash_host_class<T, bool_hash_impl>;

    template<class T>
    using tdh_fun = hash_host_class<T, timedelta_hash_impl>;

    template<class T>
    using nh_fun = hash_host_class<T, num_hash_impl>;

    template<class T>
    using dth_fun = hash_host_class<T, datetime_hash_impl>;

    template<class T>
    using dh_fun = hash_host_class<T, date_hash_impl>;

    template<class T>
    using th_fun = hash_host_class<T, text_hash_impl>;

    template<class T>
    using hash_fun = std::variant<bh_fun<T>, tdh_fun<T>, nc_fun<T>, dth_fun<T>, dh_fun<T>, th_fun<T>>;

    using col_t = column_type;
    template<typename E>
    std::unordered_map<column_type, hash_fun<E>> column_fun_hash = {
        {col_t::bool_t, bh_fun<E>()}, {col_t::number_t, nh_fun<E>()}, {col_t::datetime_t, dth_fun<E>()}
        , {col_t::date_t, dh_fun<E>()}, {col_t::timedelta_t, tdh_fun<E>()}, {col_t::text_t, th_fun<E>()}
    };

    template <class E>
    auto obtain_hash_functionality(unsigned column, auto const & types_blanks, auto const & args ) {
        hash_fun<E> result;

        auto const types = std::get<0>(types_blanks);
        auto const blanks = std::get<1>(types_blanks);

        std::visit([&](auto & arg) {
            result = arg.clone(args, blanks[column]);
        }, column_fun_hash<E>[types[column]]);

        return result;
    }

    template <class E, class ComparePair>
    class equality_checker {
        std::decay_t<decltype(std::get<1>(std::declval<ComparePair>()))> c_lang_compare_function;
    public:
        bool is_equal(auto & this_one, auto & other) {
            int result;
            std::visit([&](auto & arg) {
                result = arg(this_one, other);
            }, c_lang_compare_function);
            return result == 0;
        }
        // only getting function from a column-function pair
        equality_checker(ComparePair & compare_pair) : c_lang_compare_function(std::get<1>(compare_pair)) {}
    };
}

