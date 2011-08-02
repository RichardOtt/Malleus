/*****
3-D Pdf implementation.  This does a lot of the "real" work, this will
probably all need to be optimized, especially CalcLogLikelihood and RebuildPdf
*****/

#include "Pdf3D.h"

Pdf3D::Pdf3D() {
  masterPdf = NULL;
  dataArray = NULL;
}

Pdf3D::~Pdf3D() {
  if(masterPdf != NULL) {
    delete masterPdf;
    masterPdf = NULL;
  }
  if(dataArray != NULL) {
    delete [] dataArray;
    dataArray = NULL;
  }
}

Pdf3D& Pdf3D::operator = (const Pdf3D& other) {
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

  masterPdf = dynamic_cast<TH3D*>(other.masterPdf->Clone());
  nDataPoints = other.nDataPoints;
  dataArray = other.dataArray; //So data not copied, just shared

  return *this;
}

Bool_t Pdf3D::SetupPdf(ConfigFile& config, vector<string> parNamesMCMC) {
  Bool_t succeeded = true;
  //Needs to do: setup masterPdf - create, set binning, set title/name/axis
  //create pdf, will overwrite bins to match config file
  masterPdf = new TH3D(name.c_str(),name.c_str(),4,0,1,4,0,1,4,0,1);

  cout << "Starting setup for 3D pdf " << name << endl;

  string axisInfo, tempString;
  ostringstream ostr;

  //Axes information (xAxisBins, etc) filled in by PdfParent
  for(int axis=0; axis < 3; axis++)
    succeeded &= SetupAxis(config, axis);

  //Debug
  //Print out xAxisBins, yAxisBins, zAxisBins, axisNames, axisNums
//   for(int tempNumber = 0; tempNumber < xAxisBins.size(); tempNumber++)
//     cout << "xAxisBins[" << tempNumber << "] = " << xAxisBins[tempNumber] << endl;
//   for(int tempNumber = 0; tempNumber < yAxisBins.size(); tempNumber++)
//     cout << "yAxisBins[" << tempNumber << "] = " << yAxisBins[tempNumber] << endl;
//   for(int tempNumber = 0; tempNumber < zAxisBins.size(); tempNumber++)
//     cout << "zAxisBins[" << tempNumber << "] = " << zAxisBins[tempNumber] << endl;
//   for(int tempNumber = 0; tempNumber < axisNames.size(); tempNumber++)
//     cout << "axisNames[" << tempNumber << "] = " << axisNames[tempNumber] << endl;
//   for(int tempNumber = 0; tempNumber < axisNums.size(); tempNumber++)
//     cout << "axisNums[" << tempNumber << "] = " << axisNums[tempNumber] << endl;
  

  //Undocumented root "feature" - need to rebuild histo if altering
  //axes directly
  masterPdf->Rebuild();

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
  cout << "Opened file " << filename << endl;

  string treeName = config.read<string>(axisInfo+"_DataTree");
  TTree *dataTree = dynamic_cast<TTree*>(dataFile.Get(treeName.c_str()));
  if(dataTree == NULL) {
    Errors::AddError("Error: "+treeName+" in "+filename+" not found");
    dataFile.Close();
    gROOT->cd();
    return false;
  }
  cout << "Read tree " << treeName << endl;

  //Now, read out branch names using some TObjArray trickery
  vector<string> treeBranches;
  Int_t nTreeBranches = dataTree->GetNbranches();
  TObjArray *branchNameArray;
  branchNameArray = dataTree->GetListOfBranches();
  for(int i=0; i < nTreeBranches; i++) {
    TBranch *tempBranch = (TBranch*)branchNameArray->UncheckedAt(i);
    treeBranches.push_back(tempBranch->GetName());
  }

  for(Int_t axisNumber=0; axisNumber < 3; axisNumber++) {
    int position = SearchStringVector(treeBranches, axisNames[axisNumber]);
    if(position == -1) {
      //axisName not in the branches in the tree, abort
      Errors::AddError("Error: "+axisNames[axisNumber]+\
		       " not a branch in "+treeName);
      dataFile.Close();
      gROOT->cd();
      return false;
    }
  }
  //So now we're sure all 3 axes appear as branches in the tree

  cout << "Starting to read data for 3D pdf " << name << endl;

  //Read data in to internal array, making sure all data points
  //are within the bounds of the bins, reject those that aren't
  Double_t xMin, xMax, yMin, yMax, zMin, zMax;
  xMin = xAxisBins[0];
  xMax = xAxisBins[xAxisBins.size()-1];
  yMin = yAxisBins[0];
  yMax = yAxisBins[yAxisBins.size()-1];
  zMin = zAxisBins[0];
  zMax = zAxisBins[zAxisBins.size()-1];
  Bool_t inRange = true;
  Float_t dataEventX;
  Float_t dataEventY;
  Float_t dataEventZ;
  dataTree->SetBranchAddress(axisNames[0].c_str(),&dataEventX);
  cout << "dataEventX matched to " << axisNames[0] << endl;
  dataTree->SetBranchAddress(axisNames[1].c_str(),&dataEventY);
  cout << "dataEventY matched to " << axisNames[1] << endl;
  dataTree->SetBranchAddress(axisNames[2].c_str(),&dataEventZ);
  cout << "dataEventZ matched to " << axisNames[2] << endl;
  nDataPoints = dataTree->GetEntries();
  cout << "Bounds: (" << xMin << ", " << yMin << ", " << zMin << "); (";
  cout << xMax << ", " << yMax << ", " << zMax << ")\n";
  vector<Double_t> dataVec;
  for(int i=0; i < nDataPoints; i++) {
    dataTree->GetEvent(i);
    inRange = true;
    inRange &= (dataEventX >= xMin && dataEventX < xMax);
    inRange &= (dataEventY >= yMin && dataEventY < yMax);
    inRange &= (dataEventZ >= zMin && dataEventZ < zMax);
    if(!inRange) {
      cout << "Data point out of axis range in " << name << ", skipping\n";
      cout << "Point is: (" << dataEventX << ", " << dataEventY << ", ";
      cout << dataEventZ << "); event " << i << endl;
      continue;
    }
    dataVec.push_back(dataEventX);
    dataVec.push_back(dataEventY);
    dataVec.push_back(dataEventZ);
  }
  
  nDataPoints = dataVec.size()/3;
  dataArray = new Float_t[dataVec.size()];
  for(int i=0; i < dataVec.size(); i++) {
    dataArray[i] = dataVec[i];
  }

  dataFile.Close();
  gROOT->cd();

  return succeeded;
  
}

