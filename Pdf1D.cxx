/*****
1-D Pdf implementation.  This does a lot of the "real" work, this will
probably all need to be optimized, especially CalcLogLikelihood and RebuildPdf
*****/

#include "Pdf1D.h"

Pdf1D::Pdf1D() {
  masterPdf = NULL;
  dataArray = NULL;
  xAxisNum = 0;
  nDataPoints = 0;
}

Pdf1D::~Pdf1D() {
  if(masterPdf != NULL) {
    delete masterPdf;
    masterPdf = NULL;
  }
  if(dataArray != NULL) {
    delete [] dataArray;
    dataArray = NULL;
  }
}

Pdf1D& Pdf1D::operator = (const Pdf1D& other) {
  name = other.name;
  pdfNum = other.pdfNum;
  pars = other.pars;
  parNames = other.parNames;
  parNums = other.parNums;
  logLikelihood = other.logLikelihood;
  systematics = other.systematics;
  fluxes = other.fluxes;
  mcBranches = other.mcBranches; 
  axisNames = other.axisNames;
  axisNums = other.axisNums;

  masterPdf = dynamic_cast<TH1D*>(other.masterPdf->Clone());
  xAxisNum = other.xAxisNum;
  nDataPoints = other.nDataPoints;
  dataArray = other.dataArray; //So data not copied, just shared

  return *this;
}

Bool_t Pdf1D::SetupPdf(ConfigFile& config, vector<string> parNamesMCMC) {
  Bool_t succeeded = true;
  //Needs to do: setup masterPdf - create, set binning, set title/name/axis
  //create pdf, will overwrite bins to match config file
  masterPdf = new TH1D(name.c_str(),name.c_str(),4,0,1);

  string axisInfo, tempString;
  ostringstream ostr;

  //Now handled by PdfParent
//   vector<double> binVec;
//   Int_t binNum = 0;
//   tempString = ParInfoToString("_Axis_",0,"_Bin",binNum);
//   axisInfo.clear();
//   ostr.str(axisInfo);
//   ostr << "Pdf_" << pdfNum << tempString;
//   axisInfo = ostr.str();
//   while(config.keyExists(axisInfo)) {
//     double current = config.read<double>(axisInfo);
//     binVec.push_back(current);

//     binNum++;
//     tempString = ParInfoToString("_Axis_",0,"_Bin",binNum);
//     axisInfo.clear();
//     ostr.str(axisInfo);
//     ostr << "Pdf_" << pdfNum << tempString;
//     axisInfo = ostr.str();
//   }
  
  if(xAxisBins.size() < 2) {
    Errors::AddError("Error in "+axisInfo+": need at least one bin "\
		     "(two boundaries)");
    succeeded =  false;
  } else {
    int nBounds = xAxisBins.size();
    double *binBoundaries = new double[nBounds];
    for(int i=0; i < nBounds; i++)
      binBoundaries[i] = xAxisBins[i];
    masterPdf->SetBins(nBounds-1, binBoundaries);
    delete [] binBoundaries;
  }
  masterPdf->Rebuild();

  axisInfo.clear();
  ostr.str(axisInfo);
  ostr << "Pdf_" << pdfNum << "_Axis_0";
  axisInfo = ostr.str();

//   string axisName = config.read<string>(axisInfo+"_Name","");
//   if(axisName == "") {
//     Errors::AddError("Error: "+axisInfo+"_Name not defined");
//     succeeded = false;
//   } else {
//     masterPdf->GetXaxis()->SetTitle(axisName.c_str());
//   }
//   axisNames.push_back(axisName);

//   Int_t position = SearchStringVector(mcBranches, axisName);
//   if(position == -1) {
//     //axisName not in the branches in the tree, abort
//     Errors::AddError("Error: "+axisName+" not in MC Branches");
//     succeeded = false;
//   } else {
//     axisNums.push_back(position);
//   }

  axisInfo.clear();
  ostr.str(axisInfo);
  ostr << "Pdf_" << pdfNum;
  axisInfo = ostr.str();

  //Get and read in data
  string filename = config.read<string>(axisInfo+"_DataFile","");
  if(filename == "") {
    Errors::AddError("Error: "+axisInfo+"_DataFile not defined");
    return false;
  }
  TFile dataFile(filename.c_str());
  if(dataFile.IsZombie()) {
    Errors::AddError("Error: "+filename+" did not open properly");
    dataFile.Close();
    gROOT->cd();
    return false;
  }

  string treeName = config.read<string>(axisInfo+"_DataTree");
  TTree *dataTree = dynamic_cast<TTree*>(dataFile.Get(treeName.c_str()));
  if(dataTree == NULL) {
    Errors::AddError("Error: "+treeName+" in "+filename+" not found");
    dataFile.Close();
    gROOT->cd();
    return false;
  }

  //Now, read out branch names using some TObjArray trickery
  vector<string> treeBranches;
  Int_t nTreeBranches = dataTree->GetNbranches();
  TObjArray *branchNameArray;
  branchNameArray = dataTree->GetListOfBranches();
  for(int i=0; i < nTreeBranches; i++) {
    TBranch *tempBranch = (TBranch*)branchNameArray->UncheckedAt(i);
    treeBranches.push_back(tempBranch->GetName());
  }

  string axisName = axisNames[0];
  Double_t position = SearchStringVector(treeBranches, axisName);
  if(position == -1) {
    //axisName not in the branches in the tree, abort
    Errors::AddError("Error: "+axisName+" not a branch in "+treeName);
    dataFile.Close();
    gROOT->cd();
    return false;
  }

  Float_t dataEvent;
  dataTree->SetBranchAddress(axisName.c_str(),&dataEvent);
  nDataPoints = dataTree->GetEntries();
  dataArray = new Float_t[nDataPoints];
  for(int i=0; i < nDataPoints; i++) {
    dataTree->GetEvent(i);
    dataArray[i] = dataEvent;
  }
  
  dataFile.Close();
  gROOT->cd();

  return succeeded;
  
}

