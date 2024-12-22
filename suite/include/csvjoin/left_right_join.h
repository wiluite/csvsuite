//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto left_or_right_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup, &can_compare, &compose_compare_function] {
    assert(!c_ids.empty());
    while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
        auto const & [types0, _] = ts_n_blanks[0];
        auto const & [types1, __] = ts_n_blanks[1];
#else
        auto const & types0 = std::get<0>(ts_n_blanks[0]);
        auto const & types1 = std::get<0>(ts_n_blanks[1]);
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

            auto & this_source = deq.front();
            auto & other_source = deq[1];

            std::visit([&](auto &&arg) {

                assert(!std::holds_alternative<reader_fake<reader_type>>(other_source));

                auto & other_reader = std::get<0>(other_source);

                compromise_table_MxN other(other_reader, args);
                auto compare_fun = compose_compare_function();
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

                        join_vec.reserve(span.size() + next->size() - 1);
                        join_vec.assign(span.begin(), span.end());
                        join_vec.insert(join_vec.end(), next->begin(), next->begin() + c_ids[1]);
                        join_vec.insert(join_vec.end(), next->begin() + c_ids[1] + 1, next->end());
                        impl.add(std::move(join_vec));
                        if (!were_joins)
                            were_joins = true;
                    }
                    if (!were_joins)
                        impl.add(std::move(compose_distinct_record(span)));
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

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
