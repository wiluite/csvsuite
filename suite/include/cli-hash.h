///
/// \file   suite/include/cli-hash.h
/// \author wiluite
/// \brief  All possible hash classes, operations: type-aware and type-independent.

#pragma once
#include "cli-compare.h"

namespace csvsuite::cli::hash::detail {

    template <class EType, class Native>
    class common_hash_impl : Native {
    protected:
        std::size_t operator()(EType const & e) const {
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
        using class_type = common_hash_impl<EType, Native>;

        common_hash_impl(auto & args, bool has_blanks) : Native(args) {
            if (!args.no_inference && !has_blanks) {
                std_fun = class_type::template native_hash<EType>;
                return;
            }
            if (!args.blanks && !args.no_inference && has_blanks) {
                std_fun = class_type::template hash_no_blanks_no_I_when_blanks<EType>;
                return;
            }
            if (!args.blanks && args.no_inference && has_blanks) {
                std_fun = class_type::template hash_no_blanks_I_when_blanks<EType>;
                return;
            }
            std_fun = class_type::template hash_blanks_no_I_when_blanks<EType>;
        }
        common_hash_impl() = default;

    private:
        std::function<std::size_t(EType const &)> std_fun;
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
        static inline std::size_t native_hash (E const & e) {
            using UElemType = typename E::template rebind<csv_co::unquoted>::other;
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
    using namespace csvsuite::cli::compare;

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
    using hash_fun = std::variant<bh_fun<T>, tdh_fun<T>, nh_fun<T>, dth_fun<T>, dh_fun<T>, th_fun<T>>;

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
        bool is_equal(auto & this_one, auto & other) const {
            int result;
            std::visit([&](auto & arg) {
                result = arg(this_one, other);
            }, c_lang_compare_function);
            return result == 0;
        }
        // only getting function from a column-function pair
        equality_checker(ComparePair & compare_pair) : c_lang_compare_function(std::get<1>(compare_pair)) {}
    };

    // E -- typed_span
    template<class E, class ComparePair>
    struct Hashable : E {
        bool operator==(Hashable const& other) const {
#if TRACE_
            std::cerr << "bool operator==(Hashable)\n";
#endif
            assert(eq_checker);
            auto result = eq_checker->is_equal(*this, other);
#if TRACE_
            std::cerr << "bool operator==(Hashable) finished\n";
#endif
            return result;
        }
        static void create_equality_checker(ComparePair compare_functionality) {
            eq_checker = std::make_unique<equality_checker<E, ComparePair>> (compare_functionality);
        }
    private:
        inline static std::unique_ptr<equality_checker<E, ComparePair>> eq_checker = nullptr;
    };
    template <class E>
    using hashable_typed_span = Hashable<E, column_fun_tuple<E>>;

    template<class E, class ComparePair, class HashFun>
    struct Hash
    {
        std::size_t operator()(Hashable<E, ComparePair> const & value) const noexcept
        {
#if TRACE_
            std::cerr << "std::size_t operator()(Hashable)\n";
#endif
            std::size_t result;
            std::visit([&](auto & arg) {
                result = arg(value);
            }, hashfun);
#if TRACE_
            std::cerr << "std::size_t operator()(Hashable) finished\n";
#endif
            return result;
        }
        static void assign_hash_function(HashFun value) {
            hashfun = std::move(value);
        }
    private:
        inline static HashFun hashfun;
    };

    template<class E>
    using hash = Hash<E, column_fun_tuple<E>, hash_fun<E>>;

    template<class E>
    using field_array = std::vector<E>;

    template<class E>
    using hash_map_value = std::vector<field_array<E>>;

    template<class E>
    using hash_map = std::unordered_map<hashable_typed_span<E>, hash_map_value<E>, hash<E>>;

    template <class R, class Args, bool HibernateToFirstRow = false, bool Quoted_or_not=csv_co::quoted>
    class compromise_hash {
    public:
        using typed_span = typename std::decay_t<R>::template typed_span<Quoted_or_not>;
        using key_type = hashable_typed_span<typed_span>;
        using value_type = hash_map_value<typed_span>;
        using hashing = hash<typed_span>;
    private:
        hash_map<typed_span> map_;
    public:
        explicit compromise_hash(R & reader, Args const & args, auto const & types_blanks, unsigned hash_column) {
            using namespace csv_co;

            struct hibernator {
                explicit hibernator(R & reader, Args const & args) : reader_(reader), args_(args) { reader_.skip_rows(0); }
                ~hibernator() {
                    reader_.skip_rows(0);
                    if constexpr(HibernateToFirstRow)
                        obtain_header_and_<skip_header>(reader_, args_);
                }
            private:
                R & reader_;
                Args const & args_;
            } h(reader, args);

            skip_lines(reader, args);
            auto const rest_rows = reader.rows() - (args.no_header ? 0 : 1);
            auto const header_size = obtain_header_and_<skip_header>(reader, args).size();
            if (!rest_rows)
                throw no_body_exception("compromise_hash constructor. No data rows.", static_cast<unsigned>(header_size));

            key_type::create_equality_checker(obtain_compare_functionality<typed_span>(hash_column, types_blanks, args));

            hashing::assign_hash_function(obtain_hash_functionality<typed_span>(hash_column, types_blanks, args));

            key_type key;
            field_array<typed_span> row;

            reader.run_rows([&] (auto & row_span) {
                unsigned i = 0;
                for (auto & elem : row_span) {
                    if (i++ == hash_column)
                        key = key_type{typed_span{elem}};
                    row.push_back(typed_span{elem});
                }
                assert(row.size() == std::get<0>(types_blanks).size());
                map_[key].push_back(std::move(row));
            });
        }

        auto const & hash() const {
            return map_;
        }
    };

}