Double_t Pdf1D::CalcLogLikelihood() {
  //Will have non-normalized pdf, but know its normalization
  //Will use extended likelihood
  //Returns log likelihood
  //Need to figure out some way to minimize the number of calls to log

  //Check to see if parameters are out of range, return some
  //ridiculous value if they are
  Double_t penalty = -1e200;
  Int_t nPars = pars.size();
  for(int i=0; i < nPars; i++) {
    if(parHasMin[i] && pars[i] < parMins[i])
      return penalty;
    if(parHasMax[i] && pars[i] > parMaxes[i])
      return penalty;
  }

  //Later, move rebuilding pdf to parameter entry, so can test as pars come in
  //or at least, have it set a flag that pdf needs to be rebuilt - may not
  //be a point rebuilding then if likelihood never asked for
  RebuildPdf();
  NormalizePdfByBinWidth();

  //Double_t likelihood = 1;
  logLikelihood = 0;

  //Data
  for(int i = 0; i < nDataPoints; i++) {
    //logLikelihood += TMath::Log(GetValue(dataArray[i]));
    //Replaced log for each point by log of histo
    logLikelihood += GetValue(dataArray[i]);
  }
  //logLikelihood += TMath::Log(likelihood); //Catches leftovers
  logLikelihood -= totalEvents; //"Extended" part of likelihood
  

  //Sys constraints, GetContraint returns log likelihood
  int nSys = systematics.size();
  for(int i = 0; i < nSys; i++) {
    logLikelihood += systematics[i]->CalcLogLikelihood();
  }

  //Bkgd constraints

  //Flux constraints
  nSys = fluxes.size();
  for(int i=0; i < nSys; i++) {
    logLikelihood += fluxes[i]->CalcLogLikelihood();
  }

  //cout << "Pdf1D logLikelhood = " << logLikelihood << endl;

  return logLikelihood;

}

