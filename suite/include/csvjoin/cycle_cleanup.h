auto concat_headers = [&headers](unsigned excl_v_idx = static_cast<unsigned>(-1)) {
    std::string subst;
    unsigned const h0_size = headers[0].size();
    std::replace_copy_if(headers[1].begin(), headers[1].end(), std::back_inserter(headers[0]),
        [&](auto const & n) {
            if (!std::count(headers[0].cbegin(), headers[0].cend(), n))
                return false;

            auto mangled_part = 2u;
            for (;;) {
                auto const what_subst = n + '_' + std::to_string(mangled_part);
                if (!std::count(headers[0].cbegin(), headers[0].cend(), what_subst)) {
#if 0
                    std::cerr << "Column name " << std::quoted(n) << " already exists in Table. "
                              << "Column will be renamed to "
                              << std::quoted(n + '_' + std::to_string(mangled_part)) << ".\n";
#endif
                    subst = what_subst;
                    return true;
                }
                mangled_part++;
            }
        }, subst
    );
    if (excl_v_idx != static_cast<unsigned>(-1))
        headers[0].erase(headers[0].begin() + h0_size + excl_v_idx);
};

auto concat_ts_n_blanks = [&ts_n_blanks](unsigned excl_idx = static_cast<unsigned>(-1)) {
    unsigned const size_0 = std::get<0>(ts_n_blanks[0]).size();
    auto & types_0 = std::get<0>(ts_n_blanks[0]);
    auto const & types_1 = std::get<0>(ts_n_blanks[1]);
    auto & blanks_0 = std::get<1>(ts_n_blanks[0]);
    auto const & blanks_1 = std::get<1>(ts_n_blanks[1]); 

    types_0.insert(types_0.end(), types_1.begin(), types_1.end());
    blanks_0.insert(blanks_0.end(), blanks_1.begin(), blanks_1.end());

    if (excl_idx != static_cast<unsigned>(-1)) {
        types_0.erase(types_0.begin() + size_0 + excl_idx);
        blanks_0.erase(blanks_0.begin() + size_0 + excl_idx);
    }
};

enum class exclude_c_column {
    yes,
    no
}; 
enum class union_join {
    yes,
    no
}; 

auto cycle_cleanup = [&](exclude_c_column is_c_excluded = exclude_c_column::yes, union_join is_union_join = union_join::no) {
    deq.pop_front();
    deq.pop_front();
    concat_headers(is_c_excluded == exclude_c_column::yes ? c_ids[1] : static_cast<unsigned>(-1));
    headers.erase(headers.begin() + 1);
    concat_ts_n_blanks(is_c_excluded == exclude_c_column::yes ? c_ids[1] : static_cast<unsigned>(-1));

    if (is_union_join == union_join::no)
        c_ids.erase(c_ids.begin() + 1);
    ts_n_blanks.erase(ts_n_blanks.begin() + 1);
};
