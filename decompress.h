// decompress.h

#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include "block.h"
#include "parse.h"
#include "compress.h" // writeVarInt

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <utility>

struct CompressedBlockOrderData
{
  bool operator< (const CompressedBlockOrderData &other) const { return index < other.index; }
  uint32_t compressedIndex, index;
  std::streampos offset;
};

void decompress(const char *inputFile, const char *outputFile);
std::vector<CompressedBlockOrderData> preprocessCompressedFile(std::ifstream &fin);
void writeDecompressedBlock(std::ofstream &fout, Block *block);
void writeDecompressedBlockHeader(std::ofstream &fout, Block *block);
void writeDecompressedTransaction(std::ofstream &fout, Transaction *transaction);
void writeDecompressedTransactionFlag(std::ofstream &fout, bool flag);
void writeDecompressedTransactionInput(std::ofstream &fout, Input *input);
void writeDecompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount);
void writeDecompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime);
void writeDecompressedTransactionOutput(std::ofstream &fout, Output *output);
void writeDecompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount);
void writeDecompressedTransactionVersion(std::ofstream &fout, uint32_t version);
void writeDecompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses);

void decompress(const char *inputFile, const char *outputFile)
{
  std::cout << "Decompressing \'" << inputFile << "\' as \'" << outputFile << "\'" << std::endl;

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
  auto orderedBlocks = preprocessCompressedFile(fin);

  // While file is still open, read a block and compress it.
  for (auto blockOrderData : orderedBlocks)
  {
    fin.seekg(blockOrderData.offset, std::ios_base::beg);

    Block *block = parseCompressedBlock(fin);

    if (!block)
    {
      std::cout << "Could not parse block. Aborting." << std::endl;
      break;
    }

    // Do stuff with block.
    printBlockHeader(block);
    std::cout << std::endl;

    writeDecompressedBlock(fout, block);

    // When we're done with the block, free up memory.
    delete block;
  }
}

std::vector<CompressedBlockOrderData> preprocessCompressedFile(std::ifstream &fin)
{
  uint32_t size;
  fin.read((char*)&size, sizeof(uint32_t));
  std::vector<CompressedBlockOrderData> ret(size);

  uint32_t compressedIndex = 0;
  for (auto &t : ret)
  {
    uint32_t index;
    std::streampos offset;

    fin.read((char*)&index, sizeof(uint32_t));

    t.index = index;
  }

  std::streampos firstBlockPos = fin.tellg();
  fin.seekg(0, std::ios_base::end);
  std::streampos endPos = fin.tellg();
  fin.seekg(firstBlockPos, std::ios_base::beg);

  uint32_t i = 0;
  while (fin.tellg() < endPos)
  {
    uint32_t magicNumber, blockSize;
    ret[i++].offset = fin.tellg();

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

    // Skip that many bytes to next block
    fin.seekg(blockSize, std::ios_base::cur);
  }

  std::sort(ret.begin(), ret.end());

  return ret;
}

void writeDecompressedBlock(std::ofstream &fout, Block *block)
{
  std::streampos sizePos, endPos;
  uint32_t decompressedBlockSize;

  // Write block header
  uint32_t magicNumber = Block::MAGIC_NUMBER;
  fout.write((char*)&magicNumber, sizeof(uint32_t));

  // We would write the size of the decompressed block next, but we don't know how big it is yet.
  // Skip the next 4 bytes for now and come back later.
  sizePos = fout.tellp();
  fout.seekp(4, std::ios_base::cur);

  writeDecompressedBlockHeader(fout, block);

  writeVarInt(fout, block->transactionCount);

  for (Transaction * transaction : block->transactions)
  {
    // Write decompressed transaction
    writeDecompressedTransaction(fout, transaction);
  }
  endPos = fout.tellp();
  decompressedBlockSize = (uint32_t)(endPos - sizePos) - 4;
  fout.seekp(sizePos, std::ios_base::beg);
  fout.write((char*)&decompressedBlockSize, sizeof(uint32_t));
  fout.seekp(endPos, std::ios_base::beg);
}

