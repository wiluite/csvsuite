///
/// \file   utils/csvsuite/csvJson.cpp
/// \author wiluite
/// \brief  Convert a CSV file into JSON (or GeoJSON).

#include <cli.h>
#include <set>
#include <iomanip>

using namespace ::csvsuite::cli;

namespace csvjson::detail {
    template <std::size_t Int_Prec>
    inline std::string carefully_adjusted_number(auto const & elem) {
        static num_stringstream ss;
        static auto default_prec = ss.precision();
        ss.rdbuf()->str("");
        auto const value = elem.num();
        if (std::trunc(value) == value)
            ss << std::setprecision(Int_Prec);
        else {
            auto s = elem.str();
            trim_string(s);
            auto pos = s.find('.');
            ss << std::setprecision(pos != std::string::npos ? s.size() - pos - 1 : default_prec);
        }
        ss << value;
        return ss.str();
    }
}

namespace csvjson {

    struct Args : ARGS_positional_1 {
        std::string & num_locale = kwarg("L,locale", "Specify the locale (\"C\") of any formatted numbers.").set_default("C");
        bool & blanks = flag("blanks", R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> &null_value = kwarg("null-value","Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string &date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string &datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        bool & no_leading_zeroes = flag("no-leading-zeroes", "Do not convert a numeric value with leading zeroes to a number.");
        int & indent = kwarg("i,indent", "Indent the output JSON this many spaces. Disabled by default.").set_default(min_int_limit);
        std::string & key = kwarg("k,key","Output JSON as an object keyed by a given column, KEY, rather than as an array. All column values must be unique. If --lat and --lon are specified, this column is used as the GeoJSON Feature ID.").set_default("");
        std::string & lat = kwarg("lat", "A column index or name containing a latitude. Output will be GeoJSON instead of JSON. Requires --lon.").set_default("");
        std::string & lon = kwarg("lon", "A column index or name containing a longitude. Output will be GeoJSON instead of JSON. Requires --lat.").set_default("");
        std::string & type = kwarg("type", "A column index or name containing a GeoJSON type. Output will be GeoJSON instead of JSON. Requires --lat and --lon.").set_default("");
        std::string & geometry = kwarg("geometry", "A column index or name containing a GeoJSON geometry. Output will be GeoJSON instead of JSON. Requires --lat and --lon.").set_default("");
        std::string & crs = kwarg("crs", "A coordinate reference system string to be included with GeoJSON output. Requires --lat and --lon.").set_default("");
        bool & no_bbox = flag("no-bbox","Disable the calculation of a bounding box.");
        bool & stream = flag("stream","Output JSON as a stream of newline-separated objects, rather than an as an array.");
        bool & no_inference = flag("I,no-inference", "Disable type inference (and --locale, --date-format, --datetime-format) when parsing the input.");
        bool & date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);

        void welcome() final {
            std::cout << "\nConvert a CSV file into JSON (or GeoJSON).\n\n";
        }

    };

    void json(std::monostate &, auto const &) {}

