//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto left_or_right_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup, &can_compare, &compose_compare_function, &cache_values] {
    assert(!c_ids.empty());
    while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
        auto & [types0, blanks0] = ts_n_blanks[0];
        auto & [types1, blanks1] = ts_n_blanks[1];
#else
        auto & types0 = std::get<0>(ts_n_blanks[0]);
        auto & types1 = std::get<0>(ts_n_blanks[1]);
        auto & blanks0 = std::get<1>(ts_n_blanks[0]);
        auto & blanks1 = std::get<1>(ts_n_blanks[1]);
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

        if (can_compare(types0, types1, blanks0, blanks1)) {
            using namespace ::csvsuite::cli::compare;
            using elem_t = typename reader_type::template typed_span<quoted>;

            auto & this_source = deq.front();
            auto & other_source = deq[1];

            std::visit([&](auto &&arg) {

                assert(!std::holds_alternative<reader_fake<reader_type>>(other_source));

                auto & other_reader = std::get<0>(other_source);

                try {
                    compromise_table_MxN other(other_reader, args);
                    auto compare_fun = compose_compare_function();
                    std::stable_sort(poolstl::par, other.begin(), other.end(), sort_comparator(compare_fun, std::less<>()));
                    cache_values(other);

                    using row_t = std::vector<std::string>;
                    using rows_t = std::vector<row_t>;

                    auto process = [&](auto & this_table, auto & join_vec) {
                        auto const table_addr = std::addressof(this_table[0]);
                        std::for_each(poolstl::par, this_table.begin(), this_table.end(), [&](auto & row) {

                            auto const key = elem_t{row[c_ids[0]]};
                            const auto p = std::equal_range(other.begin(), other.end(), key, equal_range_comparator<reader_type>(compare_fun));
                            if (p.first != p.second)
                                for (auto next = p.first; next != p.second; ++next) {
                                    std::vector<std::string> joins;

                                    joins.reserve(row.size() + next->size() - 1);
                                    joins.assign(row.begin(), row.end());
                                    joins.insert(joins.end(), next->begin(), next->begin() + c_ids[1]);
                                    joins.insert(joins.end(), next->begin() + c_ids[1] + 1, next->end());
                                    join_vec[std::addressof(row) - table_addr].emplace_back(std::move(joins));
                                }
                            else
                                join_vec[std::addressof(row) - table_addr].emplace_back(std::move(compose_distinct_record(row)));
                        });
                        for (auto & rows : join_vec) {
                            for (auto & row : rows)
                                impl.add(std::move(row));
                        }
                    };

                    if constexpr(std::is_same_v<std::decay_t<decltype(arg)>, reader_type>) {
                        compromise_table_MxN this_table(arg, args);
                        std::vector<rows_t> join_vec(this_table.rows());
                        process(this_table, join_vec);
                    } else {
                        static_assert(std::is_same_v<std::decay_t<decltype(arg)>, reader_fake<reader_type>>);
                        auto const & this_table = arg.operator typename reader_fake<reader_type>::table &();
                        std::vector<rows_t> join_vec(this_table.size());
                        process(this_table, join_vec);
                    }
                }
                catch (typename reader_type::implementation_exception const &) {}
                catch (no_body_exception const &) {
                    try {
                        arg.run_rows([&](auto &span) {
                            impl.add(std::move(compose_distinct_record(span)));
                        });
                    } catch (typename reader_type::implementation_exception const &) {}
                }

            }, this_source);
        } else {
            try {
                std::visit([&impl, &compose_distinct_record](auto &&arg) {
                    arg.run_rows([&](auto &span) {
                        impl.add(std::move(compose_distinct_record(span)));
                    });
                }, deq.front());
            } catch (typename reader_type::implementation_exception const &) {}
        }
        cycle_cleanup();
        deq.push_front(std::move(impl));
    }
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
