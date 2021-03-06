// compress.h

#ifndef COMPRESS_H
#define COMPRESS_H

#include "block.h"
#include "parse.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <stdint.h>
#include <utility>

std::map<std::array<uint8_t, 32>, uint32_t> txHashes;
uint32_t nextTxHashIndex = 0;

struct BlockOrderData
{
  BlockOrderData(uint32_t t, uint32_t i, std::streampos o) : time(t), index(i), offset(o) {}
  bool operator< (const BlockOrderData &other) const { return time < other.time; }

  uint32_t time, index;
  std::streampos offset;
};

void compress(const char *inputFile, const char *outputFile);
std::vector<BlockOrderData> preprocessDatFile(std::ifstream &fin);
void writeBlockOrderData(std::ofstream &fout, std::vector<BlockOrderData> &vec);
void writeCompressedBlock(std::ofstream &fout, Block *block);
void writeCompressedBlockHeader(std::ofstream &fout, Block *block);
void writeCompressedTransaction(std::ofstream &fout, Transaction *transaction);
uint8_t writeCompressedTransactionFlag(std::ofstream &fout, Transaction *transaction);
void writeCompressedTransactionHash(std::ofstream &fout, std::array<uint8_t, 32> &hash);
void writeCompressedTransactionInput(std::ofstream &fout, Input *input, uint8_t flags);
void writeCompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount);
void writeCompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime, uint8_t flags);
void writeCompressedTransactionOutput(std::ofstream &fout, Output *output);
void writeCompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount);
void writeCompressedTransactionVersion(std::ofstream &fout, uint32_t version);
void writeCompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses);
void writeTransactionHashTable(std::ofstream &fout);
void writeVarInt(std::ofstream &fout, uint64_t val);

void compress(const char *inputFile, const char *outputFile)
{
  std::cout << "Compressing \'" << inputFile << "\' as \'" << outputFile << "\'" << std::endl;

  // Open inputFile as read-only, binary file
  std::ifstream fin(inputFile, std::ifstream::in | std::ifstream::binary);
  if (!fin.is_open())
  {
    std::cout << "Could not open file \'" << inputFile << "\'" << std::endl << std::endl;
  }

  // Open outputFile as write-only binary file
  std::ofstream fout(outputFile, std::ofstream::out | std::ofstream::binary);
  if (!fout.is_open())
  {
    std::cout << "Could not open file \'" << outputFile << "\'" << std::endl << std::endl;
  }

  // Preprocess the file
  // Build a list of (timestamp, index pairs) and sort them
  auto orderedBlocks = preprocessDatFile(fin);
  writeBlockOrderData(fout, orderedBlocks);

  int nTransactions = 0;
  int nHashes = 0;
  // While file is still open, read a block and compress it.
  for (auto blockOrderData : orderedBlocks)
  //while (fin.good())
  {
    fin.seekg(blockOrderData.offset, std::ios_base::beg);

    Block *block = parseBlock(fin);

    if (!block)
    {
      std::cout << "Could not parse block. Aborting." << std::endl;
      break;
    }

    // Do stuff with block.
    printBlockHeader(block);
    std::cout << std::endl;

    writeCompressedBlock(fout, block);

    // When we're done with the block, free up memory.
    delete block;
  }

  writeTransactionHashTable(fout);
}

std::vector<BlockOrderData> preprocessDatFile(std::ifstream &fin)
{
  std::vector<BlockOrderData> ret;
  std::streampos startPos = fin.tellg();
  fin.seekg(0, std::ios_base::end);
  std::streampos endPos = fin.tellg();
  fin.seekg(startPos, std::ios_base::beg);

  int index = 0;
  while (fin.tellg() < endPos)
  {
    BlockOrderData blockOrderData(0, index++, fin.tellg());
    uint32_t magicNumber, blockSize, timestamp;

    // Make sure we are pointing to the beginning of a block
    fin.read((char*)&magicNumber, sizeof(uint32_t));

    if (magicNumber != Block::MAGIC_NUMBER)
    {
      std::cout << "Filestream is not pointing to a valid block" << std::endl;
      if (magicNumber == Block::MAGIC_NUMBER_REVERSE)
        std::cout << "This is likely a endianness issue" << std::endl;
      return {};
    }

    // Read in the size of the block
    fin.read((char*)&blockSize, sizeof(uint32_t));

    // Skip 68 bytes, read time stamp, then skip to next block
    fin.seekg(68, std::ios_base::cur);
    fin.read((char*)&blockOrderData.time, sizeof(uint32_t));
    fin.seekg(blockSize-72, std::ios_base::cur);

    ret.push_back(blockOrderData);
  }

  // Sort the list of pairs
  std::sort(ret.begin(), ret.end());

  // Reset ifstream
  fin.seekg(startPos, std::ios_base::beg);

  return ret;
}