void Pdf1D::RebuildPdf() {
  //Ah, the real tricky one.  This puts the masterPdf together
  //from the various fluxes, systematics and backgrounds

  masterPdf->Reset();

  int nFluxes = fluxes.size();
//   int nSys = systematics.size();
  for(int fluxNum=0; fluxNum < nFluxes; fluxNum++) {

    //Set flux number in Sys, kludgy
    for(int i=0; i < systematics.size(); i++)
      systematics[i]->SetFluxType(fluxes[fluxNum]->GetFluxType());

    const vector<Double_t>& eventData = fluxes[fluxNum]->GetEvent(0);
    vector<Double_t> difference;
    int nFluxEvents = fluxes[fluxNum]->GetNEvents();
    Double_t weight = fluxes[fluxNum]->GetFluxSize() / \
      fluxes[fluxNum]->GetTimesFlux();
    difference.assign(eventData.size(),0);
    int eventDataSize = eventData.size();
    int weightLocation = eventDataSize - 1;

    int eventLoc=0, sysNum=0;

    vector<Sys*> activeSys;
    for(int i=0; i < systematics.size(); i++)
      if(systematics[i]->CheckIfAffected())
	activeSys.push_back(systematics[i]);
    Int_t numActiveSys = activeSys.size();

//     cout << "For Flux " << fluxes[fluxNum]->GetName() << endl;
//     for(int i=0; i < activeSys.size(); i++)
//       cout << "Active Sys: " << activeSys[i]->GetName() << endl;

    for(int eventNum=0; eventNum < nFluxEvents; eventNum++) {
      fluxes[fluxNum]->GetEvent(eventNum);
      //Kludgy.  Need a better way
      //difference.assign(eventData.size(),0);
      for(eventLoc=0; eventLoc < weightLocation; eventLoc++) {
	difference[eventLoc] = 0;
      }
      difference[weightLocation] = 1;

      //Apply sys to event
      for(sysNum = 0; sysNum < numActiveSys; sysNum++) {
	activeSys[sysNum]->Apply(eventData, difference);
      }
      for(eventLoc=0; eventLoc < eventDataSize; eventLoc++)
	difference[eventLoc] += eventData[eventLoc];

      masterPdf->Fill(difference[axisNums[0]],
		      weight*difference[weightLocation]);
    }

  }

  //Done with Fluxes, add backgrounds
  int nBins = masterPdf->GetNbinsX();
  int nBkgds = backgrounds.size();
  Double_t weight;
  Double_t binCenter;
  for(int bkgdNum=0; bkgdNum < nBkgds; bkgdNum++) {
    for(int bin=1; bin <= nBins; bin++) {
      weight = backgrounds[bkgdNum]->GetBinContent(bin);
      binCenter = masterPdf->GetBinCenter(bin);
      masterPdf->Fill(binCenter,weight);
    }
  }

  //Make sure each bin has at least 0.01 events, just to keep Log from
  //reacting poorly.  If bin is negative, quit - something went wrong!
  //Calculate integrated total number of events, remember 1-based counting
  totalEvents = 0;
  Double_t binValue;
  for(int bin=1; bin <= nBins; bin++) {
    binValue = masterPdf->GetBinContent(bin);
    if(binValue < -1e-11) {
      //Negative!  Bad!
      cout << "Negative pdf value!\n";
      cerr << "Negative pdf value!\n";
      this->PrintState();
      Errors::Exit("Negative pdf value encountered, exiting");
    }
    if(binValue < 1e-10) {
      binValue = 1e-10;
      masterPdf->SetBinContent(bin, binValue);
    }
    totalEvents += binValue;
  }
  //cout << "Pdf " << name << " has total events = " << totalEvents << endl;

}

void Pdf1D::NormalizePdfByBinWidth() {
  //Very important that this not change the value of totalEvents
  //even though it changes the overall scale on the pdf
  //This also takes the log of each bin, to speed up computing
  Double_t binValue, binWidth;

  for(int bin=1; bin <= masterPdf->GetXaxis()->GetNbins(); bin++) {
    binValue = masterPdf->GetBinContent(bin);
    binWidth = masterPdf->GetXaxis()->GetBinWidth(bin);
    binValue = TMath::Log(binValue/binWidth);
    masterPdf->SetBinContent(bin, binValue);
  }
}

inline Double_t Pdf1D::GetValue(Double_t point) {
  return masterPdf->GetBinContent(masterPdf->FindBin(point));
}

