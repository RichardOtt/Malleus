
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include "TROOT.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TTree.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TMath.h"
#include "TF1.h"
#include "TStyle.h"
using namespace std;

Int_t getAutoCorr(TTree *mytree, Int_t skipSteps, string outputFilename, 
		  Int_t inputMaxH = 0, Int_t skipH = 1);

int main(int argc, char *argv[]) {

  if(argc != 4 && argc != 6) {
    cout << "usage: getAutoCorr.exe inputFile skipSteps outputFile maxH skipH";
    cout << endl << "maxH and skipH are optional" << endl;
    return 1;
  }

  string inputFile, outputFile;
  Int_t skipSteps, maxH, skipH;

  inputFile = argv[1];
  outputFile = argv[3];
  skipSteps = atoi(argv[2]);

  maxH = 0;
  skipH = 1;

  if(argc == 6) {
    maxH = atoi(argv[4]);
    skipH = atoi(argv[5]);
  }

  TFile *myfile = new TFile(inputFile.c_str());
  TTree *mytree = dynamic_cast<TTree*>(myfile->Get("Tmcmc"));
  getAutoCorr(mytree, skipSteps, outputFile, maxH, skipH);

  return 0;

}


Int_t getAutoCorr(TTree *testMCMC, Int_t skipSteps, string outputFilename,
		  Int_t inputMaxH, Int_t skipH){

  //This is a root script to generate some graphs of autocorrelation
  //and the percentage of steps accepted, after burn-in
  gStyle->SetOptFit(1);

  //Process outputFilename to create appropriate CSV filename, i.e.
  //replace ending (presumably .ps) with .csv
  string csvFilename;
  string sigmaFilename;
  Int_t namepos = outputFilename.find_last_of(".");
  csvFilename = outputFilename.substr(0,namepos);
  csvFilename += ".csv";
  sigmaFilename = outputFilename.substr(0,namepos);
  sigmaFilename += ".sigma";
  
  ofstream csvFile;
  csvFile.open(csvFilename.c_str());
  cout << "Writing csv data to " << csvFilename << endl;
  csvFile << "Branch,FractionPassed,tau,tauErr,chi2\n";

  Int_t numSteps = testMCMC->GetEntries();
  cout << "Tree has " << numSteps << " entries to evaluate\n";

  //Start with % steps accepted, easier
  testMCMC->SetBranchStatus("*",0);
  testMCMC->SetBranchStatus("AccLogL",1);
  Double_t logLold, logLnew;
  testMCMC->SetBranchAddress("AccLogL",&logLnew);
  testMCMC->GetEvent(skipSteps);
  logLold = logLnew;
  Double_t num_passed = 0;
  for(int i=skipSteps+1; i < numSteps; i++) {
    testMCMC->GetEvent(i);
    if(TMath::Abs((logLold-logLnew)/(logLold+logLnew)) > 1e-10)
      num_passed++;
    logLold = logLnew;
  }
  float fractionPassed = (num_passed*1.0)/(numSteps-skipSteps);
  cout << "Percent passed is " << (num_passed*100.0)/(numSteps-skipSteps);
  cout << " % \n";

  //Get list of branch names
  vector<string> treeBranches;
  Int_t nTreeBranches = testMCMC->GetNbranches();
  TObjArray *branchNameArray;
  branchNameArray = testMCMC->GetListOfBranches();
  for(int i=0; i < nTreeBranches; i++) {
    TBranch *tempBranch = (TBranch*)branchNameArray->UncheckedAt(i);
    treeBranches.push_back(tempBranch->GetName());
  }

  //Repeat names, to make sure it worked
  cout << "Found branches:\n";
  for(int i=0; i < nTreeBranches; i++) {
    cout << treeBranches[i] << endl;
  }
  cout << endl;

  Int_t numH = (numSteps-skipSteps)/2;
  if(inputMaxH > 3)
    numH = inputMaxH;
  if(numH < 3) {
    cout << "skipSteps too large, or not enough data\n";
    return -1;
  }
  //No point in going further, run out of data
  Double_t *rhoArray, *hArray;
  Double_t xBar, denom1, denom2, numerator, valueT, value;
  rhoArray = new Double_t[numH/skipH];
  hArray = new Double_t[numH/skipH];
  testMCMC->SetBranchStatus("*",0);
  string tempfilename;
  
  //tempfilename = outputFilename + "(";

  TCanvas mycanvas("AutoCorr","AutoCorr");
  Bool_t first = true;

  //Find last branch with non-zero sigma
  Int_t last = 2;
  cout << "nTreeBranches = " << nTreeBranches << endl;
  for(int i=2; i < nTreeBranches; i++) {
    if(treeBranches[i][0] != 'A' || treeBranches[i][1] != 'c' || 
       treeBranches[i][2] != 'c') {
      continue;
    }
    testMCMC->SetBranchStatus("*",0);
    testMCMC->SetBranchStatus((treeBranches[i]).c_str(),1);
    testMCMC->SetBranchAddress((treeBranches[i]).c_str(),&value);

    xBar = 0;
    for(int step=skipSteps; step < numSteps; step++) {
      testMCMC->GetEvent(step);
      xBar += value;
    }
    xBar = xBar/(numSteps-skipSteps);

    Double_t sigmaTotal = 0;
    for(int step=skipSteps; step < numSteps; step++) {
      testMCMC->GetEvent(step);
      sigmaTotal = (value - xBar)*(value-xBar);
    }
    cout << "sigma for " << treeBranches[i] << " = " << sigmaTotal << endl;
    if(sigmaTotal < 1e-20) {
      continue;
    }
    last = i;
  }
  
  for(int i=3; i < nTreeBranches; i++) {
    if(treeBranches[i][0] != 'A' || treeBranches[i][1] != 'c' || 
       treeBranches[i][2] != 'c') {
      continue;
    }
    cout << "Working on branch " << treeBranches[i] << endl;
    testMCMC->SetBranchStatus("*",0);
    testMCMC->SetBranchStatus((treeBranches[i]).c_str(),1);
    testMCMC->SetBranchAddress((treeBranches[i]).c_str(),&value);
    Double_t *dataArray = new Double_t[numSteps];
    for(int step=0; step < numSteps; step++) {
      testMCMC->GetEvent(step);
      dataArray[step] = value;
    }

    xBar = 0;
    for(int step=skipSteps; step < numSteps; step++) {
      //testMCMC->GetEvent(step);
      value = dataArray[step];
      xBar += value;
    }
    xBar = xBar/(numSteps-skipSteps);
    cout << "xBar is " << xBar << endl;

    Double_t sigmaTotal = 0;
    for(int step=skipSteps; step < numSteps; step++) {
      //      testMCMC->GetEvent(step);
      value = dataArray[step];
      sigmaTotal = (value - xBar)*(value-xBar);
    }
    if(sigmaTotal < 1e-20) {
      cout << "Sigma zero, no variation, skipping\n";
      continue;
    }

    Int_t arrayPosition = 0;
    Bool_t foundFirst = false;
    Double_t firstMinLoc = 200;
    for(int h=0; h < numH; h += skipH, arrayPosition++) {
      
      denom1 = 0;
      denom2 = 0;
      numerator = 0;
      value = 0;
      rhoArray[arrayPosition] = 0;
      for(int t=skipSteps; t < (numSteps - h); t++) {
	value = dataArray[t];
	valueT = value;
	//cout << "valueT: " << valueT << "v. value ";
	//testMCMC->GetEvent(t+h);
	value = dataArray[t+h];
	//cout << value << endl;

	numerator += (valueT - xBar)*(value - xBar);
	denom1 += (valueT - xBar)*(valueT - xBar);
	denom2 += (value - xBar)*(value - xBar);
      }
      hArray[arrayPosition] = h;
      if(h == 200) {
	cout << "rho = " << numerator << " / sqrt(" << denom1 << ")(";
	cout << denom2 << ")\n";
      }
      rhoArray[arrayPosition] = numerator/(TMath::Sqrt(denom1)*TMath::Sqrt(denom2));
      if(!foundFirst && rhoArray[arrayPosition] < 0.3) {
	foundFirst = true;
	firstMinLoc = h;
      }
    }
    TF1 *myExpo = new TF1("myExpo","TMath::Exp(-x/[0])",0,20000);
    myExpo->SetParameter(0,firstMinLoc);    
    TGraph *mygraph = new TGraph(numH/skipH, hArray, rhoArray);
    mygraph->SetTitle((treeBranches[i]).c_str());
    mygraph->GetXaxis()->SetTitle("h, lag");
    mygraph->GetYaxis()->SetTitle("AutoCorrelation");
    mycanvas.Clear();
    mygraph->Fit(myExpo,"W");
    mygraph->Draw("LA");

    TF1 *myFitFun = mygraph->GetFunction("myExpo");
    csvFile << (treeBranches[i]).c_str() << ",";
    csvFile << scientific << fractionPassed << ",";
    csvFile << myFitFun->GetParameter(0) << "," << myFitFun->GetParError(0);
    csvFile << "," << myFitFun->GetChisquare()/myFitFun->GetNDF() << endl;

    tempfilename = outputFilename;

    if(first == true) {
      tempfilename = outputFilename + "(";
      first = false;
    }
    if(i == last)
      tempfilename = outputFilename + ")";
    
    mycanvas.SaveAs(tempfilename.c_str());
    
  }

  csvFile.close();

  ofstream sigmaFile;
  sigmaFile.open(sigmaFilename.c_str());
  sigmaFile << "Branch,sigma\n";

  for(int i=2; i < treeBranches.size(); i++) {
    if(treeBranches[i].substr(0,5) != "Sigma")
      continue;
    Double_t currentSigma = 0, totalSigma = 0;
    testMCMC->SetBranchStatus("*",0);
    testMCMC->SetBranchStatus(treeBranches[i].c_str(),1);
    testMCMC->SetBranchAddress(treeBranches[i].c_str(),&currentSigma);

    for(int step=skipSteps; step < numSteps; step++) {
      testMCMC->GetEvent(step);
      totalSigma += currentSigma;
    }
    totalSigma = totalSigma/(numSteps-skipSteps);
    sigmaFile << treeBranches[i] << "," << totalSigma << endl;

  }
  return 0;
  
}
