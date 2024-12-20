auto left_or_right_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup, &can_compare] {

    assert(!c_ids.empty());
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

        auto compose_distinct_record = [&](auto const &span) {
            std::vector<std::string> join_vec;
            join_vec.reserve(span.size() + types1.size() - 1);
            join_vec.assign(span.begin(), span.end());
            std::vector<std::string> empty_vec(types1.size() - 1);
            join_vec.insert(join_vec.end(), empty_vec.begin(), empty_vec.end());
            return join_vec;
        };

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

                arg.run_rows([&](auto &span) {

                    std::visit([&](auto &&arg) {

                        bool were_joins = false;

                        arg.run_rows([&](auto &span1) {

                            std::visit([&](auto &&cmp) {
                                if (!cmp(elem_t{span[c_ids[0]]}, elem_t{span1[c_ids[1]]})) {
                                    std::vector<std::string> join_vec;
                                    assert(types1.size() == span1.size());
                                    join_vec.reserve(span.size() + span1.size() - 1);
                                    join_vec.assign(span.begin(), span.end());
                                    join_vec.insert(join_vec.end(), span1.begin(), span1.begin() + c_ids[1]);
                                    join_vec.insert(join_vec.end(), span1.begin() + c_ids[1] + 1, span1.end());
                                    impl.add(std::move(join_vec));
                                    if (!were_joins)
                                        were_joins = true;
                                }
                            }, fun);
                        });
                        if (!were_joins)
                            impl.add(std::move(compose_distinct_record(span)));
                    }, other_source);
                });
            }, this_source);
        } else {
            std::visit([&impl, &compose_distinct_record](auto &&arg) {
                arg.run_rows([&](auto &span) {
                    impl.add(std::move(compose_distinct_record(span)));
                });
            }, deq.front());
        }
        cycle_cleanup();
        deq.push_front(std::move(impl));
    }
};
