#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "geojson.hh"
#include <cmath>

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::convert
/////////////////////////////////////////////////////////////////////////////////////////////////////

void geojson_t::convert_file(const char* file_name)
{
  size_t length;

  auto f = fopen(file_name, "rb");
  if (!f)
  {
    throw std::runtime_error(std::string("Error opening the file: '") + file_name + "'.");
  }

  fseek(f, 0, SEEK_END);
  length = ftell(f);
  fseek(f, 0, SEEK_SET);
  auto buf = (char*)malloc(length);
  if (buf)
  {
    size_t nbr = fread(buf, 1, length, f);
  }
  fclose(f);

  char *endptr;
  JsonValue value;
  JsonAllocator allocator;
  int rc = jsonParse(buf, &endptr, &value, allocator);
  if (rc != JSON_OK)
  {
    free(buf);
    throw std::runtime_error("geojson: invalid JSON format.");
  }

  parse_root(value);
  free(buf);
}

void geojson_t::convert_stream(std::string & s) {
  char *endptr;
  JsonValue value;
  JsonAllocator allocator;
  int rc = jsonParse(s.data(), &endptr, &value, allocator);
  if (rc != JSON_OK)
  {
    throw std::runtime_error("geojson: invalid JSON format.");
  }

  parse_root(value);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_root
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_root(JsonValue value)
{
  for (JsonNode *node = value.toNode(); node != nullptr; node = node->next)
  {
    //JSON organized in hierarchical levels
    //level 0, root with objects: "type", "features"
    //FeatureCollection is not much more than an object that has "type": "FeatureCollection" 
    //and then an array of Feature objects under the key "features". 

    if (std::string(node->key).compare("type") == 0)
    {
      assert(node->value.getTag() == JSON_STRING);
      std::string str = node->value.toString();
      if (str.compare("Feature") == 0)
      {
        //parse the root, contains only one "Feature"
        parse_feature(value);
      }
    }
    if (std::string(node->key).compare("features") == 0)
    {
      assert(node->value.getTag() == JSON_ARRAY);
      parse_features(node->value);
    }
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_features
//array of Feature objects under the key "features". 
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_features(JsonValue value)
{
  assert(value.getTag() == JSON_ARRAY);
  size_t arr_size = 0; //size of array
  for (JsonNode *n_feat = value.toNode(); n_feat != nullptr; n_feat = n_feat->next)
  {
    arr_size++;
  }
  for (JsonNode *n_feat = value.toNode(); n_feat != nullptr; n_feat = n_feat->next)
  {
    JsonValue object = n_feat->value;
    assert(object.getTag() == JSON_OBJECT);
    parse_feature(object);
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_feature
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_feature(JsonValue value)
{
  JsonValue object = value;
  assert(object.getTag() == JSON_OBJECT);
  feature_t feature;

  //3 objects with keys: 
  // "type", 
  // "properties", 
  // "geometry"
  //"type" has a string value "Feature"
  //"properties" has a list of objects
  //"geometry" has 2 objects: 
  //key "type" with value string geometry type (e.g."Polygon") and
  //key "coordinates" an array

  for (JsonNode *obj = object.toNode(); obj != nullptr; obj = obj->next)
  {
    if (std::string(obj->key).compare("type") == 0)
    {
      assert(obj->value.getTag() == JSON_STRING);
    }
    else if (std::string(obj->key).compare("properties") == 0)
    {
      assert(obj->value.getTag() == JSON_OBJECT);
      for (JsonNode *prp = obj->value.toNode(); prp != nullptr; prp = prp->next)
      {
        std::stringstream ss;
        dump_value(prp->value, ss);
        feature.props.emplace_back(prop{prp->key, ss.str()});

        if (std::string(prp->key).compare("NAME") == 0 || std::string(prp->key).compare("name") == 0)
        {
          assert(prp->value.getTag() == JSON_STRING);
          feature.m_name = prp->value.toString();
          std::cout << "NAME: " << feature.m_name << std::endl;
        }
      }
    }
    else if (std::string(obj->key).compare("geometry") == 0)
    {
      assert(obj->value.getTag() == JSON_OBJECT);
      parse_geometry(obj->value, feature);
    }
  }
  m_feature.push_back(feature);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_geometry
//"geometry" has 2 objects: 
//key "type" with value string geometry type (e.g."Polygon") and
//key "coordinates" an array
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_geometry(JsonValue value, feature_t &feature)
{
  assert(value.getTag() == JSON_OBJECT);
  std::stringstream ss;
  dump_value(value, ss);
  feature.gjson = ss.str();

  std::string str_geometry_type; //"Polygon", "MultiPolygon", "Point"
  for (JsonNode *node = value.toNode(); node != nullptr; node = node->next)
  {

    if (std::string(node->key).compare("type") == 0)
    {
      assert(node->value.getTag() == JSON_STRING);
      str_geometry_type = node->value.toString();
    }
    else if (std::string(node->key).compare("coordinates") == 0)
    {
      assert(node->value.getTag() == JSON_ARRAY);

      if (str_geometry_type.compare("Point") == 0)
      {
        /////////////////////////////////////////////////////////////////////////////////////////////////////
        //store geometry locally for points
        /////////////////////////////////////////////////////////////////////////////////////////////////////

        geometry_t geometry;
        geometry.m_type = str_geometry_type;
        
        polygon_t polygon;
        JsonValue arr_coord = node->value;
        double lon = arr_coord.toNode()->value.toNumber();;
        double lat = arr_coord.toNode()->next->value.toNumber();
        coord_t coord(lon, lat);
        polygon.m_coord.push_back(coord);
        geometry.m_polygons.push_back(polygon);
        feature.m_geometry.push_back(geometry);
      } else

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      //store geometry in parse_coordinates() for polygons
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      if (str_geometry_type.compare("Polygon") == 0)
      {
        assert(node->value.getTag() == JSON_ARRAY);
        parse_coordinates(node->value, str_geometry_type, feature);
      } else
      if (str_geometry_type.compare("MultiPolygon") == 0)
      {
        assert(node->value.getTag() == JSON_ARRAY);
        parse_coordinates(node->value, str_geometry_type, feature);
      } else
      if (str_geometry_type.compare("LineString") == 0)
      {
        assert(node->value.getTag() == JSON_ARRAY);
        parse_coordinates(node->value, str_geometry_type, feature);
      }

    }
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_coordinates
//"parse_coordinates" 
//for "Polygon"
//is an array of size 1 that contains another array and then an array of 2 numbers (lat, lon)
//for "MultiPolygon"
//is an array that contains another array of size 1, that contains another array,
//and then an array of 2 numbers (lat, lon)
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_coordinates(JsonValue value, const std::string &type, feature_t &feature)
{
  assert(value.getTag() == JSON_ARRAY);
  geometry_t geometry;
  geometry.m_type = type;
  if (type.compare("Polygon") == 0)
  {
    for (JsonNode *node = value.toNode(); node != nullptr; node = node->next)
    {
      JsonValue arr = node->value;
      assert(arr.getTag() == JSON_ARRAY);

      polygon_t polygon;
      for (JsonNode *n = arr.toNode(); n != nullptr; n = n->next)
      {
        JsonValue crd = n->value;
        assert(crd.getTag() == JSON_ARRAY);
        double lon = crd.toNode()->value.toNumber();;
        double lat = crd.toNode()->next->value.toNumber();
        coord_t coord(lon, lat);
        polygon.m_coord.push_back(coord);
      }
      geometry.m_polygons.push_back(polygon);
    }
  }

  if (type.compare("MultiPolygon") == 0)
  {
    for (JsonNode *node = value.toNode(); node != nullptr; node = node->next)
    {
      JsonValue arr = node->value;
      assert(arr.getTag() == JSON_ARRAY);
      for (JsonNode *n = arr.toNode(); n != nullptr; n = n->next) //array of size 1
      {
        JsonValue arr_crd = n->value;
        assert(arr_crd.getTag() == JSON_ARRAY);
        polygon_t polygon;
        for (JsonNode *m = arr_crd.toNode(); m != nullptr; m = m->next)
        {
          JsonValue crd = m->value;
          assert(crd.getTag() == JSON_ARRAY);
          double lon = crd.toNode()->value.toNumber();;
          double lat = crd.toNode()->next->value.toNumber();
          coord_t coord(lon, lat);
          polygon.m_coord.push_back(coord);
        }
        geometry.m_polygons.push_back(polygon);
      }
    }
  }
  //store
  feature.m_geometry.push_back(geometry);
  return 0;
}

namespace args_ns {
    bool no_inference;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::dump_value
/////////////////////////////////////////////////////////////////////////////////////////////////////
void geojson_t::dump_value(JsonValue & o, std::stringstream & ss, int indent)
{
  const int SHIFT_WIDTH = 0;
  double num;
  switch (o.getTag())
  {
  case JSON_NUMBER:
    num = o.toNumber();
    if (!args_ns::no_inference and num == std::round(num))
        ss << o.toNumber() << ".0";
    else
        ss << o.toNumber();
    break;
  case JSON_STRING:
    dump_string(o.toString(), ss);
    break;
  case JSON_ARRAY:
    if (!o.toNode())
    {
      ss << "[]";
      break;
    }
    ss << "[";
    for (auto i : o)
    {
      dump_value(i->value, ss, indent + SHIFT_WIDTH);
      ss << (i->next ? ", " : "");
    }
    ss << "]";
    break;
  case JSON_OBJECT:
    if (!o.toNode())
    {
      ss << "{}";
      break;
    }
    ss << "{";
    for (auto i : o)
    {
      dump_string(i->key, ss);
      ss << ": ";
      dump_value(i->value, ss, indent + SHIFT_WIDTH);
      ss << (i->next ? ", " : "");
    }
    ss << "}";
    break;
  case JSON_TRUE:
    ss << "true";
    break;
  case JSON_FALSE:
    ss << "false";
    break;
  case JSON_NULL:
    ss << "null";
    break;
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::dump_string
/////////////////////////////////////////////////////////////////////////////////////////////////////

void geojson_t::dump_string(const char *s, std::stringstream & ss)
{
  ss << '"';
  while (*s)
  {
    int c = *s++;
    switch (c)
    {
    case '\b':
      ss << "\\b";
      break;
    case '\f':
      ss << "\\f";
      break;
    case '\n':
      ss << "\\n";
      break;
    case '\r':
      ss << "\\r";
      break;
    case '\t':
      ss << "\\t";
      break;
    case '\\':
      ss << "\\\\";
      break;
    case '"':
      ss << "\\\"";
      break;
    default:
      ss << static_cast<char>(c);
    }
  }
  assert(*s == 0);
  ss << "\"";
}
