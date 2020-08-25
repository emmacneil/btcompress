// block.h

#ifndef BLOCK_H
#define BLOCK_H

#include "picosha2.h"
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
  uint8_t hash[32];
  uint8_t hashPrevBlock[32];
  uint8_t hashMerkleRoot[32];
  uint32_t time;
  uint32_t bits;
  uint32_t nonce;
  uint64_t transactionCount; //varInt
  std::vector<Transaction*> transactions;

  void computeHash();

  static const uint32_t MAGIC_NUMBER = 0xd9b4bef9;
  static const uint32_t MAGIC_NUMBER_REVERSE = 0xf9beb4d9;
};

Block::~Block()
{
  for (auto t : transactions) delete t;
}

/* Computes the block's hash from the block header.
 * The result is stored in the Block::hash field. */
void Block::computeHash()
{
  auto write_uint32_t = [](uint8_t *buf, uint32_t n)
  {
    buf[0] = n & 0xff;
    buf[1] = (n >>  8) & 0xff;
    buf[2] = (n >> 16) & 0xff;
    buf[3] = (n >> 24) & 0xff;
  };

  const uint8_t HEADER_SIZE = 80;
  const uint8_t HASH_SIZE = 32;

  std::vector<uint8_t> header_hex(HEADER_SIZE);
  write_uint32_t(header_hex.data(), version);
  for (int i = 0; i < 32; i++) header_hex[ 4 + i] = hashPrevBlock[31 - i];
  for (int i = 0; i < 32; i++) header_hex[36 + i] = hashMerkleRoot[31 - i];
  write_uint32_t(header_hex.data() + 68, time);
  write_uint32_t(header_hex.data() + 72, bits);
  write_uint32_t(header_hex.data() + 76, nonce);

  std::vector<uint8_t> firstHash(HASH_SIZE);
  std::vector<uint8_t> secondHash(HASH_SIZE);
  picosha2::hash256(header_hex, firstHash);
  picosha2::hash256(firstHash, secondHash);

  for (int i = 0; i < HASH_SIZE; i++)
    hash[i] = secondHash[HASH_SIZE - 1 - i];
}

void printBlockHeader(Block * block)
{
  std::cout << "Block size:          " << block->size << " bytes" << std::endl;
  std::cout << "Block version:       0x" <<std::hex << block->version << std::endl;
  std::cout << "Block hash:          0x";
  for (int i = 0; i < 32; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)block->hash[i];
  std::cout << std::dec << std::endl;
  std::cout << "Previous block hash: 0x";
  for (int i = 0; i < 32; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)block->hashPrevBlock[i];
  std::cout << std::endl;
  std::cout << "Merkle root:         0x";
  for (int i = 0; i < 32; i++)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)block->hashMerkleRoot[i];
  std::cout << std::dec << std::endl;

  // There's probably an easier way to print the time...
  char timebuf[80];
  time_t t = (time_t)block->time;
  strftime(timebuf, 80, "%F %T", gmtime(&t));
  std::cout << "Time:                0x" << std::hex << std::setfill('0') << std::setw(8) << block->time
            << " (" << timebuf << " UTC)" << std::endl;
  std::cout << "Bits:                0x" << std::hex << std::setfill('0') << std::setw(8) << block->bits << std::endl;
  std::cout << "Nonce:               0x" << std::hex << std::setfill('0') << std::setw(8) << block->nonce << std::endl;
  std::cout << "Transaction count:   " << std::dec << block->transactionCount << std::endl;
}

#endif