Double_t Pdf3D::CalcLogLikelihood() {
  //Will have non-normalized pdf, but know its normalization
  //Will use extended likelihood
  //Returns log likelihood
  //Need to figure out some way to minimize the number of calls to log

  //I'll have check to see if parameters are out of range, return some
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
  //Actually, it's probably best to keep the pdf up-to-date with the pars,
  //if I plan on also using this to draw itself
  RebuildPdf();

  NormalizePdfByBinWidth();

  //Double_t likelihood = 1;
  logLikelihood = 0;

  //Data
  for(int i = 0; i < nDataPoints; i++) {
    //likelihood *= TMath::Max(GetValue(dataArray[i]),1.0);
    //Is this check more expensive than just calling log every time?
//     if(i%1000 == 0) {
//       logLikelihood += TMath::Log(likelihood);
//       likelihood = 1;
//     }
    logLikelihood += TMath::Log(GetValue(dataArray[3*i],dataArray[3*i+1],
					 dataArray[3*i+2]));
  }
  //logLikelihood += TMath::Log(likelihood); //Catches leftovers
  logLikelihood -= totalEvents; //"Extended" part of likelihood
  

  //Sys constraints, GetContraint returns log likelihood
  int nSys = systematics.size();
  for(int i = 0; i < nSys; i++) {
    logLikelihood += systematics[i]->CalcLogLikelihood();
  }

  //Bkgd constraints
  nSys = backgrounds.size();
  for(int i=0; i < nSys; i++) {
    logLikelihood += backgrounds[i]->CalcLogLikelihood();
  }

  //Flux constraints
  nSys = fluxes.size();
  for(int i=0; i < nSys; i++) {
    logLikelihood += fluxes[i]->CalcLogLikelihood();
  }

  //cout << "Pdf3D logLikelhood = " << logLikelihood << endl;

  return logLikelihood;

}

