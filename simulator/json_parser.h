#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_

#include <fstream>
#include <glog/logging.h>
#include <picojson.h>

inline
picojson::value ParseJson(const std::string& filepath) {
  std::ifstream stream(filepath);
  picojson::value result;
  stream >> result;
  CHECK(stream.good()) << picojson::get_last_error();
  return result;
}

#endif  // JSON_PARSER_H_
