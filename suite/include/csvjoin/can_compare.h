auto can_compare = [&](auto const & types0, auto const & types1) {
    return (types0[c_ids[0]] == types1[c_ids[1]]) or args.no_inference;
};