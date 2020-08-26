// compress.h

#ifndef COMPRESS_H
#define COMPRESS_H

#include "block.h"
#include "parse.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <utility>

void compress(const char *inputFile, const char *outputFile);
std::vector<std::pair<uint32_t, std::streampos>> preprocessDatFile(std::ifstream &fin);
void writeCompressedBlock(std::ofstream &fout, Block *block);
void writeCompressedBlockHeader(std::ofstream &fout, Block *block);
void writeCompressedTransaction(std::ofstream &fout, Transaction *transaction);
void writeCompressedTransactionFlag(std::ofstream &fout, bool flag);
void writeCompressedTransactionInput(std::ofstream &fout, Input *input);
void writeCompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount);
void writeCompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime);
void writeCompressedTransactionOutput(std::ofstream &fout, Output *output);
void writeCompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount);
void writeCompressedTransactionVersion(std::ofstream &fout, uint32_t version);
void writeCompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses);
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

  // While file is still open, read a block and compress it.
  for (auto pr : orderedBlocks)
  //while (fin.good())
  {
    fin.seekg(pr.second, std::ios_base::beg);

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
}

std::vector<std::pair<uint32_t, std::streampos>> preprocessDatFile(std::ifstream &fin)
{
  std::vector<std::pair<uint32_t, std::streampos>> ret;
  std::streampos startPos = fin.tellg();
  fin.seekg(0, std::ios_base::end);
  std::streampos endPos = fin.tellg();
  fin.seekg(startPos, std::ios_base::beg);

  while (fin.tellg() < endPos)
  {
    std::pair<uint32_t, std::streampos> pr(0, fin.tellg());
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
    fin.read((char*)&pr.first, sizeof(uint32_t));
    fin.seekg(blockSize-72, std::ios_base::cur);

    ret.push_back(pr);
  }

  // Sort the list of pairs
  std::sort(ret.begin(), ret.end());

  // Test output
  //for (auto p : ret)
  //  std::cout << p.first << "   " << p.second << std::endl;

  // Reset ifstream
  fin.seekg(startPos, std::ios_base::beg);

  return ret;
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
  // The block header consists of the version number, previous block hash, merkle root, timestamp, 'bits', and nonce.
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
  writeCompressedTransactionVersion(fout, transaction->version);
  writeCompressedTransactionFlag(fout, transaction->flag);

  writeCompressedTransactionInputCount(fout, transaction->inputCount);
  for (Input *input : transaction->inputs)
    writeCompressedTransactionInput(fout, input);

  writeCompressedTransactionOutputCount(fout, transaction->outputCount);
  for (Output *output : transaction->outputs)
    writeCompressedTransactionOutput(fout, output);

  if (transaction->flag)
    for (Input *input : transaction->inputs)
      writeCompressedTransactionWitnessData(fout, input->witnesses);

  writeCompressedTransactionLockTime(fout, transaction->lockTime);
}

void writeCompressedTransactionFlag(std::ofstream &fout, bool flag)
{
  if (flag)
  {
    uint8_t tmp = 0x00;
    fout.write((char*)&tmp, sizeof(uint8_t));
    tmp = 0x01;
    fout.write((char*)&tmp, sizeof(uint8_t));
  }
}

void writeCompressedTransactionInput(std::ofstream &fout, Input *input)
{
  // Compress and write previous transaction hash
  for (int i = 0; i < 32; i++)
    fout.write((char*)&input->prevTransactionHash[31-i], 1);

  // Compress and write previous transaction index
  fout.write((char*)&input->prevTransactionIndex, sizeof(uint32_t));

  // Compress and write script length + script
  writeVarInt(fout, input->scriptLength);
  for (int i = 0; i < input->scriptLength; i++)
    fout.write((char*)&input->script[i], 1);

  // Compress and write sequence number
  fout.write((char*)&input->sequenceNumber, sizeof(uint32_t));
}

void writeCompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount)
{
  // This was originally stored as a varint, which is probably good enough for us
  writeVarInt(fout, inputCount);
}

void writeCompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime)
{
  fout.write((char*)&lockTime, sizeof(uint32_t));
}

void writeCompressedTransactionOutput(std::ofstream &fout, Output *output)
{
  // Compress and write value (number of Satoshis/BTC to be sent)
  fout.write((char*)&output->value, sizeof(uint64_t));

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
  // This could even be combined with the transaction flag.
  fout.write((char*)&version, sizeof(uint32_t));
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
