#ifndef GAME_JSON_UTIL_H_
#define GAME_JSON_UTIL_H_

#include <string>
#include <picojson.h>

inline
picojson::value BuildOutputEntry(int problem_id,
                                 int seed,
                                 const std::string& tag,
                                 const std::string& solution) {
  picojson::object result;
  result["problemId"] = picojson::value(static_cast<int64_t>(problem_id));
  result["seed"] = picojson::value(static_cast<int64_t>(seed));
  result["tag"] = picojson::value(tag);
  result["solution"] = picojson::value(solution);
  return picojson::value(result);
}

#endif  // GAME_JSON_UTIL_H_

