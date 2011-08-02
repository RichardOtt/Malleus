/*****

*****/

#include "PdfParentDummy.h"

PdfParentDummy::PdfParentDummy() {

  pdfNum = -1;
  name = "";

}

PdfParentDummy::~PdfParentDummy() {

}

void PdfParentDummy::SetParameters(Int_t nPars, Double_t *parameters) {
  //Each Pdf will keep track of which parameters out of the list
  //it needs.  Technically, the nPars parameter is not needed,
  //as that will be worked out earlier.  The exit check is
  //just for safety, shouldn't occur

  //This function selects the parameters this Pdf needs and
  //updates them to their new values

  if(pdfNum == -1)
    Errors::Exit("Pdf " +name+ " not initialized");

  Int_t numPars = parNums.size();
  for(int i=0; i < numPars; i++) {
    if(parNums[i] > nPars)
      Errors::Exit("SetParameters asked for parameter outside of range "
		   "in pdf "+name); //This shouldn't happen, debug
    pars[i] = parameters[parNums[i]];
  }


}

Bool_t PdfParentDummy::ReadConfig(ConfigFile& config, Int_t pdfNumInput,
			     vector<string> parNamesMCMC) {
  pdfNum = pdfNumInput;
  Bool_t succeeded = true;

  ostringstream ostr;
  string pdfInfo;
  ostr << "Pdf_" << pdfNum;
  pdfInfo = ostr.str();

  name = config.read<string>(pdfInfo+"_Name","");
  if(name == "") {
    Errors::AddError("Error: "+pdfInfo+"_Name not defined");
    succeeded = false;
  }
  
  Int_t sysNum = 0;
  Int_t parNumber = -1;
  string sysName;
  
  pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Sys_", sysNum);
  while(config.keyExists(pdfInfo+"_Name")) {
    sysName = config.read<string>(pdfInfo+"_Name");
    parNames.push_back(sysName);
    parNumber = SearchStringVector(parNamesMCMC, sysName);
    //Debug
    if(parNumber == -1) {
      parNumber = 0;
      Errors::AddError(sysName+" not in systematic list!  Impossible!");
    }
    parNums.push_back(parNumber);
    pars.push_back(0);
  }

  return succeeded;
}

vector<Double_t> PdfParentDummy::GetParameters() {
  return pars;
}

Double_t PdfParentDummy::CalcLikelihood() {
  return 0.5;
}


string PdfParentDummy::ParInfoToString(string firstStr, int firstInt, 
				  string secondStr, int secondInt) {

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

Int_t PdfParentDummy::SearchStringVector(vector<string> parNameList, 
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
