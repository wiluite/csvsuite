///
/// \file   test/csvjson_test.cpp
/// \author wiluite
/// \brief  Tests for the csvjson utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../csvjson.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "test_reader_macros.h"
#include "test_max_field_size_macros.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT std::stringstream cout_buffer;                        \
                                       {                                                     \
                                           redirect(cout)                                    \
                                           redirect_cout cr(cout_buffer.rdbuf());            \
                                           csvjson::json(r, args);                           \
                                       }

#if !defined(__unix__)
#undef GetObject
#endif

int main() {
    using namespace boost::ut;
    using namespace rapidjson;

    namespace tf = csvsuite::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    struct csvjson_specific_args {
        int indent {min_int_limit};
        std::string key;
        std::string lat;
        std::string lon;
        std::string type;
        std::string geometry;
        std::string crs;
        bool no_bbox {false};
        bool stream {false};
    };

    "simple"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "dummy.csv"; }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());
        for (auto &e: document.GetArray()) {
            expect(e.HasMember("a"));
            expect(e["a"].IsBool());
            expect(e["a"].GetBool());
            expect(e.HasMember("b"));
            expect(e["b"].IsDouble());
            expect(e["b"].GetDouble() == 2.0);
            expect(e.HasMember("c"));
            expect(e["c"].IsDouble());
            expect(e["c"].GetDouble() == 3.0);
        }
    };

    "no blanks"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "blanks.csv"; }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());
        for (auto &e: document.GetArray()) {
            expect(e.HasMember("a"));
            expect(e["a"].IsNull());
            expect(e.HasMember("b"));
            expect(e["b"].IsNull());
            expect(e.HasMember("c"));
            expect(e["c"].IsNull());
            expect(e.HasMember("d"));
            expect(e["d"].IsNull());
            expect(e.HasMember("e"));
            expect(e["e"].IsNull());
            expect(e.HasMember("f"));
            expect(e["f"].IsNull());
        }
    };

    "blanks"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "blanks.csv";
                blanks = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());
        for (auto &e: document.GetArray()) {
            expect(e.HasMember("a"));
            expect(e["a"].IsString());
            expect(e["a"].GetString() == std::string(""));
            expect(e.HasMember("b"));
            expect(e["b"].IsString());
            expect(e["b"].GetString() == std::string("NA"));
            expect(e.HasMember("c"));
            expect(e["c"].IsString());
            expect(e["c"].GetString() == std::string("N/A"));
            expect(e.HasMember("d"));
            expect(e["d"].IsString());
            expect(e["d"].GetString() == std::string("NONE"));
            expect(e.HasMember("e"));
            expect(e["e"].IsString());
            expect(e["e"].GetString() == std::string("NULL"));
            expect(e.HasMember("f"));
            expect(e["f"].IsString());
            expect(e["f"].GetString() == std::string("."));
        }
    };

    "no header row"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "no_header_row.csv";
                no_header = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());
        for (auto &e: document.GetArray()) {
            expect(e.HasMember("a"));
            expect(e["a"].IsBool());
            expect(e["a"].GetBool());
            expect(e.HasMember("b"));
            expect(e["b"].IsDouble());
            expect(e["b"].GetDouble() == 2.0);
            expect(e.HasMember("c"));
            expect(e["c"].IsDouble());
            expect(e["c"].GetDouble() == 3.0);
        }
    };

    "no inference"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "dummy.csv";
                no_inference = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());
        for (auto &e: document.GetArray()) {
            expect(e.HasMember("a"));
            expect(e["a"].IsString());
            expect(e["a"].GetString() == std::string("1"));
            expect(e.HasMember("b"));
            expect(e["b"].IsString());
            expect(e["b"].GetString() == std::string("2"));
            expect(e.HasMember("c"));
            expect(e["c"].IsString());
            expect(e["c"].GetString() == std::string("3"));

        }
    };

    "indentation"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "dummy.csv";
                indent = 4;
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str().find(R"(        "a": true,)") != std::string::npos);
    };

    "keying"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "dummy.csv";
                key = "a";
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(R"({"True": {"a": true, "b": 2.0, "c": 3.0}})" == cout_buffer.str());

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsObject());
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        expect(R"({"True":{"a":true,"b":2.0,"c":3.0}})" == std::string(buffer.GetString()));

#if 0
        if (0) {
            expect(document["True"].IsObject());
            static const char *kTypeNames[] = {"Null", "False", "True", "Object", "Array", "String", "Number"};
            for (auto const &e: document["True"].GetObject()) {
                std::cout << e.name.GetString() << " " << kTypeNames[e.value.GetType()] << std::endl;
            }
            expect(document["True"]["a"].GetBool());
            expect(document["True"]["b"].GetDouble() == 2);
        }
