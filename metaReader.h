/**********
    This converts meta files in to config files.  The config
    file can be read out as a vector<string>, one line at a time, or
    printed to a file
**********/

#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include "Errors.h"
#include "Tools.h"
using namespace std;

#ifndef _METAREADER_H_
#define _METAREADER_H_

class metaReader {

 private:
  int lineNumber;
  vector<string> configData;
  //Helper functions
  int stringToTokens(string toBreak, vector<string>& toFill);
  bool isNumber(string toCheck);

 public:
  int ReadMetaFile(string metaFilename);
  int ReadMetaFile(ifstream& inFile);
  int PrintConfigToFile(string filename);
  int PrintConfigToFile(ofstream& outFile);
  string GetConfigLine();
  //Need a "get key pair" type function
  int GetLineNumber() { return lineNumber; };
  bool SetLineNumber(int setLineNumberTo);
  int GetNumLines() { return configData.size(); };

};

#endif

