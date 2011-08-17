#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include "Errors.h"
#include "Tools.h"
#include "metaReader.h"
using namespace std;

int main(int argc, char *argv[]) {

  
  if(argc != 3) {
    cout << "usage:   metaConfig.exe inputFile outputFile\n";
    return 1;
  }

  string inFileName = argv[1];
  string outFileName = argv[2];

  //Test to make sure inFile exists
  ifstream inFile;
  inFile.open(inFileName.c_str());
  if(inFile.fail()) {
    cout << "Unable to open file " << inFileName << ", exiting\n";
    return 2;
  }

  //Test to make sure can write to outFile
  ofstream outFile;
  outFile.open(outFileName.c_str());
  if(outFile.fail()) {
    cout << "Unable to open file " << outFileName << ", exiting\n";
    return 2;
  }

  metaReader metaConfig;

  int results = metaConfig.ReadMetaFile(inFile);
  if(results != 0 || Errors::GetNErrors() != 0) {
    Errors::AddError("Fatal Error: Parsing meta file "+inFileName+" failed");
    Errors::Exit();
  }
  
  results = metaConfig.PrintConfigToFile(outFile);
  if(results != 0 || Errors::GetNErrors() != 0) {
    Errors::AddError("Fatal Error: Writing to "+outFileName+" failed");
    Errors::Exit();
  }

  return 0;
}