void Pdf3D::RebuildPdf() {
  //Ah, the really tricky one.  This puts the masterPdf together
  //from the various fluxes, systematics and backgrounds

  masterPdf->Reset();

  int nFluxes = fluxes.size();
//   int nSys = systematics.size();
  for(int fluxNum=0; fluxNum < nFluxes; fluxNum++) {

    //Set flux number in Sys, kludgy
    for(int i=0; i < systematics.size(); i++) {
      systematics[i]->SetFluxType(fluxes[fluxNum]->GetFluxType());
    }

    const vector<Double_t>& eventData = fluxes[fluxNum]->GetEvent(0);

    vector<Double_t> difference;
    int nFluxEvents = fluxes[fluxNum]->GetNEvents();
    Double_t weight = fluxes[fluxNum]->GetFluxSize() / fluxes[fluxNum]->GetTimesFlux();
    difference.assign(eventData.size(),0);
    int eventDataSize = eventData.size();
    int weightLocation = eventDataSize - 1;

    int eventLoc=0, sysNum=0;

    vector<Sys*> activeSys;
    for(int i=0; i < systematics.size(); i++)
      if(systematics[i]->CheckIfAffected())
	activeSys.push_back(systematics[i]);
    Int_t numActiveSys = activeSys.size();

//     cout << fluxes[fluxNum]->GetName();
//     for(int i=0; i < activeSys.size(); i++)
//       cout << " " << activeSys[i]->GetName();
//     cout << endl;

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
      difference[weightLocation]=1;

      //Apply sys to event
      for(sysNum = 0; sysNum < numActiveSys; sysNum++) {
	activeSys[sysNum]->Apply(eventData, difference);
      }
      for(eventLoc=0; eventLoc < eventDataSize; eventLoc++)
	difference[eventLoc] += eventData[eventLoc];
//       if(eventNum == 25) {
// 	cout << "Internal weight being applied: ";
// 	cout << difference[eventData.size()-1] << " with external weight ";
// 	cout << weight << endl;
//       }

      masterPdf->Fill(difference[axisNums[0]],difference[axisNums[1]],
		      difference[axisNums[2]],
		      weight*difference[weightLocation]);
    }

  }
  //Done with Fluxes, add backgrounds
  int nBkgds = backgrounds.size();
  Double_t weight;
  Double_t binCenterX, binCenterY, binCenterZ;
  for(int bkgdNum=0; bkgdNum < nBkgds; bkgdNum++) {
    for(int x=1; x <= masterPdf->GetNbinsX(); x++) {
      for(int y=1; y <= masterPdf->GetNbinsY(); y++) {
	for(int z=1; z <= masterPdf->GetNbinsZ(); z++) {
	  weight = backgrounds[bkgdNum]->GetBinContent(x,y,z);
	  binCenterX = masterPdf->GetXaxis()->GetBinCenter(x);
	  binCenterY = masterPdf->GetYaxis()->GetBinCenter(y);
	  binCenterZ = masterPdf->GetZaxis()->GetBinCenter(z);
	  masterPdf->Fill(binCenterX, binCenterY, binCenterZ ,weight);
	}
      }
    }
  }

//   cout << "Integral gives: " << masterPdf->Integral() << endl;

  //Make sure each bin has at least 1e-10 events, just to keep Log from
  //reacting poorly
  //Should I put a check in here for negative bins?  Force logLikelihood 
  //to report a limit if there are?
  totalEvents = 0;
  Double_t binValue = 0;
  for(int x=1; x <= masterPdf->GetNbinsX(); x++) {
    for(int y=1; y <= masterPdf->GetNbinsY(); y++) {
      for(int z=1; z <= masterPdf->GetNbinsZ(); z++) {
	binValue = masterPdf->GetBinContent(x,y,z);
	if(binValue < 1e-10) {
	  binValue = 1e-10;
	  masterPdf->SetBinContent(x,y,z,1e-10);
	}
	totalEvents += binValue;
      }
    }
  }
  //cout << "Pdf " << name << " has total events = " << totalEvents << endl;


  //Calculate integrated total number of events, the lazy way
  //totalEvents = masterPdf->Integral();

}

void Pdf3D::NormalizePdfByBinWidth() {
  //Very important that this not change the value of totalEvents
  //even though it changes the overall scale on the pdf
  Double_t binValue = 0, binWidth = 0;

  for(int x=1; x <= masterPdf->GetNbinsX(); x++) {
    for(int y=1; y <= masterPdf->GetNbinsY(); y++) {
      for(int z=1; z <= masterPdf->GetNbinsZ(); z++) {
	binValue = masterPdf->GetBinContent(x,y,z);
	binWidth = masterPdf->GetXaxis()->GetBinWidth(x);
	binWidth *= masterPdf->GetYaxis()->GetBinWidth(y);
	binWidth *= masterPdf->GetZaxis()->GetBinWidth(z);
	binValue = binValue/binWidth;
	masterPdf->SetBinContent(x,y,z,binValue);
      }
    }
  }

}

