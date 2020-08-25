// transaction.h

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "input.h"
#include "output.h"

#include <iostream>
#include <stdint.h>
#include <vector>

struct Transaction
{
  ~Transaction();

  uint32_t version;
  bool flag;
  uint64_t inputCount; // varInt
  uint64_t outputCount; // varInt
  uint32_t lockTime;
  std::vector<Input*> inputs;
  std::vector<Output*> outputs;
  //std::vector<Witness*> witnesses;
};

Transaction::~Transaction()
{
  for (auto p : inputs) delete p;
  for (auto p : outputs) delete p;
  //for (auto p : witnesses) delete p;
}

void printTransaction(Transaction * transaction)
{
  std::cout << "Version: " << transaction->version << std::endl;
  std::cout << "Flag: " << transaction->flag << std::endl;
  std::cout << "Input count: " << transaction->inputCount << std::endl;
  for (int i = 0; i < transaction->inputCount; i++)
  {
    printInput(transaction->inputs[i]);
  }

  std::cout << "Output count: " << transaction->outputCount << std::endl;
  for (int i = 0; i < transaction->outputCount; i++)
  {
    printOutput(transaction->outputs[i]);
  }

  std::cout << "Witnesses: " << "---" << std::endl;
  std::cout << "Lock time: " << std::hex << transaction->lockTime << std::dec << std::endl;
  std::cout << std::endl;
}

#endif
