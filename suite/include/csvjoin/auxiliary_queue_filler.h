//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

try {
    auto idx = 0u;
    std::for_each(deq.begin(), deq.end(), [&](auto &r) {

        auto quick_check = [&r, &args]() {
            skip_lines(std::get<0>(r), args);
            ::csvsuite::cli::quick_check(std::get<0>(r), args);
        };

        //TODO: FIXME --- THIS SHOULD REALLY WORK (and works in GCC10.2, MSVC, CLANG 16)
        // (COMPILER BUGS IN: GCC 11, CLANG 12,13,14,15)
#if 0
        std::visit([&](auto &&arg) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(arg)>, reader_fake<reader_type>>) {
                quick_check();
                ts_n_blanks.push_back(std::get<1>(::csvsuite::cli::typify(arg, args, typify_option::typify_without_precisions)));
            }

            skip_lines(arg, args);
            auto const header = obtain_header_and_<skip_header>(arg, args);

            max_field_size_checker size_checker(*std::get_if<0>(&r), args, header.size(), init_row{1});

            // TODO: it is very hard to optimize this place (get rid of premature transformation - try it out)
            //  but it is vain to do, due to no many resources spending.
            check_max_size(header_to_strings<unquoted>(header), size_checker);

            auto const q_header = header_to_strings<unquoted>(header);

            headers.push_back(q_header);
            if (!join_column_names.empty())
                c_ids.push_back(match_column_identifier(q_header, join_column_names[idx++].c_str(), get_column_offset(args)));
        }, r);
#else
        if (!std::holds_alternative<reader_fake<reader_type>>(r)) {
            quick_check();
            ts_n_blanks.push_back(std::get<1>(::csvsuite::cli::typify(std::get<0>(r), args, typify_option::typify_without_precisions)));
        }
        std::visit([&](auto &&arg) {
             skip_lines(arg, args);
             auto const header = obtain_header_and_<skip_header>(arg, args);

             max_field_size_checker size_checker(arg, args, header.size(), init_row{1});

             // TODO: it is very hard to optimize this place (get rid of premature transformation - try it out)
             //  but it is vain to do, due to no many resources spending.
             check_max_size(header_to_strings<unquoted>(header), size_checker);

             auto const q_header = header_to_strings<unquoted>(header);

             headers.push_back(q_header);
             if (!join_column_names.empty())
                 c_ids.push_back(match_column_identifier(q_header, join_column_names[idx++].c_str(), get_column_offset(args)));
        }, r);
#endif
    });
} catch (ColumnIdentifierError const& e) {
    std::cerr << e.what() << std::endl;
    return;
}

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
