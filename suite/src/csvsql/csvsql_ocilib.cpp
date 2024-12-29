///
/// \file   suite/src/csvsql/csvsql_soci.cpp
/// \author wiluite
/// \brief  Parts of the implementation of the csvSql utility based on ocilib (for Oracle).

namespace ocilib_client_ns {
    class table_creator {
    public:
        explicit table_creator (auto const & args, Connection & con) {
            if (!args.no_create) {
                std::string s = create_table_composer::table();
                Statement st(con);
                st.Execute(std::string{s.cbegin(), s.cend() - 2});
            }
        }
    };

    class table_inserter {
        template<template<class...> class F, class L> struct mp_transform_impl;
        template<template<class...> class F, class L>
        using mp_transform = typename mp_transform_impl<F, L>::type;
        template<template<class...> class F, template<class...> class L, class... T>
        struct mp_transform_impl<F, L<T...>> {
            using type = L<F<T>...>;
        };

        inline static unsigned value_index;
        void static reset_value_index() {
            value_index = 0;
        }

        using generic_bool = int;

        static inline auto fill_date_time = [](date::sys_time<std::chrono::seconds> tp, Timestamp & tstamp) {
            auto const day_point = floor<date::days>(tp);
            date::year_month_day ymd = day_point;
            date::hh_mm_ss tod {tp - day_point};
            tstamp.SetDateTime(int(ymd.year()), static_cast<int>(static_cast<unsigned>(ymd.month()))
                , static_cast<int>(static_cast<unsigned>(ymd.day())), static_cast<int>(tod.hours().count())
                , static_cast<int>(tod.minutes().count()), static_cast<int>(tod.seconds().count()), 0);
        };
        static inline auto fill_date = [](date::sys_time<std::chrono::seconds> tp, Date & date) {
            date::year_month_day ymd = floor<date::days>(tp);
            date.SetDate(int(ymd.year()), static_cast<int>(static_cast<unsigned>(ymd.month()))
                , static_cast<int>(static_cast<unsigned>(ymd.day())));
        };
        static inline auto fill_interval = [](long double secs, Interval & interval) {
            std::chrono::duration<long double, std::ratio<1>> src (secs);
            int days_ = static_cast<decltype(days_)>(floor<std::chrono::days>(src).count());
            src -= floor<std::chrono::days>(src);
            int clock_hours_ = static_cast<decltype(clock_hours_)>(floor<std::chrono::hours>(src).count());
            src -= floor<std::chrono::hours>(src);
            int clock_minutes_ = static_cast<decltype(clock_minutes_)>(floor<std::chrono::minutes>(src).count());
            src -= floor<std::chrono::minutes>(src);
            int clock_seconds_ = static_cast<decltype(clock_seconds_)>(floor<std::chrono::seconds>(src).count());
            src -= floor<std::chrono::seconds>(src);
            auto rest_ = src;
            interval.SetDaySecond(days_, clock_hours_, clock_minutes_, clock_seconds_, static_cast<int>(round<std::chrono::nanoseconds>(rest_).count()));
        };

        class simple_inserter {
            using db_types=std::variant<double, std::string, Date, Timestamp, Interval, generic_bool>;
            using db_types_ptr = mp_transform<std::add_pointer_t, db_types>;

            std::vector<db_types> data_holder;

            using ct = column_type;
            std::unordered_map<ct, db_types> type2value = {{ct::bool_t, generic_bool{}}, {ct::text_t, std::string{}}
                , {ct::number_t, double{}}, {ct::datetime_t, Timestamp{Timestamp::NoTimeZone}}
                , {ct::date_t, Date{"1970-01-01", "YYYY-MM-DD"}}, {ct::timedelta_t, Interval{Interval::DaySecond}}
            };

            table_inserter & parent_;
            Connection & con;
            create_table_composer & composer_;

