#pragma once

namespace {
    static bool is1904;

    inline static std::chrono::system_clock::time_point to_chrono_time_point(double d) {
        using ddays = std::chrono::duration<double, date::days::period>;
        if (is1904)
            return date::sys_days{date::January/01/1904} + round<std::chrono::system_clock::duration>(ddays{d});
        else if (d < 60)
            return date::sys_days{date::December/31/1899} + round<std::chrono::system_clock::duration>(ddays{d});
        else
            return date::sys_days{date::December/30/1899} + round<std::chrono::system_clock::duration>(ddays{d});
    }
}

