///
/// \file   suite/include/cli-compare.h
/// \author wiluite
/// \brief  All possible comparison classes, operations: type-aware and type-independent.

#pragma once

namespace csvsuite::cli::compare::detail {

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
            return elem1.operator UElemType const& ().text_compare(elem2.operator UElemType const& ());
        }

        template <class ElemType>
        static inline int compare_no_blanks_no_I_when_blanks (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1_is_null = elem1.operator UElemType const &().is_null_or_null_value();
            auto const e2_is_null = elem2.operator UElemType const &().is_null_or_null_value();
            return (e1_is_null && e2_is_null) ? 0 : (e1_is_null ? 1 : (e2_is_null ? -1 : Native::native_compare(elem1, elem2)));
        }

        template <class ElemType>
        static int compare_no_blanks_I_when_blanks (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            auto const e1_is_null = elem1.operator UElemType const &().is_null_or_null_value();
            auto const e2_is_null = elem2.operator UElemType const &().is_null_or_null_value();
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
            auto const e1 = (ue1.is_boolean(), static_cast<bool>(ue1.unsafe()));
            auto const e2 = (ue2.is_boolean(), static_cast<bool>(ue2.unsafe()));
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
            datetime_fmt = args.datetime_fmt;
        };
        datetime_compare_impl() = default;
    private:
        static std::string datetime_fmt;
    };
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
            date_fmt = args.date_fmt;
        };
        date_compare_impl() = default;
    private:
        static std::string date_fmt;
    };
    inline std::string date_compare_impl::date_fmt;

    class text_compare_impl {
    protected:
        template <class ElemType>
        static int native_compare (ElemType const & elem1, ElemType const & elem2) {
            using UElemType = typename ElemType::template rebind<csv_co::unquoted>::other;
            return elem1.operator UElemType const& ().text_compare(elem2.operator UElemType const& ());
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
}

namespace csvsuite::cli::compare {

    using namespace detail;

    template<class type>
    using bc_fun = compare_host_class<type, bool_compare_impl>;

    template<class type>
    using tdc_fun = compare_host_class<type, timedelta_compare_impl>;

    template<class type>
    using nc_fun = compare_host_class<type, num_compare_impl>;

    template<class type>
    using dtc_fun = compare_host_class<type, datetime_compare_impl>;

    template<class type>
    using dc_fun = compare_host_class<type, date_compare_impl>;

    template<class type>
    using tc_fun = compare_host_class<type, text_compare_impl>;

    template<class type>
    using compare_fun = std::variant<bc_fun<type>, tdc_fun<type>, nc_fun<type>, dtc_fun<type>, dc_fun<type>, tc_fun<type>>;

    template <class ElemType>
    using column_fun_tuple = std::tuple<unsigned, compare_fun<ElemType>>;

    using col_t = column_type;
    template<typename E>
    std::unordered_map<column_type, compare_fun<E>> column_fun_hash = {
        {col_t::bool_t, bc_fun<E>()}, {col_t::number_t, nc_fun<E>()}, {col_t::datetime_t, dtc_fun<E>()}
        , {col_t::date_t, dc_fun<E>()}, {col_t::timedelta_t, tdc_fun<E>()}, {col_t::text_t, tc_fun<E>()}
    };

    template <class ElemType>
    auto obtain_compare_functionality (auto const & ids, auto const & types_blanks, auto const & args ) {
        std::vector<column_fun_tuple<ElemType>> result;

        auto const types = std::get<0>(types_blanks);
        auto const blanks = std::get<1>(types_blanks);

        for (auto elem : ids) {
            std::visit([&](auto & arg) {
                result.push_back({elem, arg.clone(args, blanks[elem])});
            }, column_fun_hash<ElemType>[types[elem]]);
        }

        return result;
    }

    // Overrides the above one for an unique key/column
    template <class ElemType>
    auto obtain_compare_functionality (unsigned id, auto const & types_blanks, auto const & args ) {
        column_fun_tuple<ElemType> result;

        auto const types = std::get<0>(types_blanks);
        auto const blanks = std::get<1>(types_blanks);

        std::visit([&](auto & arg) {
            result = {id, arg.clone(args, blanks[id])};
        }, column_fun_hash<ElemType>[types[id]]);

        return result;
    }

    template <class CF, class CPP_COMP=std::less<>>
    class sort_comparator {
        CF compare_fun_;
        CPP_COMP cpp_cmp;
    public:
        bool operator()(auto & a, auto & b) {
            for (auto & elem : compare_fun_) {

                auto & col = std::get<0>(elem);
                auto & fun = std::get<1>(elem);

                int result;
                std::visit([&](auto & c_cmp) { result = c_cmp(a[col], b[col]); }, fun);
                if (result)
                    return cpp_cmp(result, 0);
            }
            return false;
        }
        sort_comparator(CF cf, CPP_COMP cmp) : compare_fun_(std::move(cf)), cpp_cmp(std::move(cmp)) {}
    };

    // partial specialization for single key/column comparison
    template <class ElemType, class CPP_COMP>
    class sort_comparator<column_fun_tuple<ElemType>, CPP_COMP> {
        column_fun_tuple<ElemType> compare_fun_;
        CPP_COMP cpp_cmp;
    public:
        bool operator()(auto & a, auto & b) {
            auto & col = std::get<0>(compare_fun_);
            int result;
            std::visit([&](auto & c_cmp) {
                result = c_cmp(a[col], b[col]);
            }, std::get<1>(compare_fun_));

            return result != 0 && cpp_cmp(result, 0);
        }
        sort_comparator(column_fun_tuple<ElemType> cf, CPP_COMP cmp) : compare_fun_(std::move(cf)), cpp_cmp(std::move(cmp)) {}
    };

    template <class R, class Args, bool HibernateToFirstRow = false, bool Quoted_or_not=csv_co::quoted>
    class compromise_table_MxN {
    public:
        using element_type = typename std::decay_t<R>::template typed_span<Quoted_or_not>;
    private:
        using field_array = std::vector<element_type>;
        using table = std::vector<field_array>;
        std::unique_ptr<table> impl;
    public:
        explicit compromise_table_MxN(R & reader, Args const & args) {
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
                throw no_body_exception("compromise_table_MxN constructor. No data rows.", static_cast<unsigned>(header_size));

            impl = std::make_unique<table>(rest_rows, field_array(header_size));

            std::size_t row = 0;
            auto & impl_ref = *impl;
            reader.run_rows([&] (auto & row_span) {
                unsigned i = 0;
                for (auto & elem : row_span)
                    impl_ref[row][i++] = elem;
                row++;
            });
        }
        decltype (auto) operator[](size_t r) {
            return (*impl)[r];
        }
        [[nodiscard]] auto rows() const {
            return (*impl).size();
        }
        [[nodiscard]] auto cols() const {
            return (*impl)[0].size();
        }

        [[nodiscard]] auto cbegin() const { return impl->cbegin(); }
        [[nodiscard]] auto cend() const { return impl->cend(); }
        auto begin() { return impl->begin(); }
        auto end() { return impl->end(); }
        compromise_table_MxN (compromise_table_MxN && other) noexcept = default;
        auto operator=(compromise_table_MxN && other) noexcept -> compromise_table_MxN & = default;
    };
    static_assert(!std::is_copy_constructible<compromise_table_MxN<csv_co::reader<>,ARGS>>::value);
    static_assert(std::is_move_constructible<compromise_table_MxN<csv_co::reader<>,ARGS>>::value);

    template <typename R>
    using typed_span_t = typename R::template typed_span<csv_co::quoted>;

    template <typename R, typename F = column_fun_tuple<typed_span_t<R>>>
    struct equal_range_comparator {
        explicit equal_range_comparator(F const & f) : column(std::get<0>(f)), cmp_func(std::get<1>(f)) {}
        bool operator()(typed_span_t<R> key, const std::vector<typed_span_t<R>>& v) const {
            int result;
            std::visit([&](auto & c_cmp) {
                result = c_cmp(key, v[column]);
            }, cmp_func);

            return result != 0 && std::less<>()(result, 0);
        }

        bool operator()(const std::vector<typed_span_t<R>>& v, typed_span_t<R> key) const {
            int result;
            std::visit([&](auto & c_cmp) {
                result = c_cmp(v[column], key);
            }, cmp_func);

            return result != 0 && std::less<>()(result, 0);
        }
    private:
        unsigned column;
        compare_fun<typed_span_t<R>> cmp_func;
    };

}

