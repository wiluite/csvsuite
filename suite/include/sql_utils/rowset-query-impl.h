///
/// \file   suite/include/sql_utils/rowset-query_impl.h
/// \author wiluite
/// \brief  SQL Query printer.

#pragma once

namespace csvsuite::cli::SOCI {
    enum struct backend_id {
        PG,
        ORCL,
        MYSQL,
        FB,
        ANOTHER
    };
}

namespace csvsuite::cli::sql {

    void rowset_query(soci::session & sql, auto const & args, std::string const & q_s) {
        using namespace soci;

        enum class if_time {
            no = 0,
            yes
        };

        bool printable_can_be_value = false;
        bool printable_can_be_weird = false;
        SOCI::backend_id backend_id {SOCI::backend_id::ANOTHER};
        if (sql.get_backend_name() == "postgresql") {    //timedelta is string
            printable_can_be_value = true;
            backend_id = SOCI::backend_id::PG;
        }
        else if (sql.get_backend_name() == "firebird") { //datetime and timedelta are weird std::tm (fixed)
            backend_id = SOCI::backend_id::FB;
            printable_can_be_weird = true;
        }

        std::map<unsigned, if_time> col2time;
        rowset<row> rs = (sql.prepare << q_s);

        // --- NOTE!---We can't do even that.--- It is dangerous for further actions!
        // if (rs.begin() == rs.end())
        //     return;
        // ---

        std::tm d{};
        char timeString_v2[std::size("yyyy-mm-dd")];
        char timeString_v11[std::size("yyyy-mm-dd hh:mm:ss.uuuuuu")];
        bool print_header = false;
        for (auto && elem : rs) {
            row const &rr = elem;
            if (!rr.size())
                return; // we had supposed it was a "select", but it was another statement!

            if (!print_header) {
                if (!args.no_header) {
                    if (args.linenumbers)
                        std::cout << "line_number" << ',';
                    std::cout << rr.get_properties(0).get_name();
                    for (std::size_t i = 1; i != rr.size(); ++i)
                        std::cout << ',' << rr.get_properties(i).get_name();
                    std::cout << '\n';
                }
                print_header = true;
            }

            auto print_data = [&](std::size_t i) {
                if (rr.get_indicator(i) == soci::i_null) {
                    if (rr.size() == 1)
                        std::cout << R"("")";
                    return;
                }

                column_properties const & props = rr.get_properties(i);
                num_stringstream ss("C");

                auto db_type = props.get_db_type();
                switch(db_type)
                {
                    case db_string:
                        std::cout << rr.get<std::string>(i);
                        break;
                    case db_double:
                        ss << csv_mcv_prec(rr.get<double>(i));
                        std::cout << ss.str();
                        break;
                    case db_int32: // (bool: SELECT on Sqlite3)
                        std::cout << std::boolalpha << static_cast<bool>(rr.get<int32_t>(i));
                        break;
                    case db_date:
                        assert(rr.get_indicator(i) == soci::i_ok);
                        d = rr.get<std::tm>(i);
                        if (col2time[i] == if_time::no and d.tm_hour == 0 and d.tm_min == 0 and d.tm_sec == 0) {
                            std::strftime(std::data(timeString_v2), std::size(timeString_v2),"%Y-%m-%d", &d);
                            std::cout << timeString_v2;
                        } else {
                            if (printable_can_be_weird)
                                d.tm_isdst = 0;
                            snprintf(timeString_v11, std::size(timeString_v11), "%d-%02d-%02d %02d:%02d:%02d.%06d",
                                d.tm_year + 1900, d.tm_mon + 1, d.tm_mday, d.tm_hour, d.tm_min, d.tm_sec, d.tm_isdst);
                            std::cout << timeString_v11;

                            if (col2time[i] == if_time::no)
                                col2time[i] = if_time::yes;
                        }
                        break;
                    case db_int8: // (bool: SELECT on PGSQL, MYSQL)
                        std::cout << std::boolalpha << static_cast<bool>(static_cast<int>(rr.get<int8_t>(i)));
                        break;
                    default:
                        break;
                }
            };

            if (args.linenumbers) {
                static auto line = 1ul;
                std::cout << line++ << ',';
            }
            print_data(0);
            for (std::size_t i = 1; i != rr.size(); ++i) {
                std::cout << ',';
                print_data(i);
            }
            std::cout << '\n';
        }

        if (!print_header) { // no data - just print table header
            if (!args.no_header) {
                row v;
                sql.once << q_s,into (v);
                if (!v.size())
                    return;  // we had supposed it was a "select", but it was another statement!

                if (args.linenumbers)
                    std::cout << "line_number" << ',';

                std::cout << static_cast<column_properties const&>(v.get_properties(0)).get_name();
                for (auto i = 1u; i < v.size(); i++)
                    std::cout << ',' << static_cast<column_properties const&>(v.get_properties(i)).get_name();
                std::cout << '\n';
            }
        }
    }