#endif
    };

    "duplicate keys"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "dummy3.csv";
                key = "a";
            }
        } args;

        notrimming_reader_type r(args.file);

        expect(throws<std::runtime_error>([&] { csvjson::json(r, args); }));

        try { csvjson::json(r, args); } catch (std::runtime_error const &e) {
            expect(e.what() == std::string("ValueError: Value True is not unique in the key column."));
        }
    };

    "geojson with id"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "test_geo.csv";
                lat = "latitude";
                lon = "longitude";
                key = "slug"; /*indent=1;*/}
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.HasMember("type"));
        expect(document["type"].GetString() == std::string("FeatureCollection"));
        expect(cout_buffer.str().find("crc") == std::string::npos);
        expect(document.HasMember("bbox"));
        expect(document["bbox"].IsArray());
        expect(document["bbox"].GetArray().Size() == 4);
        expect(document["bbox"].GetArray()[0].GetDouble() == -95.334619);
        expect(document["bbox"].GetArray()[1].GetDouble() == 32.299076986939205);
        expect(document["bbox"].GetArray()[2].GetDouble() == -95.250699);
        expect(document["bbox"].GetArray()[3].GetDouble() == 32.351434);
        expect(document["features"].IsArray());
        expect(document["features"].GetArray().Size() == 17);
        for (auto &e: document["features"].GetArray()) {
            expect(e["type"].GetString() == std::string("Feature"));
            expect(e.HasMember("id"));
            expect(e.HasMember("properties"));
            expect(e["properties"].IsObject());

            expect(e["properties"].MemberEnd() - e["properties"].MemberBegin() > 1);

            expect(e["geometry"]["coordinates"].IsArray());
            expect(e["geometry"]["coordinates"].GetArray().Size() == 2);
            static const char *kTypeNames[] = {"Null", "False", "True", "Object", "Array", "String", "Number"};
            expect(kTypeNames[e["geometry"]["coordinates"][0].GetType()] == std::string("Number"));
            expect(kTypeNames[e["geometry"]["coordinates"][1].GetType()] == std::string("Number"));
        }
    };

    "geojson point"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() {
                file = "test_geo.csv";
                lat = "latitude";
                lon = "longitude";
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());

        for (auto &e: document["features"].GetArray())
            expect(!e.HasMember("id"));
    };

    //TODO: document no geometry option support

    "geojson with crs"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "test_geo.csv"; lat = "latitude"; lon = "longitude"; crs = "EPSG:4269";
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(document.HasMember("crs"));
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document["crs"].Accept(writer);
        expect(R"({"type":"name","properties":{"name":"EPSG:4269"}})" == std::string(buffer.GetString()));
    };

    "geojson with no bbox"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "test_geo.csv"; lat = "latitude"; lon = "longitude"; no_bbox = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasMember("bbox"));
    };

    "ndjson"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "testjson_converted.csv"; stream = true; }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect( cout_buffer.str() ==
R"({"text": "Chicago Reader", "float": 1.0, "datetime": "1971-01-01T04:14:00", "boolean": true, "time": "4:14:00", "date": "1971-01-01", "integer": 40.0}
{"text": "Chicago Sun-Times", "float": 1.27, "datetime": "1948-01-01T14:57:13", "boolean": true, "time": "14:57:13", "date": "1948-01-01", "integer": 63.0}
{"text": "Chicago Tribune", "float": 41800000.01, "datetime": "1920-01-01T00:00:00", "boolean": false, "time": "0:00:00", "date": "1920-01-01", "integer": 164.0}
{"text": "This row has blanks", "float": null, "datetime": null, "boolean": null, "time": null, "date": null, "integer": null}
{"text": "Unicode! Σ", "float": null, "datetime": null, "boolean": null, "time": null, "date": null, "integer": null}
)");
    };

    "ndjson streaming, no inference"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "testjson_converted.csv"; stream = true; no_inference = true; }
        } args;

        notrimming_reader_type r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect( cout_buffer.str() ==
R"({"text": "Chicago Reader", "float": "1.0", "datetime": "1971-01-01T04:14:00", "boolean": "True", "time": "4:14:00", "date": "1971-01-01", "integer": "40"}
{"text": "Chicago Sun-Times", "float": "1.27", "datetime": "1948-01-01T14:57:13", "boolean": "True", "time": "14:57:13", "date": "1948-01-01", "integer": "63"}
{"text": "Chicago Tribune", "float": "41800000.01", "datetime": "1920-01-01T00:00:00", "boolean": "False", "time": "0:00:00", "date": "1920-01-01", "integer": "164"}
{"text": "This row has blanks", "float": "", "datetime": "", "boolean": "", "time": "", "date": "", "integer": ""}
{"text": "Unicode! Σ", "float": "", "datetime": "", "boolean": "", "time": "", "date": "", "integer": ""}
)");
    };

    "ndgeojson"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "test_geo.csv"; lat = "latitude"; lon = "longitude"; stream = true; date_fmt = "%m/%d/%y";
            }
        } args;

        test_reader_r1 r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() ==
