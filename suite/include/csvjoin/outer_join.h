auto outer_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup, &can_compare] {
    assert(!c_ids.empty());
    assert (!args.left_join and !args.right_join and args.outer_join);
    while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
        auto const & [types0, blanks0] = ts_n_blanks[0];
        auto const & [types1, blanks1] = ts_n_blanks[1];
#else
        auto const & types0 = std::get<0>(ts_n_blanks[0]);
        auto const & blanks0 = std::get<1>(ts_n_blanks[0]);
        auto const & types1 = std::get<0>(ts_n_blanks[1]);
        auto const & blanks1 = std::get<1>(ts_n_blanks[1]);
#endif
        reader_fake<reader_type> impl{0, 0};

        auto compose_distinct_left_part = [&](auto const &span) {
            std::vector<std::string> join_vec;
            join_vec.reserve(span.size() + types1.size());
            join_vec.assign(span.begin(), span.end());
            std::vector<std::string> empty_vec(types1.size());
            join_vec.insert(join_vec.end(), empty_vec.begin(), empty_vec.end());
            return join_vec;
        };

        auto compose_distinct_right_part = [&](auto const &span) {
            std::vector<std::string> join_vec;
            join_vec.reserve(span.size() + types0.size());
            std::vector<std::string> empty_vec(types0.size());
            join_vec.assign(empty_vec.begin(), empty_vec.end());
            join_vec.insert(join_vec.end(), span.begin(), span.end());
            return join_vec;
        };

        bool recalculate_types_blanks = false;
        if (can_compare(types0, types1)) {
            using namespace ::csvsuite::cli::compare;
            using elem_t = typename std::decay_t<decltype(std::get<0>(deq.front()))>::template typed_span<quoted>;
#if !defined(__clang__) || __clang_major__ >= 16
            auto [_, fun] = obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0];
#else
            auto fun = std::get<1>(obtain_compare_functionality<elem_t>(blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::vector<unsigned>{c_ids[0]} : std::vector<unsigned>{c_ids[1]}
                , blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1}, args)[0]);
#endif
            auto & this_source = deq.front();
            auto & other_source = deq[1];

            std::visit([&](auto &&arg) {

                assert(!std::holds_alternative<reader_fake<reader_type>>(other_source));

                auto & other_reader = std::get<0>(other_source);

                constexpr bool skip_to_first_line_after_fill = true;
                compromise_table_MxN<reader_type, args_type, skip_to_first_line_after_fill> other(other_reader, args);
                bool blanks_regress = blanks0[c_ids[0]] >= blanks1[c_ids[1]];
                auto compare_fun = obtain_compare_functionality<elem_t>
                    (blanks_regress ? c_ids[0] : c_ids[1], blanks_regress ? ts_n_blanks[0] : ts_n_blanks[1], args);
                std::stable_sort(poolstl::par, other.begin(), other.end(), sort_comparator(compare_fun, std::less<>()));
                auto cache_types = [&] {
                    for_each(poolstl::par, other.begin(), other.end(), [&](auto &item) {
                        for (auto & elem : item) {
                            using UElemType = typename std::decay_t<decltype(elem)>::template rebind<csv_co::unquoted>::other;
                            elem.operator UElemType const&().type();
                        }
                    });
                };

                cache_types();

                arg.run_rows([&](auto &span) {
                    bool were_joins = false;
                    auto key = elem_t{span[c_ids[0]]};
                    const auto p = std::equal_range(other.begin(), other.end(), key, equal_range_comparator<reader_type>(compare_fun));
                    for (auto next = p.first; next != p.second; ++next) {
                        std::vector<std::string> join_vec;
                        join_vec.reserve(span.size() + next->size());
                        join_vec.assign(span.begin(), span.end());
                        join_vec.insert(join_vec.end(), next->begin(), next->end());
                        impl.add(std::move(join_vec));
                        if (!were_joins)
                            were_joins = true;
                    }
                    if (!were_joins)
                        impl.add(std::move(compose_distinct_left_part(span)));
                });
            }, this_source);

            // Here, outer join right table
            std::visit([&](auto &&arg) {
                arg.run_rows([&](auto &span) {
                    std::visit([&](auto &&arg) {
                        bool were_joins = false;
                        arg.run_rows([&](auto &span1) {
                            std::visit([&](auto &&cmp) {
                                if (!cmp(elem_t{span[c_ids[1]]}, elem_t{span1[c_ids[0]]}))
                                    were_joins = true;
                            }, fun);
                        });
                        if (!were_joins) {
                            impl.add(std::move(compose_distinct_right_part(span)));
                            recalculate_types_blanks = true;
                        }
                    }, this_source);
                });
            }, other_source);
        } else {
            std::visit([&](auto &&arg) {
                arg.run_rows([&impl, &compose_distinct_left_part](auto &span) {
                    impl.add(std::move(compose_distinct_left_part(span)));
                });
            }, deq.front());
            std::visit([&](auto &&arg) {
                arg.run_rows([&impl, &compose_distinct_right_part](auto &span) {
                    impl.add(std::move(compose_distinct_right_part(span)));
                });
            }, deq[1]);
            recalculate_types_blanks = true;
        }

        cycle_cleanup(exclude_c_column::no);

        bool const recalculate = args.honest_outer_join ? recalculate_types_blanks : (recalculate_types_blanks && !deq.empty());

        if (recalculate) {
            typename std::decay_t<decltype(std::get<0>(deq.front()))> tmp_reader(impl.operator typename reader_fake<reader_type>::table &());
            struct {
                bool no_header = true;
                unsigned skip_lines = 0;
                std::string num_locale;
                std::string datetime_fmt;
                std::string date_fmt;
                bool no_leading_zeroes{};
                bool blanks{};
                mutable std::vector<std::string> null_value;
                bool no_inference{};
                bool date_lib_parser{};
                unsigned maxfieldsize = max_unsigned_limit;
            } tmp_args{
                true
                , 0
                , args.num_locale
                , args.datetime_fmt
                , args.date_fmt
                , args.no_leading_zeroes
                , args.blanks
                , args.null_value
                , args.no_inference
                , args.date_lib_parser
                , args.maxfieldsize
            };
            ts_n_blanks[0] = std::get<1>(typify::typify(tmp_reader, tmp_args, typify_option::typify_without_precisions));
        }
        deq.push_front(std::move(impl));
        assert(deq.size() == ts_n_blanks.size());
        assert(deq.size() == c_ids.size());
    }
};
