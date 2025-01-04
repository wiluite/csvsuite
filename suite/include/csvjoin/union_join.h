//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

/// Performs the union join
auto union_join = [&cycle_cleanup, &deq] {
    while (deq.size() > 1) {
        std::size_t rows = 0u;
        std::size_t cols = 0u;
        std::for_each(deq.begin(), deq.begin() + 2, [&](auto &r) {
            std::visit([&](auto &&arg) {
                rows = std::max(rows, arg.rows());
                cols += arg.cols();
            }, r);
        });

        reader_fake<reader_type> impl{rows, cols};

        auto col_ofs = 0;
        std::for_each(deq.begin(), deq.begin() + 2, [&](auto &r) {
            std::visit([&](auto &&arg) {
                std::size_t row = 0;
                auto const total_cols = arg.cols();

                arg.run_rows([&](auto &row_span) {
                    unsigned col = 0;
                    for (auto &elem: row_span) {
                        if constexpr (std::is_same_v<std::remove_reference_t<decltype(elem)>, std::string>)
                            impl[row][col_ofs + col++] = std::move(elem);
                        else
                            impl[row][col_ofs + col++] = std::move(elem.operator cell_string());
                    }
                    ++row;
                });
                col_ofs += total_cols;
            }, r);
        });

        cycle_cleanup(exclude_c_column::no, union_join::yes);
        deq.push_front(std::move(impl));
    }
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
