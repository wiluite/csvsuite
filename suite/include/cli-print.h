#pragma once

namespace csvsuite::cli {
    inline bool ostream_numeric_corner_cases(std::ostringstream & ss, auto const & rep, auto const & args) {
        assert(!rep.is_null());
        ss.str({});

        // Surprisingly, csvkit represents a number from file without distortion:
        // knowing, that it is a valid number in a locale, it simply (almost for
        // sure) removes the thousands separators and replaces the decimal point
        // with its C-locale equivalent. Thus, the number actually written to the
        // file is output, and we have to do some tricks.

        auto const value = rep.num();

        if (std::isnan(value)) {
            ss << "NaN";
            return true;
        }
        if (std::isinf(value)) {
            ss << (value > 0 ? "Infinity" : "-Infinity");
            return true;
        } 
        if (args.num_locale != "C") {
            std::string s = rep.str();
            rep.to_C_locale(s);
            ss << s;
            return true; 
        }
        return false;  
    }
}