inline Double_t Pdf3D::GetValue(Double_t x, Double_t y, Double_t z) {
  return masterPdf->GetBinContent(masterPdf->FindBin(x,y,z));
}

Bool_t Pdf3D::SetupAxis(ConfigFile& config, Int_t axisNumber) {
  //Reads axis info from config file, sets up axis (binning, name)
  //Fills AxisNums, AxisNames

  Bool_t succeeded = true;

  string axisInfo, tempString;
  ostringstream ostr;

  vector<double> binVec;
  //Int_t binNum = 0;
  Double_t *binBoundaries = NULL;
  //Handled by PdfParent
 //  tempString = ParInfoToString("_Axis_",axisNumber,"_Bin",binNum);
//   axisInfo.clear();
//   ostr.str(axisInfo);
//   ostr << "Pdf_" << pdfNum << tempString;
//   axisInfo = ostr.str();
//   while(config.keyExists(axisInfo)) {
//     double current = config.read<double>(axisInfo);
//     binVec.push_back(current);

//     binNum++;
//     tempString = ParInfoToString("_Axis_",axisNumber,"_Bin",binNum);
//     axisInfo.clear();
//     ostr.str(axisInfo);
//     ostr << "Pdf_" << pdfNum << tempString;
//     axisInfo = ostr.str();
//   }
  
  if(axisNumber ==0) {
    binVec = xAxisBins;
  } else if(axisNumber == 1) {
    binVec = yAxisBins;
  } else if(axisNumber == 2) {
    binVec = zAxisBins;
  }

  if(binVec.size() < 2) {
    Errors::AddError("Error in "+axisInfo+": need at least one bin "\
		     "(two boundaries)");
    succeeded =  false;
  } else {
    int nBounds = binVec.size();
    binBoundaries = new Double_t[nBounds];
    for(int i=0; i < nBounds; i++)
      binBoundaries[i] = binVec[i];
    if(axisNumber == 0) {
      masterPdf->GetXaxis()->Set(nBounds-1, binBoundaries);
    } else if(axisNumber == 1) {
      masterPdf->GetYaxis()->Set(nBounds-1, binBoundaries);
    } else if(axisNumber == 2) {
      masterPdf->GetZaxis()->Set(nBounds-1, binBoundaries);
    }
  }

  axisInfo.clear();
  ostr.str(axisInfo);
  ostr << "Pdf_" << pdfNum << "_Axis_" << axisNumber;
  axisInfo = ostr.str();

//   string axisName = config.read<string>(axisInfo+"_Name","");
//   if(axisName == "") {
//     Errors::AddError("Error: "+axisInfo+"_Name not defined");
//     succeeded = false;
//   } else {
//     cout << "Found axis " << axisName << " for Pdf " << name << endl;
//     if(axisNumber == 0)
//       masterPdf->GetXaxis()->SetTitle(axisName.c_str());
//     if(axisNumber == 1)
//       masterPdf->GetYaxis()->SetTitle(axisName.c_str());
//     if(axisNumber == 2)
//       masterPdf->GetZaxis()->SetTitle(axisName.c_str());
//   }
//   axisNames.push_back(axisName);

//   Int_t position = SearchStringVector(mcBranches, axisName);
//   if(position == -1) {
//     //axisName not in the branches in the tree, abort
//     Errors::AddError("Error: "+axisName+" not in MC Branches");
//     cout << "position == -1 for " << axisName << endl;
//     succeeded = false;
//   } else {
//     axisNums.push_back(position);
//     cout << axisName << " found in mcBranches at position " << position << endl;
//   }

  delete [] binBoundaries;

  return succeeded;
}