    void rowset_query(Connection & con, auto const & args, std::string const & q_s) {
        Statement stmt(con);
        stmt.Execute(q_s);
        Resultset rs = stmt.GetResultset();
        if (nullptr != rs) {
            if (!args.no_header) {
                if (args.linenumbers)
                    std::cout << "line_number" << ',';
                std::cout << rs.GetColumn(1).GetName();

                for (auto i = 2u; i <= rs.GetColumnCount(); i++)
                    std::cout << ',' << rs.GetColumn(i).GetName();
                std::cout << '\n';
            }

            auto print_data = [&](std::size_t i) {
                if (rs.IsColumnNull(i)) {
                    if (rs.GetColumnCount() == 1)
                        std::cout << R"("")";
                    else
                        std::cout << R"()";
                    return;
                }

                num_stringstream ss("C");

                char timeString_v2[std::size("yyyy-mm-dd")];
                char timeString_v11[std::size("yyyy-mm-dd hh:mm:ss.uuuuuu")];
                std::tm tm_{};
                Date date;
                Timestamp ts(Timestamp::NoTimeZone);
                Interval il(Interval::DaySecond);
                switch(static_cast<int>(rs.GetColumn(i).GetType()))
                {
                    case DataTypeValues::TypeNumeric:
                        ss << csv_mcv_prec(rs.Get<double>(i));
                        std::cout << ss.str();
                        break;
                    case DataTypeValues::TypeString:
                        std::cout << rs.Get<std::string>(i);
                        break;
                    case DataTypeValues::TypeLong:
                        std::cout << rs.Get<int32_t>(i);
                        break;
                    case DataTypeValues::TypeDate:
                        date = rs.Get<Date>(i);
                        snprintf(std::data(timeString_v2), std::size(timeString_v2), "%04d-%02d-%02d", date.GetYear()
                            , date.GetMonth(), date.GetDay());
                        std::cout << timeString_v2;
                        break;
                    case DataTypeValues::TypeTimestamp:
                        ts = rs.Get<Timestamp>(i);
                        snprintf(timeString_v11, std::size(timeString_v11), "%d-%02d-%02d %02d:%02d:%02d.%06d",
                            ts.GetYear(), ts.GetMonth(), ts.GetDay(), ts.GetHours(), ts.GetMinutes()
                            , ts.GetSeconds(), ts.GetMilliSeconds());
                        std::cout << timeString_v11;
                        break;
                    case DataTypeValues::TypeInterval:
                        il = rs.Get<Interval>(i);
                        char buf [64];
                        #define GetNanoSeconds GetMilliSeconds
                        if (il.GetDay())
                            snprintf(buf, 64, "\"%d %s, %02d:%02d:%02d.%06d\"", il.GetDay(),
                                (il.GetDay() > 1 ? "days" : "day"), il.GetHours(), il.GetMinutes()
                                , il.GetSeconds(), il.GetNanoSeconds()/1000);
                        else
                            snprintf(buf, 64, "%02d:%02d:%02d.%06d", il.GetHours(), il.GetMinutes()
                                , il.GetSeconds(), il.GetNanoSeconds()/1000);
                        #undef GetNanoSeconds
                        std::cout << buf;
                        break;

                    default:
                        break;
                }
            };
            while (rs++) {
                if (args.linenumbers) {
                    static auto line = 1ul;
                    std::cout << line++ << ',';
                }
                print_data(1);
                for (auto i = 2u; i <= rs.GetColumnCount(); ++i) {
                    std::cout << ',';
                    print_data(i);
                }
                std::cout << '\n';
            }
        }
    }
}
