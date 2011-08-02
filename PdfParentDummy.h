/*****

Dummy version of parent class that pdf classes will inherit from, for
testing MCMC

Strips out a lot of the complicated behavior, but (basically) does
what PdfParent and Pdf1D/Pdf3D do, as far as MCMC is concerned.

*****/

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "ConfigFile.h"
#include "TROOT.h"
#include "Errors.h"
using namespace std;

#ifndef _PDFPARENT_H_
#define _PDFPARENT_H_

class PdfParentDummy {

 protected:
  string name;
  Int_t pdfNum;
  vector<Double_t> pars;
  vector<string> parNames;
  vector<Int_t> parNums;

  //Internal helper functions
  string ParInfoToString(string firstStr, int firstInt, string secondStr,
			 int secondInt);
  Int_t SearchStringVector(vector<string> parNameList, string targetName);

 public:
  PdfParentDummy();
  virtual ~PdfParentDummy();
  virtual void SetParameters(Int_t nPars, Double_t *parameters);
  virtual Double_t CalcLikelihood();
  virtual Bool_t ReadConfig(ConfigFile& config, Int_t pdfNumInput,
			    vector<string> parNamesMCMC);
  vector<Double_t> GetParameters();
};

#endif