R"({"type": "Feature", "properties": {"slug": "dcl", "title": "Downtown Coffee Lounge", "description": "In addition to being the only coffee shop in downtown Tyler, DCL also features regular exhibitions of work by local artists.", "address": "200 West Erwin Street", "type": "Gallery", "last_seen_date": "2012-03-30"}, "geometry": {"type": "Point", "coordinates": [-95.30181, 32.35066]}}
{"type": "Feature", "properties": {"slug": "tyler-museum", "title": "Tyler Museum of Art", "description": "The Tyler Museum of Art on the campus of Tyler Junior College is the largest art museum in Tyler. Visit them online at <a href=\"http://www.tylermuseum.org/\">http://www.tylermuseum.org/</a>.", "address": "1300 South Mahon Avenue", "type": "Museum", "last_seen_date": "2012-04-02"}, "geometry": {"type": "Point", "coordinates": [-95.28174, 32.33396]}}
{"type": "Feature", "properties": {"slug": "genecov", "title": "Genecov Sculpture", "description": "Stainless Steel Sculpture", "address": "1350 Dominion Plaza", "type": "Sculpture", "photo_url": "http://i.imgur.com/DICdn.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-04"}, "geometry": {"type": "Point", "coordinates": [-95.31571447849274, 32.299076986939205]}}
{"type": "Feature", "properties": {"slug": "gallery-main-street", "title": "Gallery Main Street", "description": "The only dedicated art gallery in Tyler. Visit them online at <a href=\"http://www.heartoftyler.com/downtowntylerarts/\">http://www.heartoftyler.com/downtowntylerarts/</a>.", "address": "110 West Erwin Street", "type": "Gallery", "last_seen_date": "2012-04-09"}, "geometry": {"type": "Point", "coordinates": [-95.30123, 32.35066]}}
{"type": "Feature", "properties": {"slug": "spirit-of-progress", "title": "The Spirit of Progress", "address": "100 block of North Spring Avenue", "type": "Relief", "photo_url": "http://media.hacktyler.com/artmap/photos/spirit-of-progress.jpg", "photo_credit": "Photo by Christopher Groskopf. Used with permission.", "last_seen_date": "2012-04-11"}, "geometry": {"type": "Point", "coordinates": [-95.2995, 32.3513]}}
{"type": "Feature", "properties": {"slug": "celestial-conversations-2", "title": "Celestial Conversations #2", "artist": "Simon Saleh", "description": "Steel Sculpture", "address": "100 block of North Spring Avenue", "type": "Sculpture", "photo_url": "http://media.hacktyler.com/artmap/photos/celestial-conversations-2.jpg", "photo_credit": "Photo by Christopher Groskopf. Used with permission.", "last_seen_date": "2012-04-11"}, "geometry": {"type": "Point", "coordinates": [-95.2995, 32.35100]}}
{"type": "Feature", "properties": {"slug": "pivot-bounce", "title": "Pivot Bounce", "artist": "Chelsea Cope", "address": "100 block of North Spring Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/pmxyi.jpg?1", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-11"}, "geometry": {"type": "Point", "coordinates": [-95.29944, 32.351434]}}
{"type": "Feature", "properties": {"slug": "children-of-peace", "title": "Children of Peace", "artist": "Gary Price", "description": "Cast Bronze", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/rUikO.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.300222, 32.339826]}}
{"type": "Feature", "properties": {"slug": "ross-bears", "title": "Ross' Bears", "description": "Granite Sculpture", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/SpJQI.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.300034, 32.339776]}}
{"type": "Feature", "properties": {"slug": "butterfly-garden", "title": "Butterfly Garden", "description": "Cast Bronze", "type": "Sculpture", "photo_url": "http://i.imgur.com/0L8DF.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.300219, 32.339559]}}
{"type": "Feature", "properties": {"slug": "goose-fountain", "title": "TJC Goose Fountain", "description": "Copper (?) Sculpture", "address": "1300 S. Mahon Ave.", "type": "Sculpture", "photo_url": "http://i.imgur.com/UWfS6.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.28263, 32.333944]}}
{"type": "Feature", "properties": {"slug": "tjc-cement-totems", "description": "Cast Cement Totems", "address": "1300 S. Mahon Ave.", "type": "Sculpture", "photo_url": "http://i.imgur.com/lRmYd.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.283894, 32.333899]}}
{"type": "Feature", "properties": {"slug": "alison", "title": "Alison", "description": "Cast Bronze", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/7OcrG.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.299887, 32.339809]}}
{"type": "Feature", "properties": {"slug": "jackson", "title": "Jackson", "description": "Cast Bronze", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/aQJfv.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.299939, 32.339828]}}
{"type": "Feature", "properties": {"slug": "505-third", "title": "Untitled", "description": "Stainless Steel", "address": "505 Third St.", "type": "Sculpture", "photo_url": "http://i.imgur.com/0moUY.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.305429, 32.333082]}}
{"type": "Feature", "properties": {"slug": "obeidder", "title": "Obeidder Monster", "description": "Sharpie and Spray Paint", "address": "3319 Seaton St.", "type": "Street Art", "photo_url": "http://i.imgur.com/3aX7E.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "2012-04-15"}, "geometry": {"type": "Point", "coordinates": [-95.334619, 32.314431]}}
{"type": "Feature", "properties": {"slug": "sensor-device", "title": "Sensor Device", "artist": "Kurt Dyrhaug", "address": "University of Texas, Campus Drive", "type": "Sculpture", "photo_url": "http://media.hacktyler.com/artmap/photos/sensor-device.jpg", "photo_credit": "Photo by Christopher Groskopf. Used with permission.", "last_seen_date": "2012-04-16"}, "geometry": {"type": "Point", "coordinates": [-95.250699, 32.317216]}}
)");
    };

    "ndgeojson streaming, no inference"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "test_geo.csv"; lat = "latitude"; lon = "longitude"; stream = true; date_fmt = "%m/%d/%y"; no_inference = true;
            }
        } args;

        test_reader_r2 r(args.file);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() ==
R"({"type": "Feature", "properties": {"slug": "dcl", "title": "Downtown Coffee Lounge", "description": "In addition to being the only coffee shop in downtown Tyler, DCL also features regular exhibitions of work by local artists.", "address": "200 West Erwin Street", "type": "Gallery", "last_seen_date": "3/30/12"}, "geometry": {"type": "Point", "coordinates": [-95.30181, 32.35066]}}
{"type": "Feature", "properties": {"slug": "tyler-museum", "title": "Tyler Museum of Art", "description": "The Tyler Museum of Art on the campus of Tyler Junior College is the largest art museum in Tyler. Visit them online at <a href=\"http://www.tylermuseum.org/\">http://www.tylermuseum.org/</a>.", "address": "1300 South Mahon Avenue", "type": "Museum", "last_seen_date": "4/2/12"}, "geometry": {"type": "Point", "coordinates": [-95.28174, 32.33396]}}
{"type": "Feature", "properties": {"slug": "genecov", "title": "Genecov Sculpture", "description": "Stainless Steel Sculpture", "address": "1350 Dominion Plaza", "type": "Sculpture", "photo_url": "http://i.imgur.com/DICdn.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/4/12"}, "geometry": {"type": "Point", "coordinates": [-95.31571447849274, 32.299076986939205]}}
{"type": "Feature", "properties": {"slug": "gallery-main-street", "title": "Gallery Main Street", "description": "The only dedicated art gallery in Tyler. Visit them online at <a href=\"http://www.heartoftyler.com/downtowntylerarts/\">http://www.heartoftyler.com/downtowntylerarts/</a>.", "address": "110 West Erwin Street", "type": "Gallery", "last_seen_date": "4/9/12"}, "geometry": {"type": "Point", "coordinates": [-95.30123, 32.35066]}}
{"type": "Feature", "properties": {"slug": "spirit-of-progress", "title": "The Spirit of Progress", "address": "100 block of North Spring Avenue", "type": "Relief", "photo_url": "http://media.hacktyler.com/artmap/photos/spirit-of-progress.jpg", "photo_credit": "Photo by Christopher Groskopf. Used with permission.", "last_seen_date": "4/11/12"}, "geometry": {"type": "Point", "coordinates": [-95.2995, 32.3513]}}
{"type": "Feature", "properties": {"slug": "celestial-conversations-2", "title": "Celestial Conversations #2", "artist": "Simon Saleh", "description": "Steel Sculpture", "address": "100 block of North Spring Avenue", "type": "Sculpture", "photo_url": "http://media.hacktyler.com/artmap/photos/celestial-conversations-2.jpg", "photo_credit": "Photo by Christopher Groskopf. Used with permission.", "last_seen_date": "4/11/12"}, "geometry": {"type": "Point", "coordinates": [-95.2995, 32.35100]}}
{"type": "Feature", "properties": {"slug": "pivot-bounce", "title": "Pivot Bounce", "artist": "Chelsea Cope", "address": "100 block of North Spring Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/pmxyi.jpg?1", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/11/12"}, "geometry": {"type": "Point", "coordinates": [-95.29944, 32.351434]}}
{"type": "Feature", "properties": {"slug": "children-of-peace", "title": "Children of Peace", "artist": "Gary Price", "description": "Cast Bronze", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/rUikO.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.300222, 32.339826]}}
{"type": "Feature", "properties": {"slug": "ross-bears", "title": "Ross' Bears", "description": "Granite Sculpture", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/SpJQI.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.300034, 32.339776]}}
{"type": "Feature", "properties": {"slug": "butterfly-garden", "title": "Butterfly Garden", "description": "Cast Bronze", "type": "Sculpture", "photo_url": "http://i.imgur.com/0L8DF.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.300219, 32.339559]}}
{"type": "Feature", "properties": {"slug": "goose-fountain", "title": "TJC Goose Fountain", "description": "Copper (?) Sculpture", "address": "1300 S. Mahon Ave.", "type": "Sculpture", "photo_url": "http://i.imgur.com/UWfS6.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.28263, 32.333944]}}
{"type": "Feature", "properties": {"slug": "tjc-cement-totems", "description": "Cast Cement Totems", "address": "1300 S. Mahon Ave.", "type": "Sculpture", "photo_url": "http://i.imgur.com/lRmYd.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.283894, 32.333899]}}
{"type": "Feature", "properties": {"slug": "alison", "title": "Alison", "description": "Cast Bronze", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/7OcrG.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.299887, 32.339809]}}
{"type": "Feature", "properties": {"slug": "jackson", "title": "Jackson", "description": "Cast Bronze", "address": "900 South Broadway Avenue", "type": "Sculpture", "photo_url": "http://i.imgur.com/aQJfv.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.299939, 32.339828]}}
{"type": "Feature", "properties": {"slug": "505-third", "title": "Untitled", "description": "Stainless Steel", "address": "505 Third St.", "type": "Sculpture", "photo_url": "http://i.imgur.com/0moUY.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.305429, 32.333082]}}
{"type": "Feature", "properties": {"slug": "obeidder", "title": "Obeidder Monster", "description": "Sharpie and Spray Paint", "address": "3319 Seaton St.", "type": "Street Art", "photo_url": "http://i.imgur.com/3aX7E.jpg", "photo_credit": "Photo by Justin Edwards. Used with permission.", "last_seen_date": "4/15/12"}, "geometry": {"type": "Point", "coordinates": [-95.334619, 32.314431]}}
{"type": "Feature", "properties": {"slug": "sensor-device", "title": "Sensor Device", "artist": "Kurt Dyrhaug", "address": "University of Texas, Campus Drive", "type": "Sculpture", "photo_url": "http://media.hacktyler.com/artmap/photos/sensor-device.jpg", "photo_credit": "Photo by Christopher Groskopf. Used with permission.", "last_seen_date": "4/16/12"}, "geometry": {"type": "Point", "coordinates": [-95.250699, 32.317216]}}
)");

        "max field size in geojson mode"_test = [&] {
            args.file = "test_field_size_limit.csv";
            args.lat = "Along";
            args.lon = "Boasting";

            notrimming_reader_type r(args.file);

            expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT}));

            using namespace z_test;

            Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
#ifndef _MSC_VER
            // Fixme. Recent MSVC Community, Preview has bug in place for this test to work. You can safely uncomment this code but the test does not throw the (implied) exception here!
            Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#endif

            args.lat = "1";
            args.lon = "2";
#ifndef _MSC_VER
            // Fixme. Recent MSVC Community, Preview has bug in place for this test to work. You can safely uncomment this code but the test does not throw the (implied) exception here!
            Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
            Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
#endif
            Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
#ifndef _MSC_VER
            // Fixme. Recent MSVC Community, Preview has bug in place for this test to work. You can safely uncomment this code but the test does not throw the (implied) exception here!
            Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
#endif
        };
    };

    "max field size"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, csvjson_specific_args {
            Args() { file = "test_field_size_limit.csv"; }
        } args;

        notrimming_reader_type r(args.file);

        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT}));

        using namespace z_test;
        Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

        Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

        Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        Z_CHECK(csvjson::json, notrimming_reader_type, skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
    };

}

