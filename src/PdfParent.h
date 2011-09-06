/*****

Parent class that pdf classes will inherit from.

The goal is to have it do as much as possible, and the "lower down" classes
differ as little as possible, since the way they deal with things is
nearly identical

*****/

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "ConfigFile.h"
#include "TROOT.h"
#include "Flux.h"
#include "Sys.h"
#include "Bkgd.h"
#include "Errors.h"
#include "TH1D.h"
#include "TLegend.h"
using namespace std;

#ifndef _PDFPARENT_H_
#define _PDFPARENT_H_

class PdfParent {

 protected:
  string name;
  Int_t pdfNum;
  Int_t pdfDim;
  vector<Double_t> pars;
  vector<string> parNames;
  vector<Int_t> parNums;
  vector<Double_t> parMins;
  vector<Double_t> parMaxes;
  vector<Bool_t> parHasMin;
  vector<Bool_t> parHasMax;
  Double_t logLikelihood;
  vector<Sys*> systematics;
  vector<Flux*> fluxes;
  vector<Bkgd*> backgrounds;
  Double_t totalEvents; //The number of events in the pdf, normalization
  vector<string> mcBranches; //Names of branches needed in fluxes
  vector<string> axisNames;
  vector<Int_t> axisNums;
  vector<Double_t> xAxisBins;
  vector<Double_t> yAxisBins;
  vector<Double_t> zAxisBins;
  //Int_t weightNum;


  //Internal helper functions
  string ParInfoToString(string firstStr, int firstInt, string secondStr,
			 int secondInt);
  Int_t SearchStringVector(vector<string> parNameList, string targetName);
  Bool_t ExtractFlux(ConfigFile& config, vector<string> parNamesMCMC);
  Bool_t ExtractSys(ConfigFile& config, vector<string> parNamesMCMC);
  Bool_t ExtractBkgd(ConfigFile& config, vector<string> parNamesMCMC);
  Bool_t ExtractAxes(ConfigFile& config, vector<string> parNamesMCMC);

 public:
  PdfParent();
  virtual ~PdfParent();
  //virtual PdfParent& operator = (const PdfParent&) = 0;
  virtual void SetParameters(Int_t nPars, Double_t *parameters);
  virtual Double_t CalcLogLikelihood() = 0;
  virtual void RebuildPdf() = 0;
  Bool_t ReadConfig(ConfigFile& config, Int_t pdfNumInput,
			    vector<string> parNamesMCMC);
  virtual Bool_t SetupPdf(ConfigFile& config, vector<string> parNamesMCMC) = 0;
  vector<Double_t> GetParameters();
  virtual void PrintState() = 0;
  virtual void Draw(string fileToPrintTo) = 0;

};

#endif
