#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TF1.h"
#include "TMatrixD.h"
#include "TMath.h"
#include "TObjString.h"
#include "TObjArray.h"
#include "TEventList.h"
#include "TDirectory.h"
using namespace std;

map<string,double> autoFit(TTree *testMCMC, Int_t skipSteps, string outFile, 
			   Bool_t biVariate = true, 
			   Bool_t unbinnedFit = false, Int_t freq = 1,
			   Bool_t unbinnedPredictor = true) {

  map<string,double> resultMap;
  if(freq < 1)
    freq = 1;

  //Start with % steps accepted
  TCanvas mycanvas("data","data");
  gStyle->SetOptFit(1111);
  Int_t numSteps = testMCMC->GetEntries();
  testMCMC->SetBranchStatus("*",0);
  testMCMC->SetBranchStatus("AccLogL",1);
  Double_t logLold, logLnew;
  testMCMC->SetBranchAddress("AccLogL",&logLnew);
  testMCMC->GetEvent(skipSteps);
  logLold = logLnew;
  Double_t num_passed = 0;
  for(int i=skipSteps; i < numSteps; i++) {
    testMCMC->GetEvent(i);
    if(TMath::Abs((logLold-logLnew)/(logLold+logLnew)) > 1e-10)
      num_passed++;
    logLold = logLnew;
  }
  cout << "Percent passed is " << (num_passed*100.0)/(numSteps-skipSteps);
  cout << " % \n";

  Double_t value=0;

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

  Bool_t first = true;
  Double_t xBar;

  //Find last branch with non-zero sigma
  Int_t last = 3;
  for(int i=3; i < nTreeBranches; i++) {
    if(treeBranches[i].substr(0,3) != "Acc")
      continue;
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
    if(sigmaTotal < 1e-20) {
      continue;
    }
    last = i;
  }

  ostringstream str;
  string stepCut;
  str << "Step>=" << skipSteps;
  stepCut = str.str();
  testMCMC->SetBranchStatus("Step",1);
  testMCMC->Draw(">>mylist",stepCut.c_str());
  TEventList *mylist = (TEventList*)gDirectory->Get("mylist");
  //testMCMC->SetEventList(mylist);
  Int_t nListEvents = mylist->GetN();
  stepCut.clear();
  str.str(stepCut);

  for(int i=3; i <= last; i++) {
    if(treeBranches[i].substr(0,3) != "Acc")
      continue;
    cout << "Working on branch " << treeBranches[i] << endl;
    testMCMC->SetBranchStatus("*",0);
    testMCMC->SetBranchStatus("Step",1);
    testMCMC->SetBranchStatus((treeBranches[i]).c_str(),1);
    testMCMC->SetBranchAddress((treeBranches[i]).c_str(),&value);


    Double_t *dataArray = new Double_t[nListEvents];
    for(int step=0; step < nListEvents; step++) {
      testMCMC->GetEvent(mylist->GetEntry(step));
      dataArray[step] = value;
    }

    xBar = 0;
    Double_t sigmaTotal = 0;
    Double_t currMin = dataArray[0];
    Double_t currMax = dataArray[0];
    cout << "Before loop: " << xBar << " +- " << sigmaTotal << endl;
    cout << "First few events: " << dataArray[0] << " " << dataArray[1];
    cout << " " << dataArray[2] << endl;
    for(int step=0; step < nListEvents; step++) {
      xBar += dataArray[step];
      sigmaTotal += dataArray[step]*dataArray[step];
      if(dataArray[step] < currMin)
	currMin = dataArray[step];
      if(dataArray[step] > currMax)
	currMax = dataArray[step];
    }
    xBar = xBar/(nListEvents);
    cout << "xBar is " << xBar << endl;
    sigmaTotal = sigmaTotal/(nListEvents) - xBar*xBar;
    if(sigmaTotal < 1e-20) {
      cout << "Sigma zero, no variation, skipping\n";
      resultMap.insert(pair<string,double>(treeBranches[i]+"Mean",xBar));
      continue;
    }
    Double_t mean = xBar;
    Double_t widthLow, widthHigh, width;
    width = TMath::Sqrt(sigmaTotal);

    cout << "Sigma is " << width << endl;

//     for(int step=skipSteps; step < numSteps; step++) {
//       value = dataArray[step];
//       sigmaTotal += (value - xBar)*(value - xBar);
//     }

//     Double_t currMin = testMCMC->GetMinimum((treeBranches[i]).c_str());
//     Double_t currMax = testMCMC->GetMaximum((treeBranches[i]).c_str());
    Double_t range = currMax - currMin;
    currMin = currMin - range*0.1;
    currMax = currMax + range*0.1;


    TF1 *fitGaus = new TF1("fitGaus","[3]*TMath::Exp(-(x-[0])**2/(2*[1]**2)*(x<=[0]) -(x-[0])**2/(2*([1]+[2])**2)*(x>[0]))/(TMath::Sqrt(TMath::Pi()/2)*(TMath::Abs([1]) + TMath::Abs(([1]+[2]))))",-50,50);

    //For two sided gaus
    fitGaus->SetParLimits(0,currMin,currMax);
    fitGaus->SetParLimits(1,0,range/2);
    fitGaus->SetParLimits(2,-range/2,range/2);
    fitGaus->SetParameter(0,mean);
    fitGaus->SetParameter(1,width);
    fitGaus->SetParameter(2,0);
    fitGaus->SetParameter(3,1);
    fitGaus->FixParameter(3,1);

    if(!biVariate) {
      //For normal Gaus
      fitGaus->FixParameter(2,0);
    }

    str.str(stepCut);
    str << "Step>=" << skipSteps << " && " << treeBranches[i] << ">";
    str << mean-2*width;
    str << " && " << treeBranches[i] << "<" << mean+2*width;
    if(freq > 1)
      str << " && Step%" << freq << "==0";
    string mycut = str.str();
    fitGaus->SetRange(mean-2*width,mean+2*width);
    
    if(unbinnedPredictor) {
      testMCMC->UnbinnedFit("fitGaus", (treeBranches[i]).c_str(),
			    mycut.c_str());      
      mean = fitGaus->GetParameter(0);
      widthLow = fitGaus->GetParameter(1);
      widthHigh = TMath::Abs(fitGaus->GetParameter(2) + widthLow);
      cout << "First fit gives: Mean = " << mean << " + " << widthHigh;
      cout << " - " << widthLow << endl;
    } else {
      widthLow = width;
      widthHigh = width;
    }

    if(!unbinnedFit)
      fitGaus->ReleaseParameter(3);


    Double_t widthLowFixed = widthLow, widthHighFixed = widthHigh;


    TH1D *myhisto = new TH1D((treeBranches[i]).c_str(),(treeBranches[i]).c_str(), 40, mean-3*widthLow, mean+3*widthHigh);
    for(int step=0; step < nListEvents; step++) {
      if(step%freq == 0)
	myhisto->Fill(dataArray[step]);
    }

    if(!unbinnedFit)
      fitGaus->SetParameter(3,(nListEvents)*6*width/40);


    Double_t widthDiff = 0;

    //Currently fixing fit range to prevent problems with biVariate fits,
    //but this also sort of defeats the purpose of the iterated fits
    //Ideal would be to have program decide what to do dynamically
    //(i.e. if low asymmetry, use univariate automatically to avoid problems)
    mycut.clear();
    str.str(mycut);
    str << "Step>=" << skipSteps << " && " << treeBranches[i] << ">";
    str << mean-2*widthLowFixed;
    str << " && " << treeBranches[i] << "<" << mean+2*widthHighFixed;
    mycut = str.str();

    for(int iteration=0; iteration < 3; iteration++) {
      fitGaus->SetParameter(0,mean);
      fitGaus->SetParameter(1,widthLow);
      if(biVariate)
	fitGaus->SetParameter(2,widthDiff);
      fitGaus->SetRange(mean-2*widthLowFixed,mean+2*widthHighFixed);
      if(unbinnedFit) {
	 testMCMC->UnbinnedFit("fitGaus",(treeBranches[i]).c_str(),mycut.c_str());
      } else {
	myhisto->Fit("fitGaus","QLI","",mean-2*widthLowFixed,mean+2*widthHighFixed);
      }
      //fitinfo = myhisto->GetFunction("gaus");
      mean = fitGaus->GetParameter(0);
      widthLow = fitGaus->GetParameter(1);
      widthDiff = fitGaus->GetParameter(2);
      widthHigh = TMath::Abs(widthLow + widthDiff);
    }

    //Use average width, for now
    width = (widthLow + widthHigh)/2;

    Double_t widthHighErr;
    Double_t meanErr, widthErr;
    meanErr = fitGaus->GetParError(0);
    widthErr = fitGaus->GetParError(1);
    widthHighErr = fitGaus->GetParError(2);
    widthErr = TMath::Sqrt(2*widthErr*widthErr + widthHighErr*widthHighErr)/2;

    cout << treeBranches[i] << ": ";
    cout << "mean: " << mean << " +- " << meanErr << ", width: ";
    cout << width << " +- " << widthErr << endl;
    resultMap.insert(pair<string,double>(treeBranches[i]+"Mean",mean));
    resultMap.insert(pair<string,double>(treeBranches[i]+"MeanErr",meanErr));
    resultMap.insert(pair<string,double>(treeBranches[i]+"Width",width));
    resultMap.insert(pair<string,double>(treeBranches[i]+"WidthErr",widthErr));
    resultMap.insert(pair<string,double>(treeBranches[i]+"WidthDiff",widthDiff));

    mycanvas.Clear();
    myhisto->Draw();
//     fitGaus->SetParameter(2,mean-10*width);
//     fitGaus->SetParameter(3,mean+10*width);
    if(unbinnedFit) {
      fitGaus->SetParameter(3,(nListEvents)*6*width/40);
      fitGaus->Draw("SAME");
    }

    string tempfile = outFile;
    if(first) {
      tempfile = outFile + "(";
      first = false;
    }
    if(i == last)
      tempfile = outFile + ")";

    mycanvas.SaveAs(tempfile.c_str());

    delete myhisto;
    delete fitGaus;
    delete [] dataArray;

  }

  return resultMap;

}

