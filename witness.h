// witness.h

#ifndef WITNESS_H
#define WITNESS_H

#include <stdint.h>
#include <vector>

struct Witness
{
  uint64_t size; // varInt
  std::vector<uint8_t> data;
};

#endif