void Pdf3D::PrintState() {
  cout << "Printing state of " << name << " of class Pdf3D\n";

  masterPdf->Print();
  cout << "nPdfBinsX = " << masterPdf->GetNbinsX() << endl;
  cout << "nPdfBinsY = " << masterPdf->GetNbinsY() << endl;
  cout << "nPdfBinsZ = " << masterPdf->GetNbinsZ() << endl;
  cout << "Bin Centers: " << endl;
  for(int i=0; i < masterPdf->GetNbinsX(); i++)
    cout << "X bin " << i << ": " << masterPdf->GetXaxis()->GetBinCenter(i) << endl;
  for(int i=0; i < masterPdf->GetNbinsY(); i++)
    cout << "Y bin " << i << ": " << masterPdf->GetYaxis()->GetBinCenter(i) << endl;
  for(int i=0; i < masterPdf->GetNbinsZ(); i++)
    cout << "Z bin " << i << ": " << masterPdf->GetZaxis()->GetBinCenter(i) << endl;
  
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
  //Print out histo contents(!)
  for(int xBin=1; xBin <= masterPdf->GetXaxis()->GetNbins(); xBin++) {
    for(int yBin=1; yBin <= masterPdf->GetYaxis()->GetNbins(); yBin++) {
      for(int zBin=1; zBin <= masterPdf->GetZaxis()->GetNbins(); zBin++) {
	cout << "Bin(" << xBin << "," << yBin << "," << zBin << ") = ";
	cout << masterPdf->GetBinContent(xBin,yBin,zBin) << endl;
      }
    }
  }  
  for(int i=0; i < systematics.size(); i++)
    systematics[i]->PrintState();
  for(int i=0; i < backgrounds.size(); i++)
    backgrounds[i]->PrintState();

  
}

