//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto inner_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup, &can_compare, &compose_compare_function, &cache_values] {
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

        if (can_compare(types0, types1)) {
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

                    arg.run_rows([&](auto &span) {
                        auto const key = elem_t{span[c_ids[0]]};
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
                }
                catch (typename reader_type::implementation_exception const &) {}
                catch (no_body_exception const &) {}

            }, this_source);
        }

        cycle_cleanup();
        deq.push_front(std::move(impl));
    }
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