    void json(auto & reader, auto const & args) {

        using namespace csv_co;
        using namespace detail;

        auto const geojson = (!args.lat.empty() or !args.lon.empty() or !args.type.empty() or !args.geometry.empty() or !args.crs.empty() or args.no_bbox);
        if (!args.lat.empty() and args.lon.empty())
            throw std::runtime_error("csvjson: error: --lon is required whenever --lat is specified.");

        if (args.lat.empty() and !args.lon.empty())
            throw std::runtime_error("csvjson: error: --lat is required whenever --lon is specified.");

        if ((args.lat.empty() or args.lon.empty()) and !args.type.empty())
            throw std::runtime_error("csvjson: error: --type is only allowed when --lat and --lon are also specified.");

        if (args.stream && !args.key.empty() && (args.lat.empty() or args.lon.empty()))
            throw std::runtime_error("csvjson: error: --key is only allowed with --stream when --lat and --lon are also specified.");

        if (!geojson and args.indent != min_int_limit and args.stream)
            throw std::runtime_error("ValueError: newline and indent may not be specified together.");

        if (!args.geometry.empty())
            throw std::runtime_error("csvjson: error: sorry, --geometry option for now is not supported.");

        using args_type = decltype(args);

        skip_lines(reader, args);
        quick_check(reader, args);

        // Detect types and blanks presence in columns
#if defined(__clang__) && (__clang_major__ < 16)
        static
#endif
        auto const [types, blanks] = std::get<1>(typify(reader, args, typify_option::typify_without_precisions));
        using types_type = decltype(types);
        using blanks_type = decltype(blanks);

        skip_lines(reader, args);
        static auto const header = obtain_header_and_<skip_header>(reader, args);
        using header_type = decltype(header);

        using elem_type = typename std::decay_t<decltype(reader)>::template typed_span<unquoted>;
        unsigned key_idx {};
        if (!geojson and !args.key.empty()) {
            auto const key_iter = std::find(std::begin(header), std::end(header), args.key);
            if (key_iter == std::end(header)) {
                std::string err = "KeyError: '" + args.key + '\'';
                throw std::runtime_error(err.c_str());
            }
            key_idx = key_iter - begin(header);

            static auto check_dup_func_impl = [&]( auto const & elem, auto get_value_function, auto print_function) {
                static std::set<decltype(get_value_function())> dups;
                static auto nulls = 0u;
                if (elem.is_null()) {
                    if (++nulls > 1)
                        throw std::runtime_error("ValueError: Value None is not unique in the key column.");
                } else {
                    auto const value = get_value_function();
                    if (dups.contains(value)) {
                        std::string err = "ValueError: Value " + print_function(value) + " is not unique in the key column.";
                        throw std::runtime_error(err.c_str());
                    }
                    dups.insert(value);
                }
            };

            using check_dup_func = std::function<void(elem_type const &)>;
            std::array<check_dup_func, static_cast<std::size_t>(column_type::sz) - 1> type2key_check_func {
                    [&](auto const & elem) {
                        return check_dup_func_impl(elem
                                , [&] {return (elem.is_boolean(), static_cast<bool>(elem.unsafe())); }
                                , [&] (auto & value) {
                                    std::ostringstream oss;
                                    oss.imbue(std::locale(std::locale("C"), new custom_boolean_facet));
                                    oss << std::boolalpha << value;
                                    return oss.str();
                                }
                        );
                    }
                    , [&](auto const & elem) {
                        return check_dup_func_impl(elem, [&] { return elem.num(); }, [&elem] (auto &) { return carefully_adjusted_number<0>(elem); });
                    }
                    , [&](auto const & elem) {
                        return check_dup_func_impl(elem, [&] {return datetime_time_point(elem); }, [&] (auto & value) { return datetime_s_json(value); });
                    }
                    , [&](auto const & elem) {
                        return check_dup_func_impl(elem, [&] {return date_time_point(elem); }, [] (auto & value) { return date_s(value); });
                    }
                    , [&](auto const & elem) {
                        return check_dup_func_impl(elem
                                , [&] { return elem.timedelta_seconds(); }
                                , [] (auto & value) { std::ostringstream oss; oss << std::quoted(time_storage().str(value)); return oss.str();});
                    }
                    , [&](auto const & elem) {
                        return check_dup_func_impl(elem, [&] {return compose_text(elem); }, [&elem] (auto &) { return compose_text(elem); });
                    }
            };

            reader.run_rows([&] (auto & row_span) {
                type2key_check_func[static_cast<std::size_t>(types[key_idx]) - 1](elem_type{row_span[key_idx]});
            });

        }

        using output_func_type = std::function<std::string(elem_type const &)>;
        struct json_rep {};
        static std::array<output_func_type, static_cast<std::size_t>(column_type::sz) - 1> type2output_func {
                compose_bool_1_arg<elem_type,json_rep>
                , [&](auto const & elem) {
                    assert(!elem.is_null());
                    return carefully_adjusted_number<1u>(elem);
                }
                , compose_datetime_1_arg<elem_type,json_rep>
                , compose_date_1_arg<elem_type,json_rep>
                , [&](auto const & elem) {
                    typename elem_type::template rebind<unquoted>::other const & another_rep = elem;
                    return "\"" + std::get<1>(another_rep.timedelta_tuple()) +'"';
                }
                , [&](auto const & elem) {
                    std::stringstream ss;
                    ss << std::quoted(compose_text(elem));
                    return ss.str();
                }
        };

        struct print_func {
            print_func(args_type const & a, types_type const & ts, blanks_type const & blks) : args(a), types(ts), blanks(blks) {}
            std::string operator()(output_func_type & f, elem_type const & elem, std::size_t col) const {
                auto print_func_impl = [&](auto && elem_str, bool quote = false) {
                    if (quote) {
                        std::ostringstream oss;
                        oss << std::quoted(elem_str);
                        return oss.str();
                    } else
                        return std::string(elem_str);
                };

                if (elem.is_null_value())
                    return print_func_impl(std::string("null"));

                bool const is_null = elem.is_null();

                if (types[col] == column_type::text_t or (!args.blanks && is_null))
                    return !args.blanks && is_null ? (!args.no_inference ? print_func_impl(std::string("null")) : print_func_impl(std::string(""), true)) : print_func_impl(elem.str(), true);

                assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);
                return print_func_impl(f(elem));
            }
            private:
                args_type const & args;
                types_type const & types;
                blanks_type const & blanks;
        };

