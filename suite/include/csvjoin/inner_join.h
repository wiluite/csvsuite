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
            auto precise_types_blanks = blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? std::tuple{types0, blanks0} : std::tuple{types1, blanks1};
            unsigned precise_idx = blanks0[c_ids[0]] >= blanks1[c_ids[1]] ? c_ids[0] : c_ids[1];
#if 0
            auto fun = std::get<1>(obtain_compare_functionality<elem_t>(precise_idx, precise_types_blanks, args));
#endif            
            auto & this_source = deq.front();
            auto & other_source = deq[1];
#if 0
            std::visit([&](auto &&cmp) {
#endif
            std::visit([&](auto &&arg) {

                assert(!std::holds_alternative<reader_fake<reader_type>>(other_source));

                auto & other_reader = std::get<0>(other_source);
#if 0
                using other_reader_type = std::decay_t<decltype(other_reader)>;
#endif
                compromise_table_MxN table(other_reader, args);
                auto compare_fun = obtain_compare_functionality<elem_t>(precise_idx, precise_types_blanks, args);
                std::stable_sort(table.begin(), table.end(), sort_comparator(compare_fun, std::less<>()));
#if 0
                std::vector<typename other_reader_type::cell_span> other_cell_span_vector;
                std::vector<elem_t> other_typed_span_vector;
                auto const other_cols = other_reader.cols();
                auto const other_rows = other_reader.rows();
                max_field_size_checker other_size_checker(other_reader, args, other_cols, init_row{args.no_header ? 1u : 2u});

                other_cell_span_vector.reserve(other_rows * other_cols);
                other_typed_span_vector.reserve(other_rows * other_cols);

                other_reader.run_rows([&](auto & other_span) {
                    check_max_size(other_span, other_size_checker);
                    static_assert(std::is_same_v<std::decay_t<decltype(other_span)>, typename other_reader_type::row_span>);
                    for (auto & e : other_span) {
                        other_cell_span_vector.push_back(e);
                        other_typed_span_vector.push_back(elem_t{e});
                        using UElemType = typename std::decay_t<elem_t>::template rebind<csv_co::unquoted>::other;
                        other_typed_span_vector.back().operator UElemType const&().type();
                    }
                });
#endif
                max_field_size_checker this_size_checker(arg, args, arg.cols(), init_row{args.no_header ? 1u : 2u});

                arg.run_rows([&](auto &span) {
                    if constexpr (!std::is_same_v<std::remove_reference_t<decltype(span[0])>, std::string>)
                        check_max_size(span, this_size_checker);
                    auto key = elem_t{span[c_ids[0]]};
                    const auto p = std::equal_range(table.begin(), table.end(), key, equal_range_comparator<reader_type>(compare_fun));
                    for (auto next = p.first; next != p.second; ++next) {
                        std::vector<std::string> join_vec;

                        join_vec.reserve(span.size() + next->size() - 1);
                        join_vec.assign(span.begin(), span.end());
                        join_vec.insert(join_vec.end(), next->begin(), next->begin() + c_ids[1]);
                        join_vec.insert(join_vec.end(), next->begin() + c_ids[1] + 1, next->end());
                        impl.add(std::move(join_vec));
                    }
#if 0
                    std::size_t rows_count = 0;
                    while (rows_count < other_rows) {
                        typename other_reader_type::row_span e(other_cell_span_vector.begin() + rows_count * other_cols, other_cols);
                        auto & _ = other_typed_span_vector[rows_count * other_cols + c_ids[1]];
                        if (!cmp(elem_t{span[c_ids[0]]}, _)) {
                            std::vector<std::string> join_vec;

                            join_vec.reserve(span.size() + e.size() - 1);
                            join_vec.assign(span.begin(), span.end());
                            join_vec.insert(join_vec.end(), e.begin(), e.begin() + c_ids[1]);
                            join_vec.insert(join_vec.end(), e.begin() + c_ids[1] + 1, e.end());
                            impl.add(std::move(join_vec));
                        }
                        rows_count++;
                    }
#endif
                });
            }, this_source);
#if 0
            }, fun);
#endif
        }

        cycle_cleanup();
        deq.push_front(std::move(impl));
    }
};
