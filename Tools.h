/*****
A set of functions that I use repeatedly, but that aren't function-specific
*****/

#include <string>
#include <vector>
#include <sstream>
#include "TROOT.h"
#include "TMath.h"
#include "TRandom3.h"
using namespace std;

#ifndef _TOOLS_H_
#define _TOOLS_H_

class Tools {

 public:
  static Int_t SearchStringVector(vector<string> parNameList, 
			   string targetName);
  static string ParInfoToString(string firstStr, Int_t firstInt, 
				string secondStr, Int_t secondInt);
  static Bool_t DoublesAreCloseEnough(Double_t a, Double_t b, 
				      Double_t numSize=1);
  static void VectorScramble(vector<Int_t>& toScramble);
  

};

#endif