        using output_key_func_type = std::function<std::string(elem_type const &)>;
        std::array<output_key_func_type, static_cast<std::size_t>(column_type::sz) - 1> type2output_key_func {
                [] (auto & elem) {
                    std::ostringstream oss;
                    oss.imbue(std::locale(std::locale("C"), new custom_boolean_facet));
                    oss << std::boolalpha << (elem.is_boolean(), static_cast<bool>(elem.unsafe()));
                    return oss.str();
                }
                , [] (auto & elem) { return carefully_adjusted_number<0>(elem); }
                , [] (auto & elem) { return datetime_s_json(elem); }
                , [] (auto & elem) { return date_s(elem); }
                , [] (auto & elem) {
                    typename elem_type::template rebind<unquoted>::other const & another_rep = elem;
                    return std::get<1>(another_rep.timedelta_tuple());
                }
                , [] (auto & elem) { return compose_text(elem); }
        };
        auto print_key_func = [&]<class Elem, class F> (F & f, Elem && elem, std::size_t col) {
            auto print_func_impl = [&](auto && elem_str, bool quote = true) {
                if (quote) {
                    std::ostringstream oss;
                    oss << std::quoted(elem_str);
                    return oss.str();
                } else {
                    return std::string(elem_str);
                }
            };

            if (elem.is_null_value())
                return print_func_impl(std::string(R"("None")"));

            bool const is_null = elem.is_null();
            if (types[col] == column_type::text_t or (!args.blanks && is_null))
                return !args.blanks && is_null ? print_func_impl(std::string(R"("None")")) : print_func_impl(elem.str(), true);

            assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);
            return print_func_impl(f(elem));
        };

        json_indenter indenter(args.indent);

        static std::ostringstream oss;