TMatrixD *correlationMatrix(TTree *mytree, Int_t skipSteps, 
			   TObjArray& namesArray) {

  Int_t numSteps = mytree->GetEntries();

  //Get list of branch names
  vector<string> treeBranches;
  Int_t nTreeBranches = mytree->GetNbranches();
  TObjArray *branchNameArray;
  branchNameArray = mytree->GetListOfBranches();
  for(int i=0; i < nTreeBranches; i++) {
    TBranch *tempBranch = (TBranch*)branchNameArray->UncheckedAt(i);
    treeBranches.push_back(tempBranch->GetName());
  }

  TObjString *tempString;
  namesArray.SetOwner();
  namesArray.Clear();
  vector<int> goodPars;
  Double_t value;
  vector<Double_t> sigmas;
  vector<Double_t> means;
  //Only keep branches with non-zero sigma
  for(int i=3; i < nTreeBranches; i++) {
    if(treeBranches[i].substr(0,3) != "Acc")
      continue;
    mytree->SetBranchStatus("*",0);
    mytree->SetBranchStatus((treeBranches[i]).c_str(),1);
    mytree->SetBranchAddress((treeBranches[i]).c_str(),&value);

    Double_t xBar = 0;
    for(int step=skipSteps; step < numSteps; step++) {
      mytree->GetEvent(step);
      xBar += value;
    }
    xBar = xBar/(numSteps-skipSteps);

    Double_t sigmaTotal = 0;
    for(int step=skipSteps; step < numSteps; step++) {
      mytree->GetEvent(step);
      sigmaTotal += (value - xBar)*(value - xBar);
    }
    if(sigmaTotal > 1e-20) {
      goodPars.push_back(i);
      string nameString = (treeBranches[i]).substr(3);
      tempString = new TObjString(nameString.c_str());
      //tempString->Print("");
      namesArray.AddLast(tempString);
      sigmas.push_back(sigmaTotal);
      means.push_back(xBar);
    }
  }
  namesArray.SetOwner(kTRUE);
  
  TMatrixD *corrMat = new TMatrixD(goodPars.size(),goodPars.size());

  Int_t totalSteps = numSteps - skipSteps;

  Double_t *values = new Double_t[goodPars.size()*totalSteps];

  for(int i=0; i < goodPars.size(); i++) {
    mytree->SetBranchStatus("*",0);
    mytree->SetBranchStatus((treeBranches[goodPars[i]]).c_str(),1);
    mytree->SetBranchAddress((treeBranches[goodPars[i]]).c_str(),&value);

    for(int j=0; j < totalSteps; j++) {
      mytree->GetEvent(j+skipSteps);
      values[j+totalSteps*i] = value;
    }
  }

  Double_t coVar;
  for(int first=0; first < goodPars.size(); first++) {
    for(int second=0; second < goodPars.size(); second++) {
      coVar = 0;
      for(int i=0; i < totalSteps; i++) {
	  coVar += (values[i+totalSteps*first]-means[first])*\
	    (values[i+totalSteps*second]-means[second]);
      }
      coVar = coVar / TMath::Sqrt(sigmas[first]*sigmas[second]);
      (*corrMat)[first][second] = coVar;
    }
  }

  return corrMat;

}

