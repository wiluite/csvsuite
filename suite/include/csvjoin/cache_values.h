//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------

auto cache_values = [&](auto & container) {
    for_each(poolstl::par, container.begin(), container.end(), [&](auto &item) {
        for (auto & elem : item) {
            using UElemType = typename std::decay_t<decltype(elem)>::template rebind<csv_co::unquoted>::other;
            elem.operator UElemType const&().type();
        }
    });
};

//------------------- This is just a code to inline it "in place" by the C preprocessor directive #include. See csvJoin.cpp --------------