void writeBlockOrderData(std::ofstream &fout, std::vector<BlockOrderData> &vec)
{
  // Write the number of blocks
  uint32_t tmp = vec.size();
  fout.write((char*)&tmp, sizeof(uint32_t));

  // For each block, write the order in which they were originally encountered.
  for (auto data : vec)
  {
    fout.write((char*)&data.index, sizeof(uint32_t));
    //fout.write((char*)&data.offset, sizeof(uint32_t));
  }
}

void writeCompressedBlock(std::ofstream &fout, Block *block)
{
  std::streampos sizePos, endPos;
  uint32_t compressedBlockSize;

  // Write block header
  uint32_t magicNumber = Block::MAGIC_NUMBER;
  fout.write((char*)&magicNumber, sizeof(uint32_t));

  // We would write the size of the compressed block next, but we don't know how big it is yet.
  // Skip the next 4 bytes for now and come back later.
  sizePos = fout.tellp();
  fout.seekp(4, std::ios_base::cur);

  writeCompressedBlockHeader(fout, block);

  writeVarInt(fout, block->transactionCount);

  for (Transaction * transaction : block->transactions)
  {
    // Write compressed transaction
    writeCompressedTransaction(fout, transaction);
  }
  endPos = fout.tellp();
  compressedBlockSize = (uint32_t)(endPos - sizePos) - 4;
  // std::cout << "compressed block size = " << compressedBlockSize << std::endl;
  fout.seekp(sizePos, std::ios_base::beg);
  fout.write((char*)&compressedBlockSize, sizeof(uint32_t));
  fout.seekp(endPos, std::ios_base::beg);
}

void writeCompressedBlockHeader(std::ofstream &fout, Block *block)
{
  // The block header consists of the version number, previous block hash, merkle root, timestamp,
  // 'bits', and nonce. There is no actual compression happening here. This is just writing the
  // block header in the same format as in the original .dat file
  fout.write((char*)&block->version, sizeof(uint32_t));

  for (int i = 0; i < 32; i++)
    fout.write((char*)&block->hashPrevBlock[31-i], 1);
  for (int i = 0; i < 32; i++)
    fout.write((char*)&block->hashMerkleRoot[31-i], 1);

  fout.write((char*)&block->time, sizeof(uint32_t));
  fout.write((char*)&block->bits, sizeof(uint32_t));
  fout.write((char*)&block->nonce, sizeof(uint32_t));
}

void writeCompressedTransaction(std::ofstream &fout, Transaction *transaction)
{
  // Write compressed version and flag info.
  // This also includes information about the lock time and sequence numbers, so we do some calculations
  //writeCompressedTransactionVersion(fout, transaction->version);
  uint8_t flags = writeCompressedTransactionFlag(fout, transaction);

  writeCompressedTransactionInputCount(fout, transaction->inputCount);
  for (Input *input : transaction->inputs)
    writeCompressedTransactionInput(fout, input, flags);

  writeCompressedTransactionOutputCount(fout, transaction->outputCount);
  for (Output *output : transaction->outputs)
    writeCompressedTransactionOutput(fout, output);

  if (transaction->flag)
    for (Input *input : transaction->inputs)
      writeCompressedTransactionWitnessData(fout, input->witnesses);

  writeCompressedTransactionLockTime(fout, transaction->lockTime, flags);
}

uint8_t writeCompressedTransactionFlag(std::ofstream &fout, Transaction *transaction)
{
  // This writes not only the original flag, but also the version number and some informations
  // about the lock time and sequence numbers. The compressed flag's value is returned.
  static const uint8_t VERSION_2 = 0x1;
  static const uint8_t FLAG_PRESENT = 0x2;
  static const uint8_t LOCK_TIME_DEFAULT = 0x4;
  static const uint8_t SEQUENCE_NUMBERS_DEFAULT = 0x8;
  uint8_t flags = 0;

  if (transaction->version == 2)
    flags |= VERSION_2;
  if (transaction->flag)
    flags |= FLAG_PRESENT;
  if (transaction->lockTime == 0)
    flags |= LOCK_TIME_DEFAULT;

  uint32_t sequenceNumbers = 0xffffffff;
  for (Input *input : transaction->inputs)
    sequenceNumbers &= input->sequenceNumber;
  if (sequenceNumbers == 0xffffffff)
    flags |= SEQUENCE_NUMBERS_DEFAULT;

  fout.write((char*)&flags, sizeof(uint8_t));

  return flags;
}

