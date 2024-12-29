///
/// \file   suite/src/in2csv/common_datetime_excel.h
/// \author wiluite
/// \brief  Common datetime and chrono code for the different converters.

#pragma once

namespace {
    enum use_date_datetime_excel {
        yes,
        no
    };

    static std::vector<unsigned> dates_ids;
    static std::vector<unsigned> datetimes_ids;

    auto get_date_and_datetime_columns(auto && args, auto && header, use_date_datetime_excel use_d_dt) {
        using namespace ::csvsuite::cli;
        if (use_d_dt == use_date_datetime_excel::no)
            return;

        if (args.d_excel != "none") {
            std::string not_columns;
            dates_ids = parse_column_identifiers(columns{args.d_excel}, header, get_column_offset(args), excludes{not_columns});
        }

        if (args.dt_excel != "none") {
            std::string not_columns;
            datetimes_ids = parse_column_identifiers(columns{args.dt_excel}, header, get_column_offset(args), excludes{not_columns});
        }
    };

    bool is1904;

    inline std::chrono::system_clock::time_point to_chrono_time_point(double d) {
        using ddays = std::chrono::duration<double, date::days::period>;
        if (is1904)
            return date::sys_days{date::January/01/1904} + round<std::chrono::system_clock::duration>(ddays{d});
        else if (d < 60)
            return date::sys_days{date::December/31/1899} + round<std::chrono::system_clock::duration>(ddays{d});
        else
            return date::sys_days{date::December/30/1899} + round<std::chrono::system_clock::duration>(ddays{d});
    }

}