            void insert_data(auto & reader, create_table_composer & composer, Statement & stmt) {
                using reader_type = std::decay_t<decltype(reader)>;
                using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                using func_type = std::function<void(elem_type const&)>;

                auto col = 0u;
                std::array<func_type, static_cast<std::size_t>(column_type::sz)> fill_funcs {
                        [](elem_type const &) { assert(false && "this is unknown data type, logic error."); }
                        , [&](elem_type const & e) {
                            std::get<5>(data_holder[col]) = static_cast<generic_bool>(e.is_boolean(), static_cast<bool>(e.unsafe()));
                        }
                        , [&](elem_type const & e) {
                            std::get<0>(data_holder[col]) = static_cast<double>(e.num());
                        }
                        , [&](elem_type const & e) {
                            fill_date_time(std::get<1>(e.datetime()), std::get<3>(data_holder[col]));
                        }
                        , [&](elem_type const & e) {
                            fill_date(std::get<1>(e.date()), std::get<2>(data_holder[col]));
                        }
                        , [&](elem_type const & e) {
                            fill_interval(e.timedelta_seconds(), std::get<4>(data_holder[col]));
                        }
                        , [&](elem_type const & e) {
                            std::get<1>(data_holder[col]) = clarify_text(e);
                        }
                };

                reader.run_rows([&] (auto & row_span) {
                    col = 0u;
                    for (auto & elem : row_span) {
                        auto e {elem_type{elem}};
                        if (e.is_null())
                            stmt.GetBind(col + 1).SetDataNull(true, 1);
                        else {
                            fill_funcs[static_cast<std::size_t>(composer.types()[col])](e);
                            stmt.GetBind(col + 1).SetDataNull(false, 1);
                        }
                        col++;
                    }
                    stmt.ExecutePrepared();
                });
            }

            void prepare_statement_object(auto const & args, Statement & st) {
                st.Prepare(insert_expr(parent_.insert_prefix_, args));
                reset_value_index();

                auto prepare_next_arg = [&](auto arg) -> db_types_ptr& {
                    static db_types_ptr each_next;
                    data_holder[value_index] = type2value[arg];
                    std::visit([&](auto & arg) {
                        each_next = &arg;
                    }, data_holder[value_index]);
                    return each_next;
                };

                for(auto e : composer_.types()) {
                    std::visit([&](auto & arg) {
                        std::string name = ":v" + std::to_string(value_index);
                        if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, std::string>)
                            st.Bind(name, *arg, 200, BindInfo::In);
                        else
                            st.Bind(name, *arg, BindInfo::In);
                    }, prepare_next_arg(e));
                    value_index++;
                }
            }

        public:
            simple_inserter(table_inserter & parent, Connection & con, create_table_composer & composer) : parent_(parent), con(con), composer_(composer) {}
            void insert(auto const &args, auto & reader) {
                data_holder.resize(composer_.types().size());
                Statement stmt(con);
                prepare_statement_object(args, stmt);
                insert_data(reader, composer_, stmt);
            }
        };

        class batch_bulk_inserter {
            using db_types=std::variant<std::vector<double>, std::vector<std::string>, std::vector<Date>, std::vector<Timestamp>, std::vector<Interval>, std::vector<generic_bool>>;
            using db_types_ptr = mp_transform<std::add_pointer_t, db_types>;

            std::vector<db_types> data_holder;

            using ct = column_type;
            std::unordered_map<ct, db_types> type2value = {{ct::bool_t, std::vector<generic_bool>{}}, {ct::text_t, std::vector<std::string>{}}
                    , {ct::number_t, std::vector<double>{}}, {ct::datetime_t, std::vector<Timestamp>{}}
                    , {ct::date_t, std::vector<Date>{}}, {ct::timedelta_t, std::vector<Interval>{}}
            };

            table_inserter & parent_;
            Connection & con;
            create_table_composer & composer_;

            void insert_data(auto & reader, create_table_composer & composer, auto const & args, Statement & stmt) {
                using reader_type = std::decay_t<decltype(reader)>;
                using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                using func_type = std::function<void(elem_type const&)>;

                auto col = 0u;
                auto offset = 0u;
                std::array<func_type, static_cast<std::size_t>(column_type::sz)> fill_funcs {
                        [](elem_type const &) { assert(false && "this is unknown data type, logic error."); }
                        , [&](elem_type const & e) {
                            std::get<5>(data_holder[col])[offset] = static_cast<generic_bool>(e.is_boolean(), static_cast<bool>(e.unsafe()));
                        }
                        , [&](elem_type const & e) {
                            std::get<0>(data_holder[col])[offset] = static_cast<double>(e.num());
                        }
                        , [&](elem_type const & e) {
                            fill_date_time(std::get<1>(e.datetime()), std::get<3>(data_holder[col])[offset]);
                        }
                        , [&](elem_type const & e) {
                            fill_date(std::get<1>(e.date()), std::get<2>(data_holder[col])[offset]);
                        }
                        , [&](elem_type const & e) {
                            fill_interval(e.timedelta_seconds(), std::get<4>(data_holder[col])[offset]);
                        }
                        , [&](elem_type const & e) {
                            std::get<1>(data_holder[col])[offset] = clarify_text(e);
                        }
                };
                reader.run_rows([&] (auto & row_span) {
                    col = 0u;
                    for (auto & elem : row_span) {
                        auto e {elem_type{elem}};
                        if (e.is_null())
                            stmt.GetBind(col + 1).SetDataNull(true, offset + 1);
                        else {
                            fill_funcs[static_cast<std::size_t>(composer.types()[col])](e);
                            stmt.GetBind(col + 1).SetDataNull(false, offset + 1);
                        }
                        col++;
                    }
                    if (++offset == args.chunk_size) {
                        stmt.ExecutePrepared();
                        offset = 0;
                    }
                });
                if (offset) {
                    for (auto & vec : data_holder)
                        std::visit([&](auto & arg) {
                            arg.resize(offset);
                        }, vec);
                    stmt.SetBindArraySize(offset);
                    stmt.ExecutePrepared();
                }
            }

