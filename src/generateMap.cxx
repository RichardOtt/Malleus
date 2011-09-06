/************************
This program takes the FunctionDefs.h file, parses it to extract out
the names of the functions in the file, and generates a class called
"FunctionMap" that links the Functions defined in FunctionDefs to
strings.  This is to allow them to be generated on demand without
editing the underlying code, and to prevent the errors inherent in
doing this by hand.  This could probably be handled by meta
programming.
 ************************/

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "Errors.h"
#include "Tools.h"
using namespace std;

int parseLine(string toBreak, vector<string>& toFill);
void AddToTable(string className, int functionNum);

int main(int argc, char *argv[]) {
  if(argc != 2) {
    cout << "usage:  generateMap.exe inputFile\n";
    return 1;
  }

  string inFileName = argv[1];
  //Test to make sure inFile exists
  ifstream inFile;
  inFile.open(inFileName.c_str());
  if(inFile.fail()) {
    cout << "Unable to open file " << inFileName << ", exiting\n";
    return 2;
  }

  //Print out initial part of Decider.cxx
  cout << "#include \"Decider.h\"\n\n";
  cout << "RealFunction* ";
  cout << "Decider::GenerateFunctionFromString(string className) {\n\n";

  string lineIn, line;
  vector<string> tokens;
  bool isComment = false;
  bool isString = false;
  bool extendLine = false;
  int functionCount = 0;
  while(!inFile.eof()) {
    if(extendLine) {
      string tempLine;
      getline(inFile,tempLine);
      lineIn.append(tempLine);
      extendLine = false;
    } else {
      getline(inFile,lineIn);
    }

    //First, strip out comments or commented sections.  A little
    //tricky if /* */ is being used, and need to deal with strings
    for(int i=0; i < lineIn.size(); i++) {
      if(isComment) {
	if(lineIn[i] == '*' && (i+1 < lineIn.size()) && lineIn[i+1] == '/') {
	  isComment = false;
	  i++;
	}
      } else if(isString) {
	if(lineIn[i] == '"')
	  isString = false;
	line.push_back(lineIn[i]);
      } else if(lineIn[i] == '"' && !isComment) {
	isString = true;
	line.push_back(lineIn[i]);
      } else if(lineIn[i] == '/') {
	if(i+1 < lineIn.size()) {
	  if(lineIn[i+1] == '*') {
	    isComment = true;
	    i++;
	  } else if(lineIn[i+1] == '/') {
	    break;
	  } else {
	    line.push_back(lineIn[i]);
	  }
	} else {
	  line.push_back(lineIn[i]);
	}
      } else {
	line.push_back(lineIn[i]);
      }
    }

    //If line ends in \ (after comments removed), 
    //concatenate with next line
    size_t lastChar = lineIn.find_last_not_of(" \n\r");
    if(lineIn[lastChar] == '\\' && !isComment) {
      extendLine = true;
      continue;
    }

    //Break line in to individual pieces to examine them
    int nTokens = parseLine(line, tokens);
    if(nTokens == 0)
      continue; //blank line
    // for(int j=0; j < nTokens; j++)
    //    cout << "|" << tokens[j] << "| ";
    // cout << endl;
    
    //Need to remember: C++ generally ignores whitespace, including
    //newlines, unless they're in a string or some other special cases
    //Instead, it uses the ; token to denote ends of logical blocks,
    //but it serves multiple duty (for loops, etc)
    //Come back to this, it's a thorny issue that I'll neglect for now
    //Maybe dig in to g++ source?
    
    //Assume that class defs start on fresh line
    if(tokens[0] == "class") {
      int location = Tools::SearchStringVector(tokens, "RealFunction");
      if(location != -1 && tokens[location-2] == ":") {
	//We have a winner!  Class that derives from RealFunction
	AddToTable(tokens[1], functionCount);
	functionCount++;
      }
    }

    line.clear();
  }

  if(functionCount == 0) {
    cout << "    return NULL\n\n}\n";
  } else {
    cout << "  } else {\n    return NULL;\n  }\n\n}\n";
  }
  
}

int parseLine(string toBreak, vector<string>& toFill) {
    string temp;
  toFill.clear();
  size_t position1=0, position2=0;
  
  //Skip initial whitespace
  position1 = toBreak.find_first_not_of(" \n\r",position2);
  while(position1 != string::npos) {
    position2 = toBreak.find_first_of(" \n\r:{};",position1);
    if(position1 == position2 && position1 != string::npos) {
      //Deal with one-character-long specials
      temp = toBreak.substr(position1,1);
      position2++;
    } else if(toBreak[position1] == '"') {
      //Deal with strings
      position2 = toBreak.find_first_of("\"",position1+1);
      temp = toBreak.substr(position1,position2-position1+1);
      position2++;
    } else if(position2 == string::npos) {
      temp = toBreak.substr(position1);
    } else {
      temp = toBreak.substr(position1, position2-position1);
    }

    toFill.push_back(temp);
    position1 = toBreak.find_first_not_of(" \n\r",position2);

  }

  return toFill.size();
}

void AddToTable(string className, int functionNum) {
  if(functionNum == 0) {
    cout << "  if(className == \"" << className << "\") {\n";
    cout << "    return new " << className << "();\n";
  } else {
    cout << "  } else if(className == \"" << className << "\") {\n";
    cout << "    return new " << className << "();\n";
  }
}
