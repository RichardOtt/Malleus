/*****
This class handles fluxes, i.e. things that enter in to the MCMC
as a large list of events.  Those that enter as a histogram are
dealt with in the Bkgd class

Each flux should only deal with one set of data.  It should come in
the form of a TTree, and for a given pdf, all fluxes should have
the same list of variables.  Ideally, in later versions of this code
it will be smart enough to amalgamate dissimilar ones together.  For
now, these must be listed in the config file.  Later, I should make
it able to automatically extract them from the TTree

Note that using the = operator on this class results in only a pointer
to the data being copied, so the different copies will share.  This
should be ok, as long as it's done AFTER the setup steps, so that the
data is unchanging.  Actually, that's a huge problem with the destructor.

*****/

#include <iostream>
#include <string>
#include <vector>
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TObjArray.h"
#include "TBranch.h"
#include "TMath.h"
#include "Errors.h"
using namespace std;

#ifndef _FLUX_H_
#define _FLUX_H_

class Flux {
 protected:
  string name;
  vector<string> branchNames;
  Float_t *data;
  Int_t nBranches;
  Int_t nEvents;
  vector<Double_t> outputVec;
  Double_t timesFlux;  //How many times more MC than data
  Double_t sigma;  //For constrained fluxes
  Double_t mean;  //For constrained fluxes
  Double_t fluxSize; //in front multiplier; events = nEvents*fluxSize/timesFlux
  Int_t parNum; //Where this flux's fluxSize is in the MCMC par list
  Int_t fluxType; //Use this to distinguish ES v. CC v. NC

 public:
  Flux();
  Flux(const Flux& other);
  ~Flux();
  Flux& operator = (const Flux& other);

  Bool_t LoadData(string fluxName, TTree *dataTree, 
		  vector<string> dataBranches, Double_t fluxMult,
		  Double_t meanIn = 0, Double_t sigmaIn=-1);
  vector<Double_t>& GetEvent(Int_t eventNum);
  Int_t GetNEvents() { return nEvents; };
  vector<string> GetBranchNames() { return branchNames; };
  Double_t GetTimesFlux() { return timesFlux; };
  void SetFluxSize(Double_t inputFlux) { fluxSize = inputFlux; };
  Double_t GetFluxSize() { return fluxSize; };
  Double_t CalcLogLikelihood();
  void SetFluxType(Int_t fluxTypeIn) { fluxType = fluxTypeIn; };
  Int_t GetFluxType() { return fluxType; };
  string GetName() { return name; };

};

#endif