void writeCompressedTransactionHash(std::ofstream &fout, std::array<uint8_t, 32> &hash)
{
  // If hash is in the table of transactions hashes, fetch its index.
  // Otherwise, add it to the table and assign it an index.
  uint32_t index;
  if (txHashes.count(hash))
    index = txHashes[hash];
  else
    index = txHashes[hash] = nextTxHashIndex++;

  //char *start = (char*)hash.data();
  //char *ptr = start + 32;
  //while (ptr > start)
  //  fout.write(--ptr, sizeof(uint8_t));

  fout.write((char*)&index, sizeof(uint32_t));
}

void writeCompressedTransactionInput(std::ofstream &fout, Input *input, uint8_t flags)
{
  static const uint8_t SEQUENCE_NUMBERS_DEFAULT = 0x8;

  // Compress and write previous transaction hash
  writeCompressedTransactionHash(fout, input->prevTransactionHash);

  // Compress and write previous transaction index
  // This was originally a 32-bit integer. Now we use a varint
  writeVarInt(fout, input->prevTransactionIndex);
  //fout.write((char*)&input->prevTransactionIndex, sizeof(uint32_t));

  // Compress and write script length + script
  writeVarInt(fout, input->scriptLength);
  for (int i = 0; i < input->scriptLength; i++)
    fout.write((char*)&input->script[i], 1);

  // Compress and write sequence number
  // Because the difference between the sequence numbers and 0xffffffff is usually small,
  // we can store the difference instead.
  if (!(flags & SEQUENCE_NUMBERS_DEFAULT))
  {
    uint64_t tmp = input->sequenceNumber ^ 0xffffffff;
    writeVarInt(fout, tmp);
  }
}

void writeCompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount)
{
  // This was originally stored as a varint, which is probably good enough for us
  writeVarInt(fout, inputCount);
}

void writeCompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime, uint8_t flags)
{
  static const uint8_t LOCK_TIME_DEFAULT = 0x4;
  if (!(flags & LOCK_TIME_DEFAULT))
    fout.write((char*)&lockTime, sizeof(uint32_t));
}

void writeCompressedTransactionOutput(std::ofstream &fout, Output *output)
{
  // Compress and write value (number of Satoshis/BTC to be sent)
  //fout.write((char*)&output->value, sizeof(uint64_t));
  writeVarInt(fout, output->value);

  // Compress and write script length + script.
  writeVarInt(fout, output->scriptLength);
  for (int i = 0; i < output->scriptLength; i++)
    fout.write((char*)&output->script[i], 1);
}

void writeCompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount)
{
  // This was originally stored as a varint, which is probably good enough for us
  writeVarInt(fout, outputCount);
}

void writeCompressedTransactionVersion(std::ofstream &fout, uint32_t version)
{
  // Originally stored as a 32-bit integer.
  // A single byte is probably enough.
  uint8_t cVersion = (uint8_t)version;
  fout.write((char*)&cVersion, sizeof(uint8_t));

  // This could even be combined with the transaction flag.
}

void writeCompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses)
{
  writeVarInt(fout, witnesses.size());
  for (Witness *w : witnesses)
  {
    writeVarInt(fout, w->size);
    for (uint8_t byte : w->data)
      fout.write((char*)&byte, 1);
  }
}

void writeTransactionHashTable(std::ofstream &fout)
{
  // Put hashes into a vector of index-hash pairs
  std::vector<std::pair<uint32_t, std::array<uint8_t, 32>>> vec;
  for (auto pr : txHashes)
    vec.push_back(std::make_pair(pr.second, pr.first));
  std::sort(vec.begin(), vec.end());

  // Write number of hashes
  uint32_t nTxHashes = vec.size();
  fout.write((char*)&nTxHashes, sizeof(uint32_t));

  // Write hashes
  for (auto pr : vec)
    for (int i = 31; i >= 0; i--)
      fout.write((char*)(pr.second.data() + i), sizeof(uint8_t));
}

void writeVarInt(std::ofstream &fout, uint64_t val)
{
  if (val < 0xfd)
  {
    uint8_t tmp = val & 0xff;
    fout.write((char*)&tmp, sizeof(uint8_t));
  }
  else if (val < 0x10000)
  {
    uint8_t byte = 0xfd;
    uint16_t tmp = val & 0xffff;
    fout.write((char*)&byte, sizeof(uint8_t));
    fout.write((char*)&tmp, sizeof(uint16_t));
  }
  else if (val < 0x100000000)
  {
    uint8_t byte = 0xfe;
    uint32_t tmp = val & 0xffffffff;
    fout.write((char*)&byte, sizeof(uint8_t));
    fout.write((char*)&tmp, sizeof(uint32_t));
  }
  else
  {
    uint8_t byte = 0xff;
    fout.write((char*)&byte, sizeof(uint8_t));
    fout.write((char*)&val, sizeof(uint64_t));
  }
}

#endif
