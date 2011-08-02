/*****
1-D pdf class.  This does the actual heavy lifting of the process.

Note that it's not meant to be passed in to a function as a parameter,
at the moment it doesn't have a proper copy constructor
*****/

#include <iostream>
#include <string>
#include <vector>
#include "TROOT.h"
#include "TH1D.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TPaveStats.h"
#include "TLegend.h"
#include "TStyle.h"
#include "ConfigFile.h"
#include "PdfParent.h"
#include "Errors.h"
#include "Flux.h"
#include "Sys.h"

#ifndef _PDF1D_H_
#define _PDF1D_H_

class Pdf1D : public PdfParent {

 protected:
  TH1D *masterPdf;

  Int_t xAxisNum;
  Int_t nDataPoints;
  Float_t *dataArray; //For the actual, measured data
  
  //parameters and their info are stored in PdfParent

 public:
  Pdf1D();
  virtual ~Pdf1D();
  virtual Pdf1D& operator = (const Pdf1D&);
  Bool_t SetupPdf(ConfigFile& config, vector<string> parNamesMCMC);
  virtual Double_t CalcLogLikelihood();
  void RebuildPdf();
  void NormalizePdfByBinWidth();
  Double_t GetValue(Double_t point);
  virtual void PrintState();
  void Draw(string fileToPrintTo);
  TH1D* DrawFlux(Int_t fluxNumber);
  

};

#endif