void Pdf1D::PrintState() {
  cout << "Printing state of " << name << " of class Pdf1D\n";

  masterPdf->Print();
  cout << "nPdfBins = " << masterPdf->GetNbinsX() << endl;
  cout << "Integral = " << masterPdf->Integral() << endl;
  for(int i=1; i <= masterPdf->GetNbinsX(); i++)
    cout << "Content of bin " << i << " = " << masterPdf->GetBinContent(i) << endl;
  for(int i=1; i <= masterPdf->GetNbinsX(); i++)
    cout << "Bin center " << i << " = " << masterPdf->GetBinCenter(i) << endl;
  //  cout << "xAxisNum = " << xAxisNum << endl;
  cout << "nDataPoints = " << nDataPoints << endl;
  cout << "pdfNum = " << pdfNum << endl;
  cout << "pdfDim = " << pdfDim << endl;
  cout << "logLikelihood = " << logLikelihood << endl;
  cout << "totalEvents = " << totalEvents << endl;
  for(int i=0; i < pars.size(); i++)
    cout << "pars[" << i << "] = " << pars[i] << endl;
  for(int i=0; i < parNames.size(); i++)
    cout << "parNames[" << i << "] = " << parNames[i] << endl;
  for(int i=0; i < parNums.size(); i++)
    cout << "parNums[" << i << "] = " << parNums[i] << endl;
  for(int i=0; i < mcBranches.size(); i++)
    cout << "mcBranches[" << i << "] = " << mcBranches[i] << endl;
  for(int i=0; i < axisNames.size(); i++)
    cout << "axisNames[" << i << "] = " << axisNames[i] << endl;
  for(int i=0; i < axisNums.size(); i++)
    cout << "axisNums[" << i << "] = " << axisNums[i] << endl;
  for(int i=0; i < systematics.size(); i++)
    systematics[i]->PrintState();

  for(int i=0; i < backgrounds.size(); i++)
    backgrounds[i]->PrintState();

}

void Pdf1D::Draw(string fileToWriteTo) {

  gStyle->SetOptFit(0);
  gStyle->SetOptStat(0);

  TCanvas *mycanvas = new TCanvas(name.c_str(),name.c_str());
  mycanvas->cd();

  TH1D *dataHisto, *mcHisto, **fluxHisto;
  fluxHisto = new TH1D*[fluxes.size()];

  for(int i = 0; i < fluxes.size(); i++)
    fluxHisto[i] = DrawFlux(i);
  
  //Create data histo
  //Hopefully, this will copy axes and such from masterPdf to dataHisto
  dataHisto = new TH1D("dataHisto","dataHisto",4,0,1);
  mcHisto = new TH1D("mcHisto","mcHisto",4,0,1);

  Double_t *binsArray = new Double_t[xAxisBins.size()];
  for(int i=0; i < xAxisBins.size(); i++) {
    binsArray[i] = xAxisBins[i];
  }
  dataHisto->GetXaxis()->Set(xAxisBins.size()-1, binsArray);
  dataHisto->GetXaxis()->SetTitle(axisNames[0].c_str());
  dataHisto->Rebuild();

  mcHisto->GetXaxis()->Set(xAxisBins.size()-1, binsArray);
  mcHisto->GetXaxis()->SetTitle(axisNames[0].c_str());
  mcHisto->Rebuild(); 

  for(int i=0; i < nDataPoints; i++) {
    dataHisto->Fill(dataArray[i]);
  }

  mcHisto->Reset();
  for(int i=0; i < fluxes.size(); i++) {
    mcHisto->Add(fluxHisto[i]);
  }
  
  //Compute chi^2 and print it, assume an error of 1 if oounts = 0
  //I don't think this is working properly
  Double_t dataValue=0, pdfValue=0, chisq=0, sigmaSq = 1;
  Int_t nDOF = 0;
  for(int x=1; x <= xAxisBins.size()-1; x++) {
	pdfValue = masterPdf->GetBinContent(x);
	dataValue = dataHisto->GetBinContent(x);
	sigmaSq = dataValue;
	if(sigmaSq <= 0)
	  sigmaSq = 1;
	chisq += (pdfValue-dataValue)*(pdfValue-dataValue)/sigmaSq;
	nDOF++;
  }
  //cout << "Chi^2 = " << chisq << " for " << nDOF << " bins\n";



  mycanvas->Clear();
  mycanvas->cd();
  
  dataHisto->SetLineColor(1);
  dataHisto->Draw("P0 E1 X0 *H");
  gPad->Update();
  mcHisto->SetLineColor(2);
  mcHisto->Draw("SAMES");

  //Now draw all of the individual fluxes/bkgds, ugh
  for(int i=0; i < fluxes.size(); i++) {
    fluxHisto[i]->SetLineColor((i+2)%9+1);
    fluxHisto[i]->Draw("SAME");
  }

  Double_t xmin=0.75, xmax=0.95, ymin=0.65, ymax=0.95;
  

  TLegend *mylegend = new TLegend(xmin,ymin,xmax,ymax);
  mylegend->SetBorderSize(1);
  mylegend->AddEntry(dataHisto,"Data");
  mylegend->AddEntry(mcHisto,"MC");
  for(int i=0; i < fluxes.size(); i++)
    mylegend->AddEntry(fluxHisto[i],(fluxes[i]->GetName()).c_str());
  mylegend->Draw();

  string filenameToDraw;
  if(fileToWriteTo[fileToWriteTo.size()-1] == '!') {
    filenameToDraw = fileToWriteTo.substr(0,fileToWriteTo.size()-1);
  } else {
    filenameToDraw = fileToWriteTo;
  }
  
  mycanvas->SaveAs(filenameToDraw.c_str());

  masterPdf->SetLineColor(kBlack);

  delete dataHisto;
  delete [] binsArray;
  for(int i=0; i < fluxes.size(); i++)
    delete fluxHisto[i];

  delete mylegend;
  delete mycanvas;
  delete [] fluxHisto;

}


