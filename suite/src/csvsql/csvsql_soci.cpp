//
// Created by wiluite on 21.09.2024.
//

namespace soci_client_ns {
    class table_creator {
    public:
        explicit table_creator (auto const & args, soci::session & sql) {
            if (!args.no_create) {
                sql.begin();
                sql << create_table_composer::table();                
                sql.commit();
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

        using generic_bool = int32_t;

        static inline auto fill_date = [](std::tm & tm_, auto & day_point) {
            using namespace date;
            year_month_day ymd = day_point;
            tm_.tm_year = int(ymd.year()) - 1900;
            tm_.tm_mon = static_cast<int>(static_cast<unsigned>(ymd.month())) - 1;
            tm_.tm_mday = static_cast<int>(static_cast<unsigned>(ymd.day()));
        };
        static inline auto fill_time = [](std::tm & tm_, auto const & tod) {
            using namespace date;
            tm_.tm_hour = static_cast<int>(tod.hours().count());
            tm_.tm_min = static_cast<int>(tod.minutes().count());
            tm_.tm_sec = static_cast<int>(tod.seconds().count());
        };

        class simple_inserter {
            using db_types=std::variant<double, std::string, std::tm, generic_bool>;
            using db_types_ptr = mp_transform<std::add_pointer_t, db_types>;

            std::vector<db_types> data_holder;
            std::vector<soci::indicator> indicators;

            using ct = column_type;
            std::unordered_map<ct, db_types> type2value = {{ct::bool_t, generic_bool{}}, {ct::text_t, std::string{}}
                    , {ct::number_t, double{}}, {ct::datetime_t, std::tm{}}, {ct::date_t, std::tm{}}, {ct::timedelta_t, std::tm{}}
            };

            table_inserter & parent_;
            soci::session & sql_;
            create_table_composer & composer_;
            SOCI::backend_id backend_id_ {SOCI::backend_id::ANOTHER};

            void insert_data(auto & reader, create_table_composer & composer, soci::statement & stmt) {
                using reader_type = std::decay_t<decltype(reader)>;
                using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                using func_type = std::function<void(elem_type const&)>;

                auto col = 0u;
                std::array<func_type, static_cast<std::size_t>(column_type::sz)> fill_funcs {
                        [](elem_type const &) { assert(false && "this is unknown data type, logic error."); }
                        , [&](elem_type const & e) {
                            std::get<3>(data_holder[col]) = static_cast<generic_bool>(e.is_boolean(), e.unsafe_bool());
                        }
                        , [&](elem_type const & e) {
                            std::get<0>(data_holder[col]) = static_cast<double>(e.num());
                        }
                        , [&](elem_type const & e) {
                            using namespace date;

                            date::sys_time<std::chrono::seconds> tp = std::get<1>(e.datetime());
                            auto day_point = floor<days>(tp);
                            std::tm tm_{};
                            fill_date(tm_, day_point);
                            fill_time(tm_, hh_mm_ss{tp - day_point});
                            std::get<2>(data_holder[col]) = tm_;
                        }
                        , [&](elem_type const & e) {
                            using namespace date;

                            date::sys_time<std::chrono::seconds> tp = std::get<1>(e.date());
                            auto day_point = floor<days>(tp);
                            std::tm tm_{};
                            fill_date(tm_, day_point);
                            std::get<2>(data_holder[col]) = tm_;
                        }
                        , [&](elem_type const & e) {
                            using namespace date;

                            long double secs = e.timedelta_seconds();
                            date::sys_time<std::chrono::seconds> tp(std::chrono::seconds(static_cast<int>(secs)));
                            auto day_point = floor<date::days>(tp);
                            std::tm t{};
                            switch (backend_id_) {
                                double int_part;
                                char buf[80];
                                case SOCI::backend_id::PG:
                                    fill_time(t, hh_mm_ss{tp - day_point});
                                    //TODO: fix it
                                    t.tm_isdst = static_cast<int>(std::modf(static_cast<double>(secs), &int_part) * 1000000);
                                    snprintf(buf, 80, "%d:%02d:%02d.%06d", day_point.time_since_epoch().count() * 24 + t.tm_hour, t.tm_min, t.tm_sec, t.tm_isdst);
                                    std::get<1>(data_holder[col]) = buf;
                                    break;
                                default:
                                    fill_date(t, day_point);
                                    fill_time(t, hh_mm_ss{tp - day_point});
                                    t.tm_isdst = static_cast<int>(std::modf(static_cast<double>(secs), &int_part) * 1000000);
                                    std::get<2>(data_holder[col]) = t;
                                    break;
                            }
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
                            indicators[col] = soci::i_null;
                        else {
                            fill_funcs[static_cast<std::size_t>(composer.types()[col])](e);
                            indicators[col] = soci::i_ok;
                        }
                        col++;
                    }
                    stmt.execute(true);
                });
            }

            auto prepare_statement_object(auto const & args) {
                auto prep = sql_.prepare.operator<<(insert_expr(parent_.insert_prefix_, args));
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
                        prep = std::move(prep.operator,(soci::use(*arg, indicators[value_index])));
                    }, prepare_next_arg(e));
                    value_index++;
                }
                return prep;
            }
        public:
            simple_inserter(table_inserter & parent, soci::session & sql, create_table_composer & composer)
                    : parent_(parent), sql_(sql), composer_(composer) {
                if (sql.get_backend_name() == "postgresql") {
                    backend_id_ = SOCI::backend_id::PG;
                    type2value[ct::timedelta_t] = std::string{};
                }
            }

