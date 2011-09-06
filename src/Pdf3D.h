/*****
3-D pdf class.  This does the actual heavy lifting of the process.

Note that it's not meant to be passed in to a function as a parameter,
at the moment it doesn't have a proper copy constructor
*****/

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "TROOT.h"
#include "TH3D.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TStyle.h"
#include "ConfigFile.h"
#include "PdfParent.h"
#include "Errors.h"
#include "Flux.h"
#include "Sys.h"

#ifndef _PDF3D_H_
#define _PDF3D_H_

class Pdf3D : public PdfParent {

 protected:
  TH3D *masterPdf;

  Int_t nDataPoints; //Note that dataArray will be 3x this long since 3D
  Float_t *dataArray; //For the actual, measured data
  //parameters and their info are stored in PdfParent

  //Internal helper functions
  Bool_t SetupAxis(ConfigFile& config, Int_t axisNumber);

 public:
  Pdf3D();
  virtual ~Pdf3D();
  virtual Pdf3D& operator = (const Pdf3D&);
  Bool_t SetupPdf(ConfigFile& config, vector<string> parNamesMCMC);
  virtual Double_t CalcLogLikelihood();
  void RebuildPdf();
  void NormalizePdfByBinWidth();
  Double_t GetValue(Double_t x, Double_t y, Double_t z);
  virtual void PrintState();
  void Draw(string fileToPrintTo);
  TH1D *DrawFlux(Int_t fluxNumber, Int_t axisToDraw, Double_t *fluxCount = NULL);

  

};

#endif
