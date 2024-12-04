#pragma once

namespace {
    enum use_date_datetime_excel {
        yes,
        no
    };

    static std::vector<unsigned> dates_ids;
    static std::vector<unsigned> datetimes_ids;

    auto get_date_and_datetime_columns(auto && args, auto && header, use_date_datetime_excel use_d_dt) {
        using namespace ::csvkit::cli;
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
}
