//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto worst_of_blanks = [&] {
    return std::max(std::get<1>(ts_n_blanks[0])[c_ids[0]], std::get<1>(ts_n_blanks[1])[c_ids[1]]);
};

auto compose_compare_function_impl = [&] (auto & ts_n_blanks, auto & c_idx) {
    auto blanks_copy = std::get<1>(ts_n_blanks);

    blanks_copy[c_idx] = worst_of_blanks();
    using elem_t = typename reader_type::template typed_span<quoted>;
    return obtain_compare_functionality<elem_t>(c_idx, std::tuple{std::get<0>(ts_n_blanks), blanks_copy}, args);
};

auto compose_compare_function = [&] {
    return compose_compare_function_impl(ts_n_blanks[1], c_ids[1]);
};

auto compose_symmetric_compare_function = [&] {
    return compose_compare_function_impl(ts_n_blanks[0], c_ids[0]);
};

auto align_blanks_impl = [&] (auto & ts_n_blanks, auto & c_idx) {
    auto blanks_copy = std::get<1>(ts_n_blanks);
    blanks_copy[c_idx] = worst_of_blanks();
    return std::tuple{std::get<0>(ts_n_blanks), blanks_copy};
};

auto align_blanks = [&] {
    return align_blanks_impl(ts_n_blanks[1], c_ids[1]);
};

auto symmetric_align_blanks = [&] {
    return align_blanks_impl(ts_n_blanks[0], c_ids[0]);
};


//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