void Pdf3D::Draw(string fileToWriteTo) {
  //Ideally, want it to print out the "final" graphs like Blair made

  gStyle->SetOptFit(0);
  gStyle->SetOptStat(0);


  //Will make three 1-D projections, one for each data axis.
  TCanvas *mycanvas = new TCanvas(name.c_str(),name.c_str());
  //mycanvas->SetLogy();
  mycanvas->cd();
  
  TH1D *dataHisto, *mcHisto, **fluxHisto;
  fluxHisto = new TH1D*[fluxes.size()];
  
  //Build 3-D data histo for computing chi^2
  Double_t *xbins = new Double_t[xAxisBins.size()];
  Double_t *ybins = new Double_t[yAxisBins.size()];
  Double_t *zbins = new Double_t[zAxisBins.size()];
  for(int i=0; i < xAxisBins.size(); i++)
    cout << "xbins[" << i << "] = " << xAxisBins[i] << endl;
  for(int i=0; i < xAxisBins.size(); i++)
    xbins[i] = xAxisBins[i];
  for(int i=0; i < yAxisBins.size(); i++)
    ybins[i] = yAxisBins[i];
  for(int i=0; i < zAxisBins.size(); i++)
    zbins[i] = zAxisBins[i];
  TH3D *dataPdf = new TH3D("data","data",xAxisBins.size()-1,xbins,
			   yAxisBins.size()-1,ybins, zAxisBins.size()-1,zbins);
  dataPdf->Reset();
  for(int i=0; i < nDataPoints; i++) {
    dataPdf->Fill(dataArray[3*i],dataArray[3*i+1],dataArray[3*i+2]);
  }

  //Compute chi^2, assume an error of 1 if counts = 0
  Double_t dataValue=0, pdfValue=0, chisq=0, sigmaSq = 1;
  Int_t nDOF = 0;
  for(int x=1; x <= masterPdf->GetNbinsX(); x++) {
    for(int y=1; y <= masterPdf->GetNbinsY(); y++) {
      for(int z=1; z <= masterPdf->GetNbinsZ(); z++) {
	pdfValue = masterPdf->GetBinContent(x,y,z);
	dataValue = dataPdf->GetBinContent(x,y,z);
	sigmaSq = dataValue;
	if(sigmaSq <= 1)
	  sigmaSq = 1;
	chisq += (pdfValue-dataValue)*(pdfValue-dataValue)/sigmaSq;
	nDOF++;
      }
    }
  }
  cout << "Chi^2 = " << chisq << " for " << nDOF << " bins\n";

  delete dataPdf;
  delete [] xbins;
  delete [] ybins;
  delete [] zbins;

  string tempFilename;
  char lastChar = fileToWriteTo[fileToWriteTo.size()-1];
  if(lastChar == '(' || lastChar == ')' || lastChar == '!')
    tempFilename = fileToWriteTo.substr(0,fileToWriteTo.size()-1);
  else
    tempFilename = fileToWriteTo;

  string outputCSVName;
  Int_t dotLocation = tempFilename.find_last_of(".");
  if(dotLocation == -1)
    outputCSVName = tempFilename + ".csv";
  else
    outputCSVName = tempFilename.substr(0,dotLocation) + ".csv";
  ofstream outputCSV;
  if(lastChar == '(')
    outputCSV.open(outputCSVName.c_str());
  else
    outputCSV.open(outputCSVName.c_str(),ios_base::out | ios_base::app);
  
  
  for(int axisToDraw = 0; axisToDraw < 3; axisToDraw++) {
    //Draw fluxes.  Will need to do bkgds too, later
    if(lastChar == '(' || lastChar == ')' || lastChar == '!')
      tempFilename = fileToWriteTo.substr(0,fileToWriteTo.size()-1);
    else
      tempFilename = fileToWriteTo;

    Double_t histoMin = 1;
    Double_t fluxEvents;
    Double_t *fluxEventsPtr = &fluxEvents;
    for(int i = 0; i < fluxes.size(); i++) {
      fluxHisto[i] = DrawFlux(i,axisToDraw,fluxEventsPtr);
      if(axisToDraw == 0) {
	outputCSV << fluxes[i]->GetName() << "," << fluxEvents << endl;
      }
      if(histoMin > fluxHisto[i]->GetBinContent(fluxHisto[i]->GetMinimumBin()))
	histoMin =  fluxHisto[i]->GetBinContent(fluxHisto[i]->GetMinimumBin());
    }
    histoMin = histoMin/1.259; //Adjust one tick on log scale
    
    dataHisto = new TH1D(name.c_str(),name.c_str(),4,0,1);
    mcHisto = new TH1D("mcHisto","mcHisto",4,0,1);
    vector<Double_t> axisVec;
    if(axisToDraw == 0 && (lastChar == '(' || lastChar == '!'))
      tempFilename = tempFilename + "(";
    if(axisToDraw == 2 && (lastChar == ')' || lastChar == '!'))
      tempFilename = tempFilename + ")";

    //cout << "tempFilename: " << tempFilename << " for input name ";
    //cout << fileToWriteTo << endl;

    if(axisToDraw == 0) {
      axisVec = xAxisBins;
    } else if(axisToDraw == 1) {
      axisVec = yAxisBins;
    } else if(axisToDraw == 2) {
      axisVec = zAxisBins;
    }

    Double_t *binsArray = new Double_t[axisVec.size()];
    for(int i=0; i < axisVec.size(); i++) {
      binsArray[i] = axisVec[i];
    }
    dataHisto->GetXaxis()->Set(axisVec.size()-1, binsArray);
    dataHisto->GetXaxis()->SetTitle(axisNames[axisToDraw].c_str());
    dataHisto->Rebuild();

    mcHisto->GetXaxis()->Set(axisVec.size()-1, binsArray);
    mcHisto->GetXaxis()->SetTitle(axisNames[axisToDraw].c_str());
    mcHisto->Rebuild();

    for(int event=axisToDraw; event < 3*nDataPoints; event+=3) {
      //Fill data histo, organized as (xyzxyzxyzxyz)
      dataHisto->Fill(dataArray[event]);
    }

    mcHisto->Reset();
    for(int i=0; i < fluxes.size(); i++) {
      mcHisto->Add(fluxHisto[i]);
    }

    Double_t maxValue;
    maxValue = dataHisto->GetBinContent(dataHisto->GetMaximumBin());
    Double_t tempMax = mcHisto->GetBinContent(mcHisto->GetMaximumBin());
    if(tempMax > maxValue)
      maxValue = tempMax;

    mycanvas->Clear();
    mycanvas->cd();

    dataHisto->SetLineColor(1);
    dataHisto->SetMinimum(histoMin);
    dataHisto->SetMaximum(maxValue*1.1);
    dataHisto->Draw("P0 E1 X0 *H");
//     masterPdf->SetLineColor(2);
//     masterPdf->Draw("SAME");
    mcHisto->SetLineColor(2);
    mcHisto->Draw("SAME");
    for(int i=0; i < fluxes.size(); i++) {
      fluxHisto[i]->SetLineColor((i+2)%9+1);
      fluxHisto[i]->Draw("SAME");
    }

    //These define the boundaries of points in the graph
    //The graph runs from 0.1 to 0.9 in "TLegend" coordinates
    //Double_t plotXMin=axisVec[0], plotXMax=axisVec[axisVec.size()-1];
    //Double_t plotYMin=0, plotYMax=maxValue*1.1;
    
    cout << "TotalEvents = " << totalEvents << endl;

    Double_t xmin=0.9, xmax=0.99, ymin=0.6, ymax=0.9;
    
    TLegend *mylegend = new TLegend(xmin,ymin,xmax,ymax);
    mylegend->SetBorderSize(1);
    mylegend->AddEntry(dataHisto,"Data");
    mylegend->AddEntry(mcHisto,"MC");
    for(int i=0; i < fluxes.size(); i++)
      mylegend->AddEntry(fluxHisto[i],(fluxes[i]->GetName()).c_str());
    mylegend->Draw();

    //cout << tempFilename << endl;

    mycanvas->SaveAs(tempFilename.c_str());

    masterPdf->SetLineColor(kBlack);

    outputCSV.close();

    delete mylegend;
    delete dataHisto;
    delete mcHisto;
    for(int i=0; i < fluxes.size(); i++)
      delete fluxHisto[i];
    delete [] binsArray;

  }

  delete mycanvas;
  delete [] fluxHisto;

}

