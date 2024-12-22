//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto compose_compare_function = [&] {
#if !defined(__clang__) || __clang_major__ >= 16
    auto const & [types0, blanks0] = ts_n_blanks[0];
    auto const & [types1, blanks1] = ts_n_blanks[1];
#else
    auto const & types0 = std::get<0>(ts_n_blanks[0]);
    auto const & blanks0 = std::get<1>(ts_n_blanks[0]);
    auto const & types1 = std::get<0>(ts_n_blanks[1]);
    auto const & blanks1 = std::get<1>(ts_n_blanks[1]);
#endif

    auto blanks1_copy = blanks1;

    auto worst_of_blanks = [&] {
        return std::max(blanks0[c_ids[0]], blanks1[c_ids[1]]);
    };

    blanks1_copy[c_ids[1]] = worst_of_blanks();
    using elem_t = typename std::decay_t<decltype(std::get<0>(deq.front()))>::template typed_span<quoted>;
    return obtain_compare_functionality<elem_t>(c_ids[1], std::tuple{types1, blanks1_copy}, args);
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