void writeDecompressedBlockHeader(std::ofstream &fout, Block *block)
{
  // The block header consists of the version number, previous block hash,
  // merkle root, timestamp, 'bits', and nonce.
  fout.write((char*)&block->version, sizeof(uint32_t));

  for (int i = 0; i < 32; i++)
    fout.write((char*)&block->hashPrevBlock[31-i], 1);
  for (int i = 0; i < 32; i++)
    fout.write((char*)&block->hashMerkleRoot[31-i], 1);

  fout.write((char*)&block->time, sizeof(uint32_t));
  fout.write((char*)&block->bits, sizeof(uint32_t));
  fout.write((char*)&block->nonce, sizeof(uint32_t));
}

void writeDecompressedTransaction(std::ofstream &fout, Transaction *transaction)
{
  writeDecompressedTransactionVersion(fout, transaction->version);
  writeDecompressedTransactionFlag(fout, transaction->flag);

  writeDecompressedTransactionInputCount(fout, transaction->inputCount);
  for (Input *input : transaction->inputs)
    writeDecompressedTransactionInput(fout, input);

  writeDecompressedTransactionOutputCount(fout, transaction->outputCount);
  for (Output *output : transaction->outputs)
    writeDecompressedTransactionOutput(fout, output);

  if (transaction->flag)
    for (Input *input : transaction->inputs)
      writeDecompressedTransactionWitnessData(fout, input->witnesses);

  writeDecompressedTransactionLockTime(fout, transaction->lockTime);
}

void writeDecompressedTransactionFlag(std::ofstream &fout, bool flag)
{
  if (flag)
  {
    uint8_t tmp = 0x00;
    fout.write((char*)&tmp, sizeof(uint8_t));
    tmp = 0x01;
    fout.write((char*)&tmp, sizeof(uint8_t));
  }
}

void writeDecompressedTransactionInput(std::ofstream &fout, Input *input)
{
  // Decompress and write previous transaction hash
  for (int i = 0; i < 32; i++)
    fout.write((char*)&input->prevTransactionHash[31-i], 1);

  // Decompress and write previous transaction index
  fout.write((char*)&input->prevTransactionIndex, sizeof(uint32_t));

  // Decompress and write script length + script
  writeVarInt(fout, input->scriptLength);
  for (int i = 0; i < input->scriptLength; i++)
    fout.write((char*)&input->script[i], 1);

  // Decompress and write sequence number
  fout.write((char*)&input->sequenceNumber, sizeof(uint32_t));
}

void writeDecompressedTransactionInputCount(std::ofstream &fout, uint64_t inputCount)
{
  writeVarInt(fout, inputCount);
}

void writeDecompressedTransactionLockTime(std::ofstream &fout, uint32_t lockTime)
{
  fout.write((char*)&lockTime, sizeof(uint32_t));
}

void writeDecompressedTransactionOutput(std::ofstream &fout, Output *output)
{
  // Decompress and write value (number of Satoshis/BTC to be sent)
  fout.write((char*)&output->value, sizeof(uint64_t));

  // Decompress and write script length + script.
  writeVarInt(fout, output->scriptLength);
  for (int i = 0; i < output->scriptLength; i++)
    fout.write((char*)&output->script[i], 1);
}

void writeDecompressedTransactionOutputCount(std::ofstream &fout, uint64_t outputCount)
{
  writeVarInt(fout, outputCount);
}

void writeDecompressedTransactionVersion(std::ofstream &fout, uint32_t version)
{
  fout.write((char*)&version, sizeof(uint32_t));
}

void writeDecompressedTransactionWitnessData(std::ofstream &fout, std::vector<Witness*> &witnesses)
{
  writeVarInt(fout, witnesses.size());
  for (Witness *w : witnesses)
  {
    writeVarInt(fout, w->size);
    for (uint8_t byte : w->data)
      fout.write((char*)&byte, 1);
  }
}

#endif