        if (!geojson) {
            if (!args.key.empty())
                args.maxfieldsize = max_unsigned_limit;

            if (!args.stream)
                oss << (args.key.empty() ? '[' : '{');

            std::size_t row = 0;
            auto const rows = reader.rows();

            reader.run_rows([&] (auto & row_span) {
                if (!args.key.empty()) {
                    auto const key = elem_type{row_span[key_idx]};
                    to_stream(oss
                            , indenter.add_indent()
                            , print_key_func(type2output_key_func[static_cast<std::size_t>(types[key_idx]) - 1], key, key_idx)
                            , ": {");
                } else
                    to_stream(oss, indenter.add_indent(), "{");

                indenter.inc_indent();
                auto i = 0u;
                for (auto & e : row_span) {
                    to_stream(oss
                            , indenter.add_indent()
                            , std::quoted(header[i].operator unquoted_cell_string())
                            , ": "
                            , print_func(args,types,blanks)(type2output_func[static_cast<std::size_t>(types[i]) - 1], elem_type{e}, i));
                    if (++i != row_span.size())
                        oss << ", ";
                }
                indenter.dec_indent();
                to_stream(oss, indenter.add_indent(), "}");
                if (++row != rows)
                    oss << (!args.stream ? ", " : "\n");
            });

            if (!args.stream)
                to_stream(oss, indenter.add_lf(), (args.key.empty() ? ']' : '}'));
            else
                to_stream(oss, '\n');
        } else { //geojson

            static auto const lat_column = match_column_identifier(header, args.lat.c_str(), get_column_offset(args));
            if (types[lat_column] != column_type::number_t and types[lat_column] != column_type::text_t)
                throw std::runtime_error("csvjson: error: latitude column must be string or real number.");
            if (blanks[lat_column])
                throw std::runtime_error("csvjson: error: latitude column contains blanks.");
            static auto const lon_column = match_column_identifier(header, args.lon.c_str(), get_column_offset(args));
            if (types[lon_column] != column_type::number_t and types[lat_column] != column_type::text_t)
                throw std::runtime_error("csvjson: error: longitude column must be string or real number.");
            if (blanks[lon_column])
                throw std::runtime_error("csvjson: error: longitude column contains blanks.");
            static auto const type_column = !args.type.empty() ? match_column_identifier(header, args.type.c_str(), get_column_offset(args)) : std::numeric_limits<unsigned>::max();
            static auto const key_column = !args.key.empty() ? match_column_identifier(header, args.key.c_str(), get_column_offset(args)) : std::numeric_limits<unsigned>::max();

            long double min_lat = std::numeric_limits<long double>::max(), max_lat = std::numeric_limits<long double>::lowest();
            long double min_lon = std::numeric_limits<long double>::max(), max_lon = std::numeric_limits<long double>::lowest();
            elem_type min_lat_elem, max_lat_elem, min_lon_elem, max_lon_elem;

            if (!args.no_bbox and !args.stream) {
                reader.run_rows([&](auto &row_span) {
                    auto update_min_max = [&](auto & e, long double & max_, long double & min_, elem_type & max_e, elem_type & min_e) {
                        auto const et = elem_type{e};
                        auto const element_value = et.num();
                        if (element_value >= max_) {
                            max_ = element_value;
                            max_e = et;
                        }
                        if (element_value <= min_) {
                            min_ = element_value;
                            min_e = et;
                        }
                    };
                    auto i = 0u;
                    for (auto &e: row_span) {
                        if (i == lat_column)
                            update_min_max(e, max_lat, min_lat, max_lat_elem, min_lat_elem);
                        else if (i == lon_column)
                            update_min_max(e, max_lon, min_lon, max_lon_elem, min_lon_elem);
                        ++i;
                    }
                });
            }

            struct not_stream_args {
                args_type args;
                elem_type min_lon_elem;
                elem_type min_lat_elem;
                elem_type max_lon_elem;
                elem_type max_lat_elem;
            };

            struct not_stream_printer {
                not_stream_printer(json_indenter const & indenter, not_stream_args const & n_s_args)
                    : indenter(indenter), args(n_s_args.args) {
                    if (!args.stream) {
                        oss << '{';
                        to_stream(oss, indenter.add_indent(), R"("type": "FeatureCollection", )");

                        if (!args.no_bbox) {
                            to_stream(oss, indenter.add_indent(), R"("bbox": [)");
                            indenter.inc_indent();
                            to_stream(oss
                                , indenter.add_indent()
                                , carefully_adjusted_number<1u>(n_s_args.min_lon_elem), ", ", indenter.add_indent()
                                , carefully_adjusted_number<1u>(n_s_args.min_lat_elem), ", ", indenter.add_indent()
                                , carefully_adjusted_number<1u>(n_s_args.max_lon_elem), ", ", indenter.add_indent()
                                , carefully_adjusted_number<1u>(n_s_args.max_lat_elem));
                            indenter.dec_indent();
                            to_stream(oss, indenter.add_indent(), "], ");
                        }
                        to_stream(oss, indenter.add_indent(), R"("features": [)");
                        indenter.inc_indent();
                    }
                }
                ~not_stream_printer() {
                    if (!args.stream) {
                        indenter.dec_indent();
                        to_stream(oss, indenter.add_indent(), ']');
                        if (!args.crs.empty()) {
                            to_stream(oss, ',', indenter.add_indent(), R"("crs": {)");
                            indenter.inc_indent();
                            to_stream(oss, indenter.add_indent(), R"("type": "name")", ", ");
                            to_stream(oss, indenter.add_indent(), R"("properties": {)");
                            indenter.inc_indent();
                            to_stream(oss, indenter.add_indent(), R"("name": )", std::quoted(args.crs));
                            indenter.dec_indent();
                            to_stream(oss, indenter.add_indent(), "}");
                            indenter.dec_indent();
                            to_stream(oss, indenter.add_indent(), '}');
                        }
                        to_stream(oss, indenter.add_lf(), '}');
                    }
                }
            private:
                json_indenter const & indenter;
                args_type const & args;
            };

            static not_stream_args nsargs {args, min_lon_elem, min_lat_elem, max_lon_elem, max_lat_elem};
            not_stream_printer nsp (indenter, nsargs);

            std::size_t row = 0;
            auto const rows = reader.rows();

            struct props_args {
                args_type args;
                types_type tps;
                blanks_type blanks;
                unsigned lon_column;
                unsigned lat_column;
                unsigned key_col;
                unsigned type_col;
                header_type hdr;
            };

            struct key_args {
                args_type args;
                types_type tps;
                blanks_type blanks;
                unsigned key_col;
            };

            if (!args.no_bbox and !args.stream)
                args.maxfieldsize = max_unsigned_limit;

            struct item_scope_printer {
                explicit item_scope_printer(json_indenter const & indenter)
                        : indenter(indenter) { to_stream(oss, indenter.add_indent(), '{'); indenter.inc_indent(); }

                ~item_scope_printer() { indenter.dec_indent(); to_stream(oss, indenter.add_indent(), '}'); }
            private:
                json_indenter const & indenter;
            };

            reader.run_rows([&] (auto & row_span) {

                struct feature_printer {
                    explicit feature_printer(json_indenter const & indenter) {
                        to_stream(oss, indenter.add_indent(), R"("type": "Feature")", ", ");
                    }
                };

                using row_span_type = decltype(row_span);

                struct properties_printer {
                    properties_printer(json_indenter const & indenter, row_span_type && row_span, props_args const & pa) : indenter(indenter) {
                        to_stream(oss, indenter.add_indent(), R"("properties": {)");
                        indenter.inc_indent();
                        auto i = 0u;
                        bool at_least_one_prop{};
                        for (auto & e : row_span) {
                            if (i == pa.lat_column)
                                lat_elem = e;
                            else if (i == pa.lon_column)
                                lon_elem = e;
                            else if (i == pa.key_col)
                                key_elem = e;
                            else if (i != pa.type_col) {
                                if (!elem_type{e}.is_null()) {
                                    if (at_least_one_prop)
                                        oss << ", ";
                                    to_stream(oss, indenter.add_indent()
                                                 , std::quoted(pa.hdr[i].operator unquoted_cell_string()), ": "
                                                 , print_func(pa.args, pa.tps, pa.blanks)
                                                     (type2output_func[static_cast<std::size_t>(pa.tps[i]) - 1], elem_type{e}, i)
                                             );
                                    at_least_one_prop = true;
                                }
                            }
                            ++i;
                        }

                        indenter.dec_indent();
                        to_stream(oss, (at_least_one_prop ? indenter.add_indent() : ""), "}, ");
                    }
                    std::tuple<elem_type, elem_type, elem_type> get_elems() const noexcept {
                        return std::tuple{lon_elem, lat_elem, key_elem};
                    }
                private:
                    elem_type lat_elem;
                    elem_type lon_elem;
                    elem_type key_elem;
                    json_indenter const & indenter;
                };

                struct key_printer {
                    key_printer(json_indenter const & indenter, elem_type const & key, key_args const & ka) {
                        if (!ka.args.key.empty() && !key.is_null()) {
                            to_stream(oss
                                    , indenter.add_indent()
                                    , std::quoted("id"), ": "
                                    , print_func(ka.args, ka.tps, ka.blanks)
                                        (type2output_func[static_cast<std::size_t>(ka.tps[ka.key_col]) - 1], key, ka.key_col)
                                    , ", ");
                        }
                    }
                };

                struct geometry_printer {
                    explicit geometry_printer(json_indenter const &indenter, bool is_stream, elem_type const & lon, elem_type const & lat) /*: indenter(indenter)*/ {
                        bool const geometry_is_null = is_stream and (!lon.is_num() or !lat.is_num());
                        if (!geometry_is_null) {
                            to_stream(oss, indenter.add_indent(), R"("geometry": {)");
                            indenter.inc_indent();
                            {
                                coordinates_printer cp(indenter, lon, lat);
                            }
                            indenter.dec_indent();
                            to_stream(oss, indenter.add_indent(), '}');
                        } else
                            to_stream(oss, indenter.add_indent(), R"("geometry": null)");
                    }
                private:
                    struct coordinates_printer {
                        coordinates_printer(json_indenter const &indenter, elem_type const & lon, elem_type const & lat) {
                            to_stream(oss, indenter.add_indent(), R"("type": "Point")", ", ");
                            to_stream(oss, indenter.add_indent(), R"("coordinates": [)");
                            indenter.inc_indent();
                            to_stream(oss, indenter.add_indent(), carefully_adjusted_number<1u>(lon), ", ");
                            to_stream(oss, indenter.add_indent(), carefully_adjusted_number<1u>(lat));

                            indenter.dec_indent();
                            to_stream(oss, indenter.add_indent(), ']');
                        }
                    };
                };

                {
                    item_scope_printer isp(indenter);
                    feature_printer fp(indenter);
                    std::tuple<elem_type, elem_type, elem_type> llk;
                    {
                        static props_args pargs {args, types, blanks, lon_column, lat_column, key_column, type_column, header};
                        properties_printer pp(indenter, row_span, pargs);
                        llk = pp.get_elems();
                    }
                    static const key_args kargs {args, types, blanks, key_column};
                    key_printer kp(indenter, std::get<2>(llk), kargs);
                    geometry_printer gp(indenter, args.stream, std::get<0>(llk), std::get<1>(llk));
                }

                if (++row != rows and !args.stream)
                    oss <<  ", ";
                if (args.stream and args.indent == min_int_limit)
                    oss << '\n';

            }); //RUN_ROWS...............

        }
        std::cout << oss.str();
    }

} /// namespace

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[])
{
    using namespace csvjson;

    auto const args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(read_standard_input, json)

    return 0;
}

#endif
