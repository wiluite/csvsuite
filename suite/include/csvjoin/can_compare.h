//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto can_compare = [&](auto & types0, auto & types1, auto & blanks0, auto & blanks1) {
    if (types0[c_ids[0]] == types1[c_ids[1]] or args.no_inference) {
        if (args.no_inference) {
            assert(types0[c_ids[0]] == column_type::text_t);
            assert(types1[c_ids[1]] == column_type::text_t);
        }
        return true;
    }
    if (blanks0[c_ids[0]] and blanks1[c_ids[1]]) {
        types0[c_ids[0]] = column_type::text_t;
        types1[c_ids[1]] = column_type::text_t;
        return true;
    }
    return false;
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
