#ifndef GEO_JSON_HH
#define GEO_JSON_HH

#ifdef WT_BUILDING
#include "Wt/WDllDefs.h"
#else
#ifndef WT_API
#define WT_API
#endif
#endif

#include <string>
#include <vector>
#include "gason/src/gason.h"
#include <sstream>

/////////////////////////////////////////////////////////////////////////////////////////////////////
//coord_t
/////////////////////////////////////////////////////////////////////////////////////////////////////

class coord_t
{
public:
  coord_t(double x_, double y_) :
    x(x_),
    y(y_)
  {
  }
  double x;
  double y;
};

///////////////////////////////////////////////////////////////////////////////////////
//polygon_t
///////////////////////////////////////////////////////////////////////////////////////

class polygon_t
{
public:
  polygon_t()
  {};
  std::vector<coord_t> m_coord;
};

///////////////////////////////////////////////////////////////////////////////////////
//geometry_t
///////////////////////////////////////////////////////////////////////////////////////

class geometry_t
{
public:
  geometry_t()
  {};
  std::string m_type; //"Polygon", "Point"
  std::vector<polygon_t> m_polygons;
};

///////////////////////////////////////////////////////////////////////////////////////
//feature_t
///////////////////////////////////////////////////////////////////////////////////////

struct prop {
  std::string name;
  std::string value;
};
//struct geojson : prop {};

class feature_t
{
public:
  feature_t()
  {};
  std::string m_name;
  std::vector<prop> props;
  std::string gjson;
  std::vector<geometry_t> m_geometry;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t
/////////////////////////////////////////////////////////////////////////////////////////////////////

class WT_API geojson_t
{
public:
  geojson_t()
  {
  }
  void convert_file(const char* file_name);
  void convert_stream(std::string &);

  //storage is a list of features 
  std::vector<feature_t> m_feature;

private:
  int parse_root(JsonValue value);
  int parse_features(JsonValue value);
  int parse_feature(JsonValue value);
  int parse_geometry(JsonValue value, feature_t &feature);
  int parse_coordinates(JsonValue value, const std::string &type, feature_t &feature);
  void dump_value(JsonValue & value, std::stringstream & ss, int indent = 0);
  void dump_string(const char *s, std::stringstream & ss);
};

#endif




