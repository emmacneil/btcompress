// output.h

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>

struct Output
{
  ~Output() { if (script) delete[] script; }

  uint64_t value;
  uint64_t scriptLength; // varInt
  uint8_t *script;
};

void printOutput(Output * output)
{
  std::cout << "Value: " << output->value << std::endl;
  std::cout << "Script length: " << output->scriptLength << std::endl;
  std::cout << "Script pubkey: 0x";
  for (int i = 0; i < output->scriptLength; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)output->script[i];
  std::cout << std::dec << std::endl;
}

#endif
