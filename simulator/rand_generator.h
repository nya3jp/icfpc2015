#ifndef RAND_GENERATOR_H_
#define RAND_GENERATOR_H_

class RandGenerator {
 public:
  uint32_t seed() const { return seed_; }
  uint32_t current() const {
    return (current_ >> 16) & 0x7FFF;
  }

  void set_seed(uint32_t seed) { seed_ = seed; current_ = seed; }

  void Next() {
    current_ = 1103515245u * current_ + 12345;
  }

 private:
  uint32_t seed_;
  uint32_t current_;
};

#endif  // RAND_GENERATOR_H_
