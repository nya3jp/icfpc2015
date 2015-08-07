#ifndef COMMON_H_
#define COMMON_H_

#define DISALLOW_COPY_AND_ASSIGN(name) \
  name(name&) = delete; \
  void operator=(name) = delete;

#endif  // COMMON_H_
