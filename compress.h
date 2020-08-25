// compress.h

#ifndef COMPRESS_H
#define COMPRESS_H

#include "block.h"
#include "parse.h"

#include <fstream>
#include <iostream>
#include <stdint.h>

void compress(const char *inputFile, const char *outputFile);

void compress(const char *inputFile, const char *outputFile)
{
  std::cout << "Compressing \'" << inputFile << "\' as \'" << outputFile << "\'" << std::endl;

  // Open inputFile as read-only, binary file
  std::ifstream fin(inputFile, std::ifstream::in | std::ifstream::binary);
  if (!fin.is_open())
  {
    std::cout << "Could not open file \'" << inputFile << "\'" << std::endl << std::endl;
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

#endif
