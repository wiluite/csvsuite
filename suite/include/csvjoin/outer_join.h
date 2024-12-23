//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto outer_join = [&deq, &ts_n_blanks, &c_ids, &args, &cycle_cleanup, &can_compare, &compose_compare_function, compose_symmetric_compare_function, &cache_values] {
    assert(!c_ids.empty());
    assert (!args.left_join and !args.right_join and args.outer_join);
    while (deq.size() > 1) {
#if !defined(__clang__) || __clang_major__ >= 16
        auto const & [types0, _] = ts_n_blanks[0];
        auto const & [types1, __] = ts_n_blanks[1];
#else
        auto const & types0 = std::get<0>(ts_n_blanks[0]);
        auto const & types1 = std::get<0>(ts_n_blanks[1]);
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
            using elem_t = typename reader_type::template typed_span<quoted>;

            auto & this_source = deq.front();
            auto & other_source = deq[1];

            std::visit([&](auto &&arg) {

                assert(!std::holds_alternative<reader_fake<reader_type>>(other_source));

                auto & other_reader = std::get<0>(other_source);

                constexpr bool skip_to_first_line_after_fill = true;
                compromise_table_MxN<reader_type, args_type, skip_to_first_line_after_fill> other(other_reader, args);
                auto compare_fun = compose_compare_function();

                std::stable_sort(poolstl::par, other.begin(), other.end(), sort_comparator(compare_fun, std::less<>()));
                cache_values(other);

                arg.run_rows([&](auto &span) {
                    auto key = elem_t{span[c_ids[0]]};
                    const auto p = std::equal_range(other.begin(), other.end(), key, equal_range_comparator<reader_type>(compare_fun));
                    if (p.first != p.second)
                        for (auto next = p.first; next != p.second; ++next) {
                            std::vector<std::string> join_vec;
                            join_vec.reserve(span.size() + next->size());
                            join_vec.assign(span.begin(), span.end());
                            join_vec.insert(join_vec.end(), next->begin(), next->end());
                            impl.add(std::move(join_vec));
                        }
                    else
                        impl.add(std::move(compose_distinct_left_part(span)));
                });
            }, this_source);

            // Here, outer join right table
            std::visit([&](auto &&arg) {
                reader_type tmp_reader(" ");

                struct {
                    bool no_header;
                    unsigned skip_lines;
                } tmp_args(args.no_header, args.skip_lines);

                if (std::holds_alternative<reader_fake<reader_type>>(this_source)) {
                    auto & this_reader = std::get<1>(this_source);
                    reader_type standard_reader(this_reader.operator typename reader_fake<reader_type>::table &());
                    tmp_args.no_header = true;
                    tmp_args.skip_lines = 0;
                    tmp_reader = std::move(standard_reader);
                } else
                    tmp_reader = std::move(std::get<0>(this_source));

                compromise_table_MxN this_(tmp_reader, tmp_args);
                auto compare_fun = compose_symmetric_compare_function();
                std::stable_sort(poolstl::par, this_.begin(), this_.end(), sort_comparator(compare_fun, std::less<>()));
                cache_values(this_);

                arg.run_rows([&](auto &span) {
                    auto key = elem_t{span[c_ids[1]]};
                    const auto p = std::equal_range(this_.begin(), this_.end(), key, equal_range_comparator<reader_type>(compare_fun));
                    if (p.first == p.second) {
                        impl.add(std::move(compose_distinct_right_part(span)));
                        recalculate_types_blanks = true;
                    }
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
            reader_type tmp_reader(impl.operator typename reader_fake<reader_type>::table &());
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
            using namespace ::csvsuite::cli;
            ts_n_blanks[0] = std::get<1>(::csvsuite::cli::typify(tmp_reader, tmp_args, typify_option::typify_without_precisions));
        }
        deq.push_front(std::move(impl));
        assert(deq.size() == ts_n_blanks.size());
        assert(deq.size() == c_ids.size());
    }
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
