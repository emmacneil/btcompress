// compress.h

#ifndef COMPRESS_H
#define COMPRESS_H

#include "block.h"
#include "parse.h"

#include <fstream>
#include <iostream>
#include <stdint.h>

void compress(const char *inputFile, const char *outputFile);
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

  // While file is still open, read a block and compress it.
  while (fin.good())
  {
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
