// parse.h

#ifndef PARSE_H
#define PARSE_H

#include "block.h"

#include <fstream>
#include <iostream>
#include <stdint.h>

Block *parseBlock(std::ifstream &fin);
Input *parseInput(std::ifstream &fin);
Output *parseOutput(std::ifstream &fin);
Transaction *parseTransaction(std::ifstream &fin);
void readHash(std::ifstream &fin, char *buffer, int nBytes);
uint64_t readVarInt(std::ifstream &fin);

Block *parseBlock(std::ifstream &fin)
{
  // Make sure file stream is open
  if (!fin.good())
  {
    std::cout << "Attempting to parse block from invalid filestream" << std::endl;
    return 0;
  }

  // Make sure it is pointing to a block (check magic number)
  uint32_t magicNumber;
  fin.read((char*)&magicNumber, sizeof(uint32_t));

  if (magicNumber != Block::MAGIC_NUMBER)
  {
    std::cout << "Filestream is not pointing to a valid block" << std::endl;
    if (magicNumber == Block::MAGIC_NUMBER_REVERSE)
      std::cout << "This is likely a endianness issue" << std::endl;
    return 0;
  }

  Block *block = new Block;
  fin.read((char*)&block->size, sizeof(uint32_t));
  fin.read((char*)&block->version, sizeof(uint32_t));
  //fin.read((char*)&block->hashPrevBlock, 32);
  //fin.read((char*)&block->hashMerkleRoot, 32);
  readHash(fin, (char*)&block->hashPrevBlock, 32);
  readHash(fin, (char*)&block->hashMerkleRoot, 32);
  fin.read((char*)&block->time, sizeof(uint32_t));
  fin.read((char*)&block->bits, sizeof(uint32_t));
  fin.read((char*)&block->nonce, sizeof(uint32_t));
  block->computeHash();
  
  block->transactionCount = readVarInt(fin);
  block->transactions.resize(block->transactionCount);

  for (int i = 0; i < block->transactionCount; i++)
  {
    block->transactions[i] = parseTransaction(fin);
    if (!block->transactions[i])
    {
      std::cout << "Failed to parse transaction. Aborting." << std::endl;
      return 0;
    }
  }

  return block;
}

Input *parseInput(std::ifstream &fin)
{
  // Make sure file stream is open
  if (!fin.good())
  {
    std::cout << "Attempting to parse input from invalid filestream" << std::endl;
    return 0;
  }

  Input *input = new Input;
  if (!input)
  {
    std::cout << "Could not allocate memory for input. Aborting." << std::endl;
    return 0;
  }
  readHash(fin, (char*)&input->prevTransactionHash, 32);
  fin.read((char*)&input->prevTransactionIndex, sizeof(uint32_t));
  input->scriptLength = readVarInt(fin);
  input->script = new uint8_t[input->scriptLength];
  fin.read((char*)input->script, input->scriptLength);
  fin.read((char*)&input->sequenceNumber, sizeof(uint32_t));

  return input;
}

Output *parseOutput(std::ifstream &fin)
{
  if (!fin.good())
  {
    std::cout << "Attempting to parse output from invalid filestream" << std::endl;
    return 0;
  }

  Output *output = new Output;
  fin.read((char*)&output->value, sizeof(uint64_t));
  output->scriptLength = readVarInt(fin);
  output->script = new uint8_t[output->scriptLength];
  fin.read((char*)output->script, output->scriptLength);

  return output;
}

Transaction *parseTransaction(std::ifstream &fin)
{
  // Make sure file stream is open
  if (!fin.good())
  {
    std::cout << "Attempting to parse transaction from invalid filestream" << std::endl;
    return 0;
  }

  Transaction *transaction = new Transaction;
  fin.read((char*)&transaction->version, sizeof(uint32_t));

  // Check if the flag is present.
  transaction->flag = false;
  if (fin.peek() == 0) // If the next byte is 0x00
  {
    transaction->flag = true;
    fin.ignore(2); // Skip the next two bytes
  }

  transaction->inputCount = readVarInt(fin);
  transaction->inputs.resize(transaction->inputCount);
  for (uint64_t i = 0; i < transaction->inputCount; i++)
  {
    transaction->inputs[i] = parseInput(fin);
    if (!transaction->inputs[i])
    {
      std::cout << "Failed to parse input. Aborting." << std::endl;
      return 0;
    }
  }

  transaction->outputCount = readVarInt(fin);
  transaction->outputs.resize(transaction->outputCount);
  for (uint64_t i = 0; i < transaction->outputCount; i++)
  {
    transaction->outputs[i] = parseOutput(fin);
    if (!transaction->outputs[i])
    {
      std::cout << "Failed to parse output. Aborting." << std::endl;
      return 0;
    }
  }

  // Handle witnesses
  if (transaction->flag)
  {
    for (uint64_t i = 0; i < transaction->inputCount; i++)
    {
      Input *input = transaction->inputs[i];
      input->witnessCount = readVarInt(fin);
      input->witnesses.resize(input->witnessCount);
      for (uint64_t j = 0; j < input->witnessCount; j++)
      {
        input->witnesses[j] = new Witness;
        Witness *w = input->witnesses[j];
        if (!input->witnesses[j])
        {
          std::cout << "Could not allocate memory for witness data. Aborting" << std::endl;
          return 0;
        }
        w->size = readVarInt(fin);
        w->data.resize(w->size);
        for (uint64_t k = 0; k < w->size; k++)
        {
          uint8_t tmp;
          fin.read((char*)&tmp, 1);
          w->data[k] = tmp;
        }
      }
    }
  }
  fin.read((char*)&transaction->lockTime, sizeof(uint32_t));

  return transaction;
}

void readHash(std::ifstream &fin, char *buffer, int nBytes)
{
  // fin.read(buffer, 32);
  char *ptr = buffer + nBytes;
  while (ptr > buffer)
    fin.read(--ptr, 1);
}

uint64_t readVarInt(std::ifstream &fin)
{
  uint8_t firstByte;
  fin.read((char*)&firstByte, 1);

  if (firstByte < 0xfd)
  {
    return (uint64_t)firstByte;
  }
  else if (firstByte == 0xfd)
  {
    uint16_t theRest;
    fin.read((char*)&theRest, 2);
    return (uint64_t)theRest;
  }
  else if (firstByte == 0xfe)
  {
    uint32_t theRest;
    fin.read((char*)&theRest, 4);
    return (uint64_t)theRest;
  }
  else if (firstByte == 0xff)
  {
    uint64_t theRest;
    fin.read((char*)&theRest, 8);
    return theRest;
  }
}


#endif
