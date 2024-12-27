#pragma once

namespace {

    auto string_header(auto const & header) {
        std::vector<std::string> result(header.size());
        std::transform(header.cbegin(), header.cend(), result.begin(), [&](auto & elem) {
            return csvsuite::cli::compose_text(elem);
        });
        return result;
    }
    
    void print_head(auto const & args, auto const & header, std::ostream & os) {
        auto s_hdr = string_header(header);  
        auto write_header = [&s_hdr, &args, &os] {
            if (args.linenumbers)
                os << "line_number,";

            std::for_each(s_hdr.cbegin(), s_hdr.cend() - 1, [&](auto const & elem) {
                os << elem << ',';
            });
            os << s_hdr.back() << '\n';
        };

        write_header();
    }

}
