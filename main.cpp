// main.cpp

#include "compress.h"
#include "decompress.h"

#include <iostream>
#include <string.h>

void printUsage();

int main(int argc, char *argv[])
{
  bool compressMode = true;
  // Parse arguments
  // It would be nice to use getopt() here, but that is Unix-only.
  // For now, we will require arguments to be specified in a particular way
  if (argc != 4)
  {
    printUsage();
    return 0;
  }

  if (strcmp(argv[1], "-d") == 0)
    compressMode = false;
  else if (strcmp(argv[1], "-c") != 0)
  {
    printUsage();
    return 0;
  }

  if (compressMode)
    compress(argv[2], argv[3]);
  else
    decompress(argv[2], argv[3]);

  return 0;
}

void printUsage()
{
  std::cout << "Program usage:" << std::endl;
  std::cout << "To compress," << std::endl;
  std::cout << "\tbtcompress -c input_file output_file" << std::endl;
  std::cout << "To decompress," << std::endl;
  std::cout << "\tbtcompress -d input_file output_file" << std::endl;
}
