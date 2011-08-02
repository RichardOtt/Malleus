
/*****
A 2-D array may work as well (for data), should compare later
*****/

#include "Flux.h"

Flux::Flux() {
  data = NULL;
  nBranches = 0;
  nEvents = 0;
  timesFlux = 1;
  fluxSize = 1;
  parNum = -1;
}


Flux::Flux(const Flux& other) {
  
  nEvents = other.nEvents;
  nBranches = other.nBranches;
  branchNames = other.branchNames;
  outputVec = other.outputVec;
  timesFlux = other.timesFlux;
  fluxSize = other.fluxSize;
  data = other.data; //Copying pointer, not data itself
  parNum = other.parNum;

//   for(int i=0; i < nBranches*nEvents; i++) {
//     data[i] = other.data[i];
//   }
}


Flux& Flux::operator = (const Flux& other) {
  if(this == &other)
    return *this;
  //In case someone tries a = a

  nEvents = other.nEvents;
  nBranches = other.nBranches;
  branchNames = other.branchNames;
  outputVec = other.outputVec;
  timesFlux = other.timesFlux;
  fluxSize = other.fluxSize;
  data = other.data; //Copying pointer, not data itself
  parNum = other.parNum;
 
//   for(int i=0; i < nBranches*nEvents; i++) {
//     data[i] = other.data[i];
//   }

  return *this;
}


Flux::~Flux() {
  if(data != NULL) {
    delete [] data;
    data = NULL;
  }
}


Bool_t Flux::LoadData(string fluxName, TTree *dataTree, 
		      vector<string> dataBranches, Double_t fluxMult,
		      Double_t meanIn /*=0*/,
		      Double_t sigmaIn /*=-1*/) {
  timesFlux = fluxMult;
  name = fluxName;
  sigma = sigmaIn;
  mean = meanIn;
  Bool_t succeeded = true;
  Int_t nTreeBranches = dataTree->GetNbranches();
  nEvents = dataTree->GetEntries();

  nBranches = dataBranches.size();
  //Now, read out branch names using some TObjArray trickery
  vector<string> treeBranches;
  TObjArray *branchNameArray;
  branchNameArray = dataTree->GetListOfBranches();
  for(int i=0; i < nTreeBranches; i++) {
    TBranch *tempBranch = (TBranch*)branchNameArray->UncheckedAt(i);
    treeBranches.push_back(tempBranch->GetName());
  }

  //Compare lists of branches, make sure all of dataBranches are in TTree
  //and get the order right (use dataBranches order)
  vector<int> branchOrder;
  for(int dataBranch = 0; dataBranch < nBranches; dataBranch++) {
    int treeBranch = 0;
    for(treeBranch = 0; treeBranch < nTreeBranches; treeBranch++) {
      if(dataBranches[dataBranch] == treeBranches[treeBranch])
	break;
    }
    if(treeBranch == nTreeBranches) {
      //Branch not found, error
      Errors::AddError("Error in flux "+name+": Branch "+ \
		       dataBranches[dataBranch]+ " not in TTree");
      succeeded = false;
    } else {
      branchOrder.push_back(treeBranch);
    }
  }

  if(branchOrder.size() != dataBranches.size())
    return false;

  Float_t *tempNums = new Float_t[branchOrder.size()];

  for(int branch=0; branch < branchOrder.size(); branch++) {
    string branchName = treeBranches[branchOrder[branch]];
    dataTree->SetBranchAddress(branchName.c_str(),&(tempNums[branch]));
  }

  //Finally ready to go, fill up data
  data = new Float_t[nEvents*nBranches];
  for(int event=0; event < nEvents; event++) {
    dataTree->GetEvent(event);

    for(int i=0; i < nBranches; i++)
      data[i + event*nBranches] = tempNums[i];
  }

  //Setup outputVec
  for(int i=0; i < nBranches; i++)
    outputVec.push_back(0);
  //Add in weight term as extra branch to return
  outputVec.push_back(0);

  delete [] tempNums;

  return succeeded;
}

vector<Double_t>& Flux::GetEvent(Int_t eventNum) {
  //If asking for event that's not present, return 
  //a vector of all zeroes
  if(eventNum >= nEvents || eventNum < 0) {
    for(int i = 0; i < nBranches; i++)
      outputVec[i] = 0;
  } else {
    for(int i = 0; i < nBranches; i++)
      outputVec[i] = data[i+eventNum*nBranches];
  }

  return outputVec;
   
}

Double_t Flux::CalcLogLikelihood() {
  if(sigma < 0)
    return 0;

  Double_t logLikelihood = 0;
  logLikelihood -= (fluxSize-mean)*(fluxSize-mean)/2/sigma/sigma;
  logLikelihood -= TMath::Log(sigma)/2;

  return logLikelihood;
  
}
