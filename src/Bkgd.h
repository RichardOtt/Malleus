/*****
A class that keeps track of an evaluates backgrounds

Backgrounds can be of the form of a pdf that is stored in a file,
or a function that describes the background (i.e. a function that,
when binned and integrated properly, would give the background's histogram)

Internally, the types are 'h' for Histogram (i.e. one read from a file) and
'f' for Function
*****/

#include <iostream>
#include <string>
#include <vector>
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TF1.h"
#include "TF2.h"
#include "TF3.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "Tools.h"
#include "Errors.h"
using namespace std;

#ifndef _BKGD_H_
#define _BKGD_H_

class Bkgd {

 private:
  string name;
  Int_t pdfDim;
  vector<string> parNames; //pars it is in charge of
  vector<Int_t> parNums; //where pars are in MCMC list of pars
  vector<Double_t> pars; //values it is in charge of
  TH1 *bkgdPdf;
  TF1 *bkgdFunc; //Can also be TF3 or TF2, both inherit
  Char_t type;  //May need to do resolution separately
  vector<Double_t> mean;
  vector<Double_t> sigma; //For giving likelihood contribution
  vector<Int_t> mcmcParNums;  //Values it needs
  Double_t bkgdSize;  //Overall vertical scaling, for simple bkgds
  Int_t bkgdSizeLocation; //For updating
  vector<string> axisNames;
  vector<Double_t> xAxisBins; //This is the axis of axisNames[0]
  vector<Double_t> yAxisBins; //axisNames[1]
  vector<Double_t> zAxisBins; //axisNames[2]
  vector<Int_t> axisRemap; //For functions

  //Internal Helper function
  vector<string> LookupName(vector<string> mcmcParNames, 
			    vector<string>& axisNamesIn);
  void GetBinBoundaries(Int_t axisNum);
  void CopyHistogram(TH1 *tempHisto, vector<string> axisNamesIn);
  void SetupHistoFromFunction(vector<string> axisNamesIn);
  void RebuildHisto();

 public:
  Bkgd();
  ~Bkgd();
  vector<string> Setup(string bkgdName, Double_t bkgdMean, Double_t bkgdSigma,
		       vector<string> mcmcParNames,
		       vector<string> axisNamesIn,
		       vector<string> pdfAxisNames,
		       vector<Double_t> xAxisBinsIn, 
		       vector<Double_t> yAxisBinsIn,
		       vector<Double_t> zAxisBinsIn,
		       string filename = "", string histoName = "");
  Double_t GetBinContent(Int_t xBin, Int_t yBin=-1, Int_t zBin=-1);
  void SetParameters(Int_t nPars, Double_t *parameters);
  Double_t CalcLogLikelihood();
  Bool_t AddPar(string bkgdName, Double_t bkgdMean, Double_t bkgdSigma,
		vector<string> mcmcParNames); 
  void PrintState();
  string GetName() { return name; };

};

#endif