            void prepare_statement_object(auto const & args, Statement & st) {
                st.Prepare(insert_expr(parent_.insert_prefix_, args));
                st.SetBindArraySize(args.chunk_size);
                reset_value_index();

                auto prepare_next_arg = [&](auto arg) -> db_types_ptr& {
                    static db_types_ptr each_next;
                    data_holder[value_index] = type2value[arg];
                    std::visit([&](auto & arg) {
                        if constexpr(std::is_same_v<std::decay_t<decltype(arg)>, std::vector<Date>>)
                            // Note: it seems arg.resize(args.chunk_size, Date::SysDate());
                            // is not necessary for elems to be considered "bound" in this array to bind it later.
                            // We need something like:
                            for (auto i = 0u; i < args.chunk_size; i++)
                                arg.push_back(Date::SysDate());
                        else
                        if constexpr(std::is_same_v<std::decay_t<decltype(arg)>, std::vector<Timestamp>>)
                            // see above
                            for (auto i = 0u; i < args.chunk_size; i++)
                                arg.push_back(Timestamp{Timestamp::NoTimeZone});
                        else
                        if constexpr(std::is_same_v<std::decay_t<decltype(arg)>, std::vector<Interval>>)
                            // see above
                            for (auto i = 0u; i < args.chunk_size; i++)
                                arg.push_back(Interval{Interval::DaySecond});
                        else
                            arg.resize(args.chunk_size);
                        each_next = addressof(arg);
                    }, data_holder[value_index]);
                    return each_next;
                };

                for(auto e : composer_.types()) {
                    std::visit([&](auto & arg) {
                        std::string name = ":v" + std::to_string(value_index);
                        if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, std::vector<std::string>>)
                            st.Bind(name, *arg, 200, BindInfo::In);
                        else
                        if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, std::vector<Timestamp>>)
                            st.Bind(name, *arg, Timestamp::NoTimeZone, BindInfo::In);
                        else
                        if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, std::vector<Interval>>)
                            st.Bind(name, *arg, Interval::DaySecond, BindInfo::In);
                        else
                            st.Bind(name, *arg, BindInfo::In);
                    }, prepare_next_arg(e));
                    value_index++;
                }
            }

        public:
            batch_bulk_inserter (table_inserter & parent,  Connection & con, create_table_composer & composer)
                    : parent_(parent), con(con), composer_(composer) {}
            void insert(auto const &args, auto & reader) {
                data_holder.resize(composer_.types().size());
                Statement stmt(con);
                prepare_statement_object(args, stmt);
                insert_data(reader, composer_, args, stmt);
            }

        };

        Connection & con;
        create_table_composer & composer_;
        std::string const & after_insert_;
        std::string const & insert_prefix_;

    public:
        table_inserter(auto const &args, Connection & con, create_table_composer & composer)
                : con(con), composer_(composer), after_insert_(args.after_insert), insert_prefix_(args.prefix) {
        }
        ~table_inserter() {
            if (!after_insert_.empty()) {
                auto const statements = sql_split(std::stringstream(after_insert_));
                Statement stmt(con);
                for (auto & elem : statements)
                    stmt.Execute(elem);
            }
        }
        void insert(auto const &args, auto & reader) {
            if (args.chunk_size <= 1)
                simple_inserter(*this, con, composer_).insert(args, reader);
            else {
                batch_bulk_inserter(*this, con, composer_).insert(args, reader);
            }
        }
    };

    class query {
    public:
        query(auto const & args, Connection & con) {
            if (!args.query.empty()) {
                auto q_array = queries(args);
                Statement stmt(con);
                std::for_each(q_array.begin(), q_array.end() - 1, [&](auto & elem) {
                    stmt.Execute(elem);
                });

                csvsuite::cli::sql::rowset_query(con, args, q_array.back());
            }
        }
    };


}