TH1D* Pdf1D::DrawFlux(Int_t fluxNumber) {

  //int nSys = systematics.size();
  Int_t fluxNum = fluxNumber;

  TH1D *dataHisto = new TH1D((fluxes[fluxNum]->GetName()).c_str(),
		       (fluxes[fluxNum]->GetName()).c_str(),4,0,1);

  //Get axis right
  vector<Double_t> axisVec;
    axisVec = xAxisBins;

  Double_t *binsArray = new Double_t[axisVec.size()];
  for(int i=0; i < axisVec.size(); i++) {
    binsArray[i] = axisVec[i];
  }
  dataHisto->GetXaxis()->Set(axisVec.size()-1, binsArray);
  dataHisto->GetXaxis()->SetTitle(axisNames[0].c_str());
  dataHisto->Rebuild();

  //Set flux number in Sys, kludgy
  for(int i=0; i < systematics.size(); i++)
    systematics[i]->SetFluxType(fluxes[fluxNum]->GetFluxType());
  
  const vector<Double_t>& eventData = fluxes[fluxNum]->GetEvent(0);
  vector<Double_t> difference;
  int nFluxEvents = fluxes[fluxNum]->GetNEvents();
  Double_t weight = fluxes[fluxNum]->GetFluxSize() / \
    fluxes[fluxNum]->GetTimesFlux();
  difference.assign(eventData.size(),0);
  int eventDataSize = eventData.size();
  int weightLocation = eventDataSize - 1;

  vector<Sys*> activeSys;
  for(int i=0; i < systematics.size(); i++)
    if(systematics[i]->CheckIfAffected())
      activeSys.push_back(systematics[i]);
  Int_t numActiveSys = activeSys.size();
  
  for(int eventNum=0; eventNum < nFluxEvents; eventNum++) {
    fluxes[fluxNum]->GetEvent(eventNum);
    //Kludgy.  Need a better way
    //difference.assign(eventData.size(),0);
    for(int i=0; i < weightLocation; i++) {
      difference[i] = 0;
    }
    difference[weightLocation] = 1;
    
    //Apply sys to event
    for(int sysNum = 0; sysNum < numActiveSys; sysNum++) {
      activeSys[sysNum]->Apply(eventData, difference);
    }
    for(int i=0; i < eventDataSize; i++)
      difference[i] += eventData[i];
    
    dataHisto->Fill(difference[axisNums[0]],
		    weight*difference[weightLocation]);
    
  }

  Double_t totalFluxEvents = 0;
  totalFluxEvents = dataHisto->Integral();
    
  cout << "Flux " << fluxes[fluxNum]->GetName() << " has ";
  cout << totalFluxEvents << " events\n";

  delete [] binsArray;

  return dataHisto;
  
}
