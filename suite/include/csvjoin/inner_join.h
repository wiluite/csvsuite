auto inner_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup] {
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

        auto can_compare = [&]() {
            return (types0[c_ids[0]] == types1[c_ids[1]]) or args.no_inference;
        };

        if (can_compare()) {
            using namespace ::csvsuite::cli::compare;
            using elem_t = typename std::decay_t<decltype(std::get<0>(deq.front()))>::template typed_span<quoted>;

            auto & this_source = deq.front();
            auto & other_source = deq[1];

            std::visit([&](auto &&arg) {

                assert(!std::holds_alternative<reader_fake<reader_type>>(other_source));

                auto & other_reader = std::get<0>(other_source);

                compromise_table_MxN other(other_reader, args);
                auto compare_fun = obtain_compare_functionality<elem_t>(c_ids[1], ts_n_blanks[1], args);
                std::stable_sort(poolstl::par, table.begin(), table.end(), sort_comparator(compare_fun, std::less<>()));

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
                    auto key = elem_t{span[c_ids[0]]};
                    const auto p = std::equal_range(other.begin(), other.end(), key, equal_range_comparator<reader_type>(compare_fun));
                    for (auto next = p.first; next != p.second; ++next) {
                        std::vector<std::string> join_vec;

                        join_vec.reserve(span.size() + next->size() - 1);
                        join_vec.assign(span.begin(), span.end());
                        join_vec.insert(join_vec.end(), next->begin(), next->begin() + c_ids[1]);
                        join_vec.insert(join_vec.end(), next->begin() + c_ids[1] + 1, next->end());
                        impl.add(std::move(join_vec));
                    }
                });
            }, this_source);
        }

        cycle_cleanup();
        deq.push_front(std::move(impl));
    }
};
