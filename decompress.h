// decompress.h

#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <iostream>

void decompress(const char *inputFile, const char *outputFile);

void decompress(const char *inputFile, const char *outputFile)
{
  std::cout << "Decompressing \'" << inputFile << "\' as \'" << outputFile << "\'" << std::endl;
}

#endif
