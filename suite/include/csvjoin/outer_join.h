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
#if 0
        auto can_compare = [&] {
            return (types0[c_ids[0]] == types1[c_ids[1]]) or args.no_inference;
        };
#endif
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
            auto & first_source = deq.front();
            auto & second_source = deq[1];

            std::visit([&](auto &&arg) {
                //max_field_size_checker f_size_checker(arg, args, arg.cols(), init_row{args.no_header ? 1u : 2u});
                arg.run_rows([&](auto &span) {
                    //if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span[0])>, std::string>)
                    //    check_max_size(span, f_size_checker);

                    std::visit([&](auto &&arg) {
                        bool were_joins = false;
                        //max_field_size_checker s_size_checker(arg, args, arg.cols(), init_row{args.no_header ? 1u : 2u});
                        arg.run_rows([&](auto &span1) {
                            //if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span1[0])>, std::string>)
                            //    check_max_size(span1, s_size_checker);

                            std::visit([&](auto &&cmp) {
                                if (!cmp(elem_t{span[c_ids[0]]}, elem_t{span1[c_ids[1]]})) {
                                    std::vector<std::string> join_vec;
                                    assert(types1.size() == span1.size());
                                    join_vec.reserve(span.size() + span1.size());
                                    join_vec.assign(span.begin(), span.end());
                                    join_vec.insert(join_vec.end(), span1.begin(), span1.end());
                                    impl.add(std::move(join_vec));
                                    if (!were_joins)
                                        were_joins = true;
                                }
                            }, fun);
                        });
                        if (!were_joins)
                            impl.add(std::move(compose_distinct_left_part(span)));
                    }, second_source);
                });
            }, first_source);

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
                    }, first_source);
                });
            }, second_source);
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
