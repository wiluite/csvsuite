//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

/// Performs the union join
auto union_join = [&cycle_cleanup, &deq, &headers] {
    while (deq.size() > 1) {
        std::size_t rows = 0u;
        std::size_t cols = 0u;
        unsigned index = 0;
        std::for_each(deq.begin(), deq.begin() + 2, [&](auto &r) {
            std::visit([&](auto &&arg) {
                rows = std::max(rows, arg.rows());
                cols += std::holds_alternative<reader_fake<reader_type>>(r) ? arg.cols() : headers[index].size();
                ++index;
            }, r);
        });

        reader_fake<reader_type> impl{rows, cols};

        auto col_ofs = 0;
        index = 0;
        std::for_each(deq.begin(), deq.begin() + 2, [&](auto &r) {
            std::visit([&](auto &&arg) {
                std::size_t row = 0;
                auto const total_cols = std::holds_alternative<reader_fake<reader_type>>(r) ? arg.cols() : headers[index].size();
                try {
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
                } catch(typename reader_type::implementation_exception const &) {} // catch this particular exception and do nothing
                col_ofs += total_cols;
                ++index;  
            }, r);
        });
        
        cycle_cleanup(exclude_c_column::no, union_join::yes);
        deq.push_front(std::move(impl));
    }
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
