//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

try {
    auto idx = 0u;
    std::for_each(deq.begin(), deq.end(), [&](auto &r) {
        assert(!std::holds_alternative<reader_fake<reader_type>>(r));
        auto & this_reader = std::get<0>(r);

        recode_source(this_reader, args);
        skip_lines(this_reader, args);
        quick_check(this_reader, args);

        ts_n_blanks.push_back(std::get<1>(::csvsuite::cli::typify(this_reader, args, typify_option::typify_without_precisions)));
        skip_lines(this_reader, args);
        auto const header = obtain_header_and_<skip_header>(this_reader, args);

        max_field_size_checker size_checker(this_reader, args, header.size(), init_row{1});

        // TODO: it is very hard to optimize this place (get rid of premature transformation - try it out)
        //  but it is vain to do, due to no many resources spending.
        check_max_size(header_to_strings<unquoted>(header), size_checker);

        auto const q_header = header_to_strings<unquoted>(header);

        headers.push_back(q_header);
        if (!join_column_names.empty())
            c_ids.push_back(match_column_identifier(q_header, join_column_names[idx++].c_str(), get_column_offset(args)));
    });
} catch (ColumnIdentifierError const& e) {
    std::cerr << e.what() << std::endl;
    return;
}

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
