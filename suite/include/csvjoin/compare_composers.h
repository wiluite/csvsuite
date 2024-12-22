//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto worst_of_blanks = [&] {
    return std::max(std::get<1>(ts_n_blanks[0])[c_ids[0]], std::get<1>(ts_n_blanks[1])[c_ids[1]]);
};

auto compose_compare_function = [&] {
    auto blanks1_copy = std::get<1>(ts_n_blanks[1]);

    blanks1_copy[c_ids[1]] = worst_of_blanks();
    using elem_t = typename reader_type::template typed_span<quoted>;
    return obtain_compare_functionality<elem_t>(c_ids[1], std::tuple{std::get<0>(ts_n_blanks[1]), blanks1_copy}, args);
};

auto compose_symmetric_compare_function = [&] {
    auto blanks0_copy = std::get<1>(ts_n_blanks[0]);

    blanks0_copy[c_ids[0]] = worst_of_blanks();
    using elem_t = typename reader_type::template typed_span<quoted>;
    return obtain_compare_functionality<elem_t>(c_ids[0], std::tuple{std::get<0>(ts_n_blanks[0]), blanks0_copy}, args);
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
