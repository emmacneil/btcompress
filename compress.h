// compress.h

#ifndef COMPRESS_H
#define COMPRESS_H

#include <iostream>

void compress(const char *inputFile, const char *outputFile);

void compress(const char *inputFile, const char *outputFile)
{
  std::cout << "Compressing \'" << inputFile << "\' as \'" << outputFile << "\'" << std::endl;
}

#endif
