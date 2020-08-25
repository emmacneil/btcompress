// input.h

#ifndef INPUT_H
#define INPUT_H

#include "witness.h"

#include <iomanip>
#include <iostream>

#include <stdint.h>
#include <vector>

struct Input
{
  ~Input();
  uint8_t prevTransactionHash[32];
  uint32_t prevTransactionIndex;
  uint64_t scriptLength; // varInt
  uint8_t *script;
  uint32_t sequenceNumber;
  uint64_t witnessCount; // varInt
  std::vector<Witness*> witnesses;
};

Input::~Input()
{
  if (script)
    delete[] script;
  for (auto p : witnesses)
    delete p;
}

void printInput(Input * input)
{
  std::cout << "Previous transaction hash: 0x";
  for (int i = 0; i < 32; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)input->prevTransactionHash[i];
  std::cout << std::endl;
  std::cout << "Previous transaction index: " << std::dec << input->prevTransactionIndex << std::endl;
  std::cout << "Script length: " << input->scriptLength << std::endl;
  std::cout << "Script signature: 0x";
  for (int i = 0; i < input->scriptLength; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)input->script[i];
  std::cout << std::endl;
  std::cout << "Sequence number: " << std::hex << std::setfill('0') << std::setw(8)
            << input->sequenceNumber << std::dec << std::endl;
}

#endif
