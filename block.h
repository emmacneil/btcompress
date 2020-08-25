// block.h

#ifndef BLOCK_H
#define BLOCK_H

#include "transaction.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <vector>

struct Block
{
  ~Block();

  uint32_t size;
  uint32_t version;
  uint8_t hashPrevBlock[32];
  uint8_t hashMerkleRoot[32];
  uint32_t time;
  uint32_t bits;
  uint32_t nonce;
  uint64_t transactionCount; //varInt
  std::vector<Transaction*> transactions;

  static const uint32_t MAGIC_NUMBER = 0xd9b4bef9;
  static const uint32_t MAGIC_NUMBER_REVERSE = 0xf9beb4d9;
};

Block::~Block()
{
  for (auto t : transactions) delete t;    
}

void printBlockHeader(Block * block)
{
  std::cout << "Block size: " << block->size << " bytes" << std::endl;
  std::cout << "Block version: 0x" <<std::hex << block->version << std::endl;
  std::cout << "Previous block hash: 0x";
  for (int i = 0; i < 32; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)block->hashPrevBlock[i];
  std::cout << std::endl << "Merkle root: 0x";
  for (int i = 0; i < 32; i++)
  std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)block->hashMerkleRoot[i];
  std::cout << std::dec << std::endl;

  // There's probably an easier way to print the time...
  char timebuf[80];
  time_t t = (time_t)block->time;
  strftime(timebuf, 80, "%F %T", gmtime(&t));
  std::cout << "Time: 0x" << std::hex << std::setfill('0') << std::setw(8) << block->time
            << " (" << timebuf << " UTC)" << std::endl;
  std::cout << "Bits: 0x" << std::hex << std::setfill('0') << std::setw(8) << block->bits << std::endl;
  std::cout << "Nonce: 0x" << std::hex << std::setfill('0') << std::setw(8) << block->nonce << std::endl;
  std::cout << "Transaction count: " << std::dec << block->transactionCount << std::endl;
}

#endif
