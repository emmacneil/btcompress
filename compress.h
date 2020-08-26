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
void writeCompressedTransaction(std::ofstream &fout, Transaction *transaction);
void writeCompressedTransactionFlag(std::ofstream &fout, bool flag);
void writeCompressedTransactionInput(std::ofstream &fout, Input *input);
void writeCompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount);
void writeCompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime);
void writeCompressedTransactionOutput(std::ofstream &fout, Output *output);
void writeCompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount);
void writeCompressedTransactionVersion(std::ofstream &fout, uint32_t version);
void writeCompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses);

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
  // Write block header

  for (Transaction * transaction : block->transactions)
  {
    // Write compressed transaction
    writeCompressedTransaction(fout, transaction);
  }
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
  for (Input *input : transaction->inputs)
    writeCompressedTransactionWitnessData(fout, input->witnesses);
  writeCompressedTransactionLockTime(fout, transaction->lockTime);
}

void writeCompressedTransactionFlag(std::ofstream &fout, bool flag)
{

}

void writeCompressedTransactionInput(std::ofstream &fout, Input *input)
{
  // Compress and write previous transaction hash
  // Compress and write previous transaction index
  // Compress and write script length + script
  // Compress and write sequence number
}

void writeCompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount)
{
  // This was originally stored as a varint, which is probably good enough for us
}

void writeCompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime)
{
}

void writeCompressedTransactionOutput(std::ofstream &fout, Output *output)
{
  // Compress and write value (number of Satoshis/BTC to be sent)
  // Compress and write script length + script.
}

void writeCompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount)
{
  // This was originally stored as a varint, which is probably good enough for us
}

void writeCompressedTransactionVersion(std::ofstream &fout, uint32_t version)
{
  // Originally stored as a 32-bit integer.
  // A single byte is probably enough.
  // This could even be combined with the transaction flag.
}

void writeCompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses)
{

}

#endif
