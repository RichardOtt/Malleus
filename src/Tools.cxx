/*****

*****/

#include "Tools.h"

Int_t Tools::SearchStringVector(vector<string> parNameList, 
				string targetName) {
  //Looks through parNameList for targetName, returns
  //the entry number in the vector if found, -1 if not

  int length = parNameList.size();
  Int_t position = -1;
  int i;

  for(i = 0; i < length; i++) {
    if(parNameList[i] == targetName)
      break;
  }

  if(i < length)
    position = i;

  return position;

}

string Tools::ParInfoToString(string firstStr, Int_t firstInt, string secondStr,
		       Int_t secondInt) {
  //Outputs a string in the format of the config file:
  //example: ParInfoToString("Pdf_",1,"_Sys_",2) returns
  //  Pdf_1_Sys_02   as a string

  string output;
  ostringstream ostr;

  ostr.width(1);
  ostr << firstStr;
  ostr << firstInt;
  ostr << secondStr;
  ostr.width(2);
  ostr.fill('0');
  ostr << secondInt;
  
  return ostr.str();

}

Bool_t Tools::DoublesAreCloseEnough(Double_t a, Double_t b, 
				    Double_t numSize /*=1*/) {
  //Need to make sure doubles have a little bit of error tolerance for
  //the last few bits of precision, for rounding errors and such
  //Also note that numSize will be used to deal with ambiguous cases, by
  //providing the size of "typical numbers" in the problem - if numbers
  //are of order 1, and you have 1e-14 and 2e-14, that's fine.  If
  //numbers are of order 1e-12, that's a problem

  if(TMath::Abs(a) < 1e-13*numSize && TMath::Abs(b) < 1e-13*numSize) {
    //In case where normal method fails, too close to zero
    //If they're both this small, call them the same
    return true;
  } else if( TMath::Abs(a+b) < 1e-13*numSize ) {
    //So, not too small of a number, but sum is tiny - too far apart
    //Want to avoid division by zero
    return false;
  } else if( TMath::Abs((a-b)/(a+b)) < 2e-13) {
    return true;
  } else {
    return false;
  }

}

void Tools::VectorScramble(vector<Int_t>& toScramble) {
  for(int i=0; i < toScramble.size()-1; i++) {
    Int_t x = i + gRandom->Rndm()*(toScramble.size()-i);
    Int_t temp = toScramble[i];
    toScramble[i] = toScramble[x];
    toScramble[x] = temp;
  }
}