            void insert(auto const &args, auto & reader) {
                data_holder.resize(composer_.types().size());
                indicators.resize(composer_.types().size(), soci::i_ok);

                sql_.begin();

                soci::statement stmt = prepare_statement_object(args);
                insert_data(reader, composer_, stmt);

                sql_.commit();
            }
        };

        class batch_bulk_inserter {
            using db_types=std::variant<std::vector<double>, std::vector<std::string>, std::vector<std::tm>, std::vector<generic_bool>>;
            using db_types_ptr = mp_transform<std::add_pointer_t, db_types>;
            static_assert(std::is_same_v<db_types_ptr, std::variant<std::vector<double>*, std::vector<std::string>*, std::vector<std::tm>*, std::vector<generic_bool>*>>);

            std::vector<db_types> data_holder;
            std::vector<std::vector<soci::indicator>> indicators;

            using ct = column_type;
            template <typename T>
            using vec = std::vector<T>;
            std::unordered_map<ct, db_types> type2value = {{ct::bool_t, vec<generic_bool>{}}, {ct::text_t, vec<std::string>{}}
                    , {ct::number_t, vec<double>{}}, {ct::datetime_t, vec<std::tm>{}}, {ct::date_t, vec<std::tm>{}}, {ct::timedelta_t, vec<std::tm>{}}
            };

            table_inserter & parent_;
            soci::session & sql_;
            create_table_composer & composer_;
            SOCI::backend_id backend_id_ {SOCI::backend_id::ANOTHER};

            void insert_data(auto & reader, create_table_composer & composer, auto const & args) {
                using reader_type = std::decay_t<decltype(reader)>;
                using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                using func_type = std::function<void(elem_type const&)>;

                std::function<void (soci::statement&, decltype(args) const &)> bulk_mode_fixer;
                if (backend_id_ == SOCI::backend_id::MYSQL or backend_id_ == SOCI::backend_id::PG)
                    bulk_mode_fixer = [&](soci::statement& stmt, decltype(args) & args) { stmt = prepare_statement_object(args, false);};
                else
                    bulk_mode_fixer = [](soci::statement&, decltype(args) const &) {};

                auto col = 0u;
                auto offset = 0u;
                std::array<func_type, static_cast<std::size_t>(column_type::sz)> fill_funcs {
                        [](elem_type const &) { assert(false && "this is unknown data type, logic error."); }
                        , [&](elem_type const & e) {
                            std::get<3>(data_holder[col])[offset] = static_cast<generic_bool>(e.is_boolean(), e.unsafe_bool());
                        }
                        , [&](elem_type const & e) {
                            std::get<0>(data_holder[col])[offset] = static_cast<double>(e.num());
                        }
                        , [&](elem_type const & e) {
                            using namespace date;

                            date::sys_time<std::chrono::seconds> tp = std::get<1>(e.datetime());
                            auto day_point = floor<days>(tp);
                            std::tm tm_{};
                            fill_date(tm_, day_point);
                            fill_time(tm_, hh_mm_ss{tp - day_point});
                            std::get<2>(data_holder[col])[offset] = tm_;
                        }
                        , [&](elem_type const & e) {
                            using namespace date;

                            date::sys_time<std::chrono::seconds> tp = std::get<1>(e.date());
                            auto day_point = floor<days>(tp);
                            std::tm tm_{};
                            fill_date(tm_, day_point);
                            std::get<2>(data_holder[col])[offset] = tm_;
                        }
                        , [&](elem_type const & e) {
                            using namespace date;

                            long double secs = e.timedelta_seconds();
                            date::sys_time<std::chrono::seconds> tp(std::chrono::seconds(static_cast<int>(secs)));
                            auto day_point = floor<date::days>(tp);
                            std::tm t{};

                            switch (backend_id_) {
                                double int_part;
                                char buf[80];
                                case SOCI::backend_id::PG:
                                    fill_time(t, hh_mm_ss{tp - day_point});
                                    //TODO: fix it
                                    t.tm_isdst = static_cast<int>(std::modf(static_cast<double>(secs), &int_part) * 1000000);
                                    snprintf(buf, 80, "%d:%02d:%02d.%06d", day_point.time_since_epoch().count() * 24 + t.tm_hour, t.tm_min, t.tm_sec, t.tm_isdst);
                                    std::get<1>(data_holder[col])[offset] = buf;
                                    break;
                                default:
                                    fill_date(t, day_point);
                                    fill_time(t, hh_mm_ss{tp - day_point});
                                    t.tm_isdst = static_cast<int>(std::modf(static_cast<double>(secs), &int_part) * 1000000);
                                    std::get<2>(data_holder[col])[offset] = t;
                                    break;
                            }
                        }
                        , [&](elem_type const & e) {
                            std::get<1>(data_holder[col])[offset] = clarify_text(e);
                        }
                };

                soci::statement stmt = prepare_statement_object(args);
                reader.run_rows([&] (auto & row_span) {
                    col = 0u;
                    for (auto & elem : row_span) {
                        auto e {elem_type{elem}};
                        if (e.is_null())
                            indicators[col][offset] = soci::i_null;
                        else {
                            fill_funcs[static_cast<std::size_t>(composer.types()[col])](e);
                            indicators[col][offset] = soci::i_ok;
                        }
                        col++;
                    }
                    if (++offset == args.chunk_size) {
                        stmt.execute(true);
                        bulk_mode_fixer(stmt, args);
                        offset = 0;
                    }
                });

                // Insert rest data less than the chunk
                if (offset) {
                    for (auto & vec : data_holder)
                        std::visit([&](auto & arg) {
                            arg.resize(offset);
                        }, vec);
                    for (auto & ind : indicators)
                        ind.resize(offset);
                    stmt.execute(true);
                }
            }

            auto prepare_statement_object(auto const & args, bool resize = true) {
                auto prep = sql_.prepare.operator<<(insert_expr(parent_.insert_prefix_, args));
                reset_value_index();

                for(auto e : composer_.types()) {
                    std::visit([&](auto & arg) {
                        prep = std::move(prep.operator,(soci::use(*arg, indicators[value_index])));
                    }, [&](auto arg) -> db_types_ptr & {
                        static db_types_ptr each_next;
                        if (resize)
                            data_holder[value_index] = type2value[arg];
                        std::visit([&](auto &arg) {
                            if (resize)
                                arg.resize(args.chunk_size);
                            each_next = &arg;
                        }, data_holder[value_index]);
                        return each_next;
                    }(e));
                    value_index++;
                }
                return prep;
            }

        public:
            batch_bulk_inserter (table_inserter & parent, soci::session & sql, create_table_composer & composer)
                    : parent_(parent), sql_(sql), composer_(composer) {
                if (sql.get_backend_name() == "postgresql") {
                    backend_id_ = SOCI::backend_id::PG;
                    type2value[ct::timedelta_t] = std::vector{std::string{}};
                }
                else
                if (sql.get_backend_name() == "mysql" or sql.get_backend_name() == "mariadb")
                    backend_id_ = SOCI::backend_id::MYSQL;
            }
            void insert(auto const &args, auto & reader) {
                data_holder.resize(composer_.types().size());
                indicators.resize(composer_.types().size(), std::vector<soci::indicator>{args.chunk_size, soci::i_ok});

                sql_.begin();
                insert_data(reader, composer_, args);
                sql_.commit();
            }
        };

        soci::session & sql_;
        create_table_composer & composer_;
        std::string const & after_insert_;
        std::string const & insert_prefix_;
    public:
        table_inserter(auto const &args, soci::session & sql, create_table_composer & composer)
                :sql_(sql), composer_(composer), after_insert_(args.after_insert), insert_prefix_(args.prefix) {
            if (!args.before_insert.empty()) {
                auto const statements = sql_split(std::stringstream(args.before_insert));
                sql.begin();
                for (auto & elem : statements)
                    sql << elem;
                sql.commit();
            }
        }
        ~table_inserter() {
            if (!after_insert_.empty()) {
                auto const statements = sql_split(std::stringstream(after_insert_));
                sql_.begin();
                for (auto & elem : statements)
                    sql_ << elem;
                sql_.commit();
            }
        }

        void insert(auto const &args, auto & reader) {
            if (args.chunk_size <= 1)
                simple_inserter(*this, sql_, composer_).insert(args, reader);
            else
                batch_bulk_inserter(*this, sql_, composer_).insert(args, reader);
        }
    };

    class query {
    public:
        query(auto const & args, soci::session & sql) {
            if (!args.query.empty()) {
                auto q_array = queries(args);
                std::for_each(q_array.begin(), q_array.end() - 1, [&](auto & elem){
                    sql.begin();
                    sql << elem;
                    sql.commit();
                });

                csvsuite::cli::sql::rowset_query(sql, args, q_array.back());
            }
        }
    };
}