int main(int argc, char *argv[]) {

  string inputFilename;
  Int_t skipSteps = 2000;
  Bool_t biVariate = false;
  Bool_t unbinnedFit = false;
  Bool_t unbinnedPredictor = true;
  Int_t freq = 1;

  if(argc < 2 || argc > 5) {
    cout << "Usage: autoFit.exe options filename\nGenerates an output ";
    cout << "file named filename.fits.root\n";
    cout << "Options: -number sets \"skipsteps\" to number, default is 2000\n";
    cout << "-unbinned sets to use unbinned fits (default is binned fits)\n";
    cout << "-biVariate sets to use bivariate gaussian (default is normal gaus)\n";
    cout << "-Snumber uses every numberth point in the fit (so -S50 would only use every 50th point\n";
    cout << "-noUnbinnedPredictor disables the initial unbinned fit used to predict values for the binned fits (default is to do this fit)\n";
    return 1;
  }

  for(int i=1; i < argc; i++) {
    if(argv[i][0] == '-') {
      string line = argv[i];
      if(line == "-unbinned") {
	unbinnedFit = true;
      } else if(line == "-biVariate") {
	biVariate = true;
      } else if(line == "-noUnbinnedPredictor") {
	unbinnedPredictor = false;
      } else if(line.substr(0,2) == "-S") {
	if(line.size() == 2) {
	  cout << "-S used without number, defaulting to 1\n";
	  freq = 1;
	} else {
	  freq = atoi(argv[i]+2);
	}
      } else if(argv[i][1] >= '0' && argv[i][1] <= '9') {
	skipSteps = atoi(argv[i]+1);
      } else {
	cout << "Unrecognized option " << line << ", exiting\n";
	return 1;
      }
    } else {
      if(inputFilename.size() != 0) {
	cout << "Multiple filenames entered, using last\n";
      }
      inputFilename = argv[i];      
    }
    if(skipSteps < 0) {
      cout << "Negative skipSteps entered, exiting\n";
      return 1;
    }
  }

  string fileNameIn, fileNamePrint;
  map<string,double> fileResults;
  TFile *myfile;
  TTree *mytree;
  //TFile outFile("summedResults.root","RECREATE");
  ostringstream str;
  str << inputFilename << ".fits.root";
  string outFileName = str.str();
  cout << "OutFile is " << outFileName << endl;
  TFile outFile(outFileName.c_str(),"RECREATE");
  outFile.cd();
  TTree *outTree = new TTree("FitResults","Fits to parameters");
  Double_t *values = NULL;

  TObjArray *branches;
  TObjArray namesArray;
  branches = &namesArray;
  TMatrixD *corrMatPoint=NULL;

  outTree->Branch("correlationMatrix","TMatrixD",&corrMatPoint);
  outTree->Branch("branchNames","TObjArray",&branches);

  //cin >> fileNameIn;
  //cout << fileNameIn << endl;
  //Bool_t first = true;
  //vector<string> listOfNames;
  //while(!cin.fail()) {
  myfile = new TFile(inputFilename.c_str());
  fileNameIn = inputFilename;
  if(myfile->IsZombie()) {
    cout << fileNameIn << " isn't a valid file, skipping\n";
    return 1;
  }
  mytree = dynamic_cast<TTree*>(myfile->Get("Tmcmc"));
  if(mytree == NULL) {
    cout << fileNameIn << " isn't a valid file, skipping\n";
    return 1;
  }
  
  fileNamePrint = fileNameIn+".fits.ps";
  fileResults = autoFit(mytree,skipSteps,fileNamePrint,biVariate,
			unbinnedFit,freq, unbinnedPredictor);
  map<string,double>::iterator resultIter;

  vector<string> listOfNames;

  values = new Double_t[fileResults.size()];
  Int_t current=0;
  for(resultIter = fileResults.begin(); resultIter != fileResults.end();
      resultIter++) {
    string tempName = resultIter->first;
    outTree->Branch(tempName.c_str(),&(values[current]),
		    (tempName+"/D").c_str());
    listOfNames.push_back(tempName);
    current++;
  }

  for(int nameNum=0; nameNum < listOfNames.size(); nameNum++)
    values[nameNum] = fileResults[listOfNames[nameNum]];

  corrMatPoint = correlationMatrix(mytree, skipSteps, namesArray);

  outFile.cd();
  outTree->Fill();

  delete corrMatPoint;
  delete myfile;

  //output fit results to file amenable to using drawResults.exe
  ofstream fitFile;
  fitFile.open((fileNameIn+".fitResult").c_str());
  map<string,double>::iterator iter;
  for(iter = fileResults.begin(); iter != fileResults.end(); iter++) {
    string tempstring = iter->first;
    if(tempstring.substr(tempstring.size()-4) == "Mean") {
      fitFile << tempstring.substr(3,tempstring.size()-7);
      fitFile << "=" << iter->second << endl;
      }
  }
  
  
  outFile.cd();
  outTree->Write();
  delete outTree;
  outFile.Close();
  


}