TH1D* Pdf3D::DrawFlux(Int_t fluxNumber, Int_t axisToDraw, 
		      Double_t *fluxCount) {

  //int nSys = systematics.size();
  Int_t fluxNum = fluxNumber;

  string fluxName = fluxes[fluxNum]->GetName();

  //Get axis right
  vector<Double_t> axisVec;
  if(axisToDraw == 0) {
    axisVec = xAxisBins;
    fluxName = fluxName + "_x";
  } else if(axisToDraw == 1) {
    axisVec = yAxisBins;
    fluxName = fluxName + "_y";
  } else if(axisToDraw == 2) {
    axisVec = zAxisBins;
    fluxName = fluxName + "_z";
  }

  TH1D *dataHisto = new TH1D(fluxName.c_str(),fluxName.c_str(),4,0,1);


  Double_t *binsArray = new Double_t[axisVec.size()];
  for(int i=0; i < axisVec.size(); i++) {
    binsArray[i] = axisVec[i];
  }
  dataHisto->GetXaxis()->Set(axisVec.size()-1, binsArray);
  dataHisto->GetXaxis()->SetTitle(axisNames[axisToDraw].c_str());
  dataHisto->Rebuild();

  //Set flux number in Sys, kludgy
  for(int i=0; i < systematics.size(); i++) {
    systematics[i]->SetFluxType(fluxes[fluxNum]->GetFluxType());
  }
  
  const vector<Double_t>& eventData = fluxes[fluxNum]->GetEvent(0);
  
  vector<Double_t> difference;
  int nFluxEvents = fluxes[fluxNum]->GetNEvents();
  Double_t weight = fluxes[fluxNum]->GetFluxSize() / fluxes[fluxNum]->GetTimesFlux();
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
    difference[weightLocation]=1;

    //Apply sys to event
    for(int sysNum = 0; sysNum < numActiveSys; sysNum++) {
      activeSys[sysNum]->Apply(eventData, difference);
    }
    for(int i=0; i < eventDataSize; i++)
      difference[i] += eventData[i];

    //Trim on all axes, needed to get sizes right
    if(difference[axisNums[0]] < xAxisBins[0] || 
       difference[axisNums[0]] > xAxisBins[xAxisBins.size()-1])
      continue;
    if(difference[axisNums[1]] < yAxisBins[0] || 
       difference[axisNums[1]] > yAxisBins[yAxisBins.size()-1])
      continue;
    if(difference[axisNums[2]] < zAxisBins[0] || 
       difference[axisNums[2]] > zAxisBins[zAxisBins.size()-1])
      continue;

    dataHisto->Fill(difference[axisNums[axisToDraw]],
		    weight*difference[weightLocation]);
  }
  
  Double_t totalFluxEvents = 0;

  for(int i=1; i <= dataHisto->GetXaxis()->GetNbins(); i++)
    totalFluxEvents += dataHisto->GetBinContent(i);

  //totalFluxEvents = dataHisto->Integral();


  cout << "Flux " << fluxes[fluxNum]->GetName() << " has ";
  cout << totalFluxEvents << " events\n";

  if(fluxCount != NULL)
    *fluxCount = totalFluxEvents;

  delete [] binsArray;
  
//   TCanvas temp("temp","temp");
//   temp.cd();
//   dataHisto->Draw();
//   temp.SaveAs((fluxName+".ps").c_str());

  return dataHisto;
  
}
