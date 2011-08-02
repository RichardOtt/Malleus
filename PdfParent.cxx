/*****

*****/

#include "PdfParent.h"

PdfParent::PdfParent() {

  pdfNum = -1;
  name = "";
  logLikelihood = 0;
  totalEvents = 0;
  //weightNum = -1;


}

PdfParent::~PdfParent() {
  for(int i = 0; i < fluxes.size(); i++)
    delete fluxes[i];

  for(int i=0; i < systematics.size(); i++)
    delete systematics[i];
}

void PdfParent::SetParameters(Int_t nPars, Double_t *parameters) {
  //Each Pdf will keep track of which parameters out of the list
  //it needs.  Technically, the nPars parameter is not needed,
  //as that will be worked out earlier.  The exit check is
  //just for safety, shouldn't occur

  //This function selects the parameters this Pdf needs and
  //updates them to their new values

  if(pdfNum == -1)
    Errors::Exit("Pdf " +name+ " not initialized");

  Int_t numPars = parNums.size();
  for(int i=0; i < numPars; i++) {
    //Should remove this check later for speed?
    if(parNums[i] > nPars)
      Errors::Exit("SetParameters asked for parameter outside of range "
		   "in pdf "+name);

    pars[i] = parameters[parNums[i]];
  }

  Int_t nSys = systematics.size();
  for(int i=0; i < nSys; i++)
    systematics[i]->SetParameters(nPars, parameters);

  nSys = fluxes.size();
  //Remember, fluxes are listed first in the pars list, and only need one num
  for(int i=0; i < nSys; i++)
    fluxes[i]->SetFluxSize(pars[i]);

  nSys = backgrounds.size();
  for(int i=0; i < nSys; i++)
    backgrounds[i]->SetParameters(nPars, parameters);
  
}

Bool_t PdfParent::ReadConfig(ConfigFile& config, Int_t pdfNumInput,
			     vector<string> parNamesMCMC) {
  pdfNum = pdfNumInput;
  Bool_t succeeded = true;

  ostringstream ostr;
  string pdfInfo;
  ostr << "Pdf_" << pdfNum;
  pdfInfo = ostr.str();

  name = config.read<string>(pdfInfo+"_Name","");
  if(name == "") {
    Errors::AddError("Error: "+pdfInfo+"_Name not defined");
    succeeded = false;
  }

  pdfDim = config.read<int>(pdfInfo+"_Dimension",-1);
  if(pdfDim == -1) {
    Errors::AddError("Error: "+pdfInfo+"_Dimension not defined");
    succeeded = false;
  }
  
  Int_t branchNum = 0;
  string branchName;
  //Extract branch names
  pdfInfo = ParInfoToString("Pdf_",pdfNum,"_MCBranch_",branchNum);
  while(config.keyExists(pdfInfo)) {
    branchName = config.read<string>(pdfInfo);
    mcBranches.push_back(branchName);
    branchNum++;
    pdfInfo = ParInfoToString("Pdf_",pdfNum,"_MCBranch_",branchNum);
  }
  if(mcBranches.size() == 0) {
    Errors::AddError("Error: "+pdfInfo+" not defined."
		     "Please use zero based counting");
    succeeded = false;
  }

  //Extract axes info, need for Bkgd
  succeeded &= ExtractAxes(config, parNamesMCMC);
  cout << "Extracted Axes: \n";
  for(int i=0; i < axisNames.size(); i++) {
    cout << "Axis " << i << ": " << axisNames[i] << endl;
  }

  succeeded &= ExtractFlux(config, parNamesMCMC);
  cout << "Extracted all Fluxes\n";

  succeeded &= ExtractSys(config, parNamesMCMC);
  cout << "Extracted all Systematics\n";

  succeeded &= ExtractBkgd(config, parNamesMCMC);
  cout << "Extracted all Backgrounds\n";

  //SetupPdf will call the Pdf1D or Pdf3D version, as appropriate
  //that will deal with data, etc.
  succeeded &= SetupPdf(config, parNamesMCMC);

  cout << "Finished with setup for Pdf " << name << endl;

  return succeeded;
}


vector<Double_t> PdfParent::GetParameters() {
  return pars;
}


string PdfParent::ParInfoToString(string firstStr, int firstInt, 
				  string secondStr, int secondInt) {

  //Outputs a string in the format of the config file:
  //example: ParInfoToString("Pdf_",1,"_Sys_",2) returns
  //  Pdf_1_Sys_02   as a string

  string output;
  ostringstream ostr;

  ostr.width(1);
  ostr << firstStr;
  ostr << firstInt;
  ostr << secondStr;
  ostr.width(2);
  ostr.fill('0');
  ostr << secondInt;
  
  return ostr.str();
  
}

Int_t PdfParent::SearchStringVector(vector<string> parNameList, 
				    string targetName) {
  //Looks through parNameList for targetName, returns
  //the entry number in the vector if found, -1 if not

  int length = parNameList.size();
  Int_t position = -1;
  int i;

  for(i = 0; i < length; i++) {
    if(parNameList[i] == targetName)
      break;
  }

  if(i < length)
    position = i;

  return position;
}

Bool_t PdfParent::ExtractFlux(ConfigFile& config, 
				vector<string> parNamesMCMC) {
  //Takes out flux info from config file, sets up fluxes
  //No need to really separate from ReadConfig, other than
  //making things look nicer
  Bool_t succeeded = true;

  Int_t fluxNum = 0;
  Int_t parNumber = -1;
  string fluxName;
  string pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Flux_", fluxNum);
  while(config.keyExists(pdfInfo+"_Name")) {
    fluxName = config.read<string>(pdfInfo+"_Name");
    parNames.push_back(fluxName);
    parNumber = SearchStringVector(parNamesMCMC, fluxName);
    //Debug
    if(parNumber == -1) {
      parNumber = 0;
      Errors::AddError(fluxName+" not in parameter list!  Impossible!");
    }
    parNums.push_back(parNumber);
    pars.push_back(0);

    //Get FluxNum, if defined, default to -1 (for not special)
    Int_t inputFluxNum = config.read<Int_t>(pdfInfo+"_FluxNumber",-1);

    //Get Max and Min, if defined
    if(config.keyExists(pdfInfo+"_Min")) {
      parHasMin.push_back(true);
      parMins.push_back(config.read<Double_t>(pdfInfo+"_Min"));
    } else {
      cout << pdfInfo << "_Min not defined, defaulting to no lower bound\n";
      parHasMin.push_back(false);
      parMins.push_back(0);
    }
    if(config.keyExists(pdfInfo+"_Max")) {
      parHasMax.push_back(true);
      parMaxes.push_back(config.read<Double_t>(pdfInfo+"_Max"));
    } else {
      cout << pdfInfo << "_Max not defined, defaulting to no upper bound\n";
      parHasMax.push_back(false);
      parMaxes.push_back(0);
    }
    
    
    if(!config.keyExists(pdfInfo+"_File")) {
      Errors::AddError("Error: "+pdfInfo+"_File not defined");
      return false;
    }
    string filename = config.read<string>(pdfInfo+"_File");
    TFile fluxFile(filename.c_str());
    if(fluxFile.IsZombie()) {
      Errors::AddError("Error: couldn't open "+filename);
      gROOT->cd();
      return false;
    }
    if(!config.keyExists(pdfInfo+"_Tree")) {
      Errors::AddError("Error: "+pdfInfo+"_Tree not defined");
      gROOT->cd();
      return false;
    }
    string treeName = config.read<string>(pdfInfo+"_Tree");

    TTree *fluxTree = dynamic_cast<TTree*>(fluxFile.Get(treeName.c_str()));
    if(fluxTree == NULL) {
      Errors::AddError("Error: couldn't find tree "+treeName+" in file "\
		      +filename);
      gROOT->cd();
      return false;
    }
    //Ok, have tree now

    if(!config.keyExists(pdfInfo+"_TimesExpected")) {
      cout << pdfInfo << "_TimesExpected not defined, defaulting to 1\n";
    }
    Double_t timesExpected = config.read<double>(pdfInfo+"_TimesExpected",1);

    if(!config.keyExists(pdfInfo+"_Sigma")) {
      cout << pdfInfo << "_Sigma not defined, defaulting to no constraint\n";
    }
    Double_t sigmaFlux = config.read<double>(pdfInfo+"_Sigma",-1);

    if(!config.keyExists(pdfInfo+"_Mean") && sigmaFlux > 0) {
      Errors::AddError("Error: "+pdfInfo+"_Mean not defined, but "+pdfInfo+\
		       "_Sigma > 0");
      succeeded = false;
    }
    Double_t meanFlux = config.read<double>(pdfInfo+"_Mean",0);

    Flux *thisFlux = new Flux();
    //thisFlux->Setup(fluxName, fluxMean, fluxSigma, parNamesMCMC, mcBranches);
    succeeded &= thisFlux->LoadData(fluxName, fluxTree, 
				    mcBranches, timesExpected, meanFlux, 
				    sigmaFlux);
    thisFlux->SetFluxType(inputFluxNum);
    fluxes.push_back(thisFlux);

    fluxFile.Close(); //This should delete the tree, too

    fluxNum++;
    pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Flux_", fluxNum);
  }

  return succeeded;
}

Bool_t PdfParent::ExtractSys(ConfigFile& config, 
				vector<string> parNamesMCMC) {
  //Takes out sys info from config file, sets up systematics
  //No need to really separate from ReadConfig, other than
  //making things look nicer

  Bool_t succeeded = true;

  //For multiple parameters
  vector<string> usedNames;
  vector<Int_t> usedNamesTargets;

  Int_t sysNum = 0;
  Int_t parNumber = -1;
  string sysName;
  string pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Sys_", sysNum);
  while(config.keyExists(pdfInfo+"_Name")) {
    sysName = config.read<string>(pdfInfo+"_Name");
    parNames.push_back(sysName);
    parNumber = SearchStringVector(parNamesMCMC, sysName);
    //Debug
    if(parNumber == -1) {
      parNumber = 0;
      Errors::AddError(sysName+" not in parameter list!  Impossible!");
    }
    parNums.push_back(parNumber);
    pars.push_back(0);


    //Get Max and Min, if defined
    if(config.keyExists(pdfInfo+"_Min")) {
      parHasMin.push_back(true);
      parMins.push_back(config.read<Double_t>(pdfInfo+"_Min"));
    } else {
      cout << pdfInfo << "_Min not defined, defaulting to no lower bound\n";
      parHasMin.push_back(false);
      parMins.push_back(0);
    }
    if(config.keyExists(pdfInfo+"_Max")) {
      parHasMax.push_back(true);
      parMaxes.push_back(config.read<Double_t>(pdfInfo+"_Max"));
    } else {
      cout << pdfInfo << "_Max not defined, defaulting to no upper bound\n";
      parHasMax.push_back(false);
      parMaxes.push_back(0);
    }


    if(!config.keyExists(pdfInfo+"_Mean") && \
       config.keyExists(pdfInfo+"_Sigma")) {
      Errors::AddError(pdfInfo+"_Mean not defined, but "+pdfInfo+"_Sigma is");
      succeeded = false;
      continue;
    }
    //So, either both mean and sigma defined, or sigma not defined
    if(!config.keyExists(pdfInfo+"_Sigma"))
      cout << pdfInfo << "_Sigma not defined, defaulting to no constraint\n";
    Double_t sysMean = config.read<double>(pdfInfo+"_Mean",0);
    Double_t sysSigma = config.read<double>(pdfInfo+"_Sigma",-1);
    if(!config.keyExists(pdfInfo+"_UseOriginalData"))
      cout << pdfInfo << "_UseOriginalData not defined, defaulting to true\n";
//     Bool_t useOriginalData = config.read<bool>(pdfInfo+"_UseOriginalData",
// 					       true);
    Int_t usedPosition = SearchStringVector(usedNames, sysName);
    if(usedPosition == -1) {
      //Not an already used name, set up
      vector<string> setupOutput;
      Sys *thisSys = new Sys();    
      //thisSys->SetUseOriginalData(useOriginalData); //Taken care of for SNO
      setupOutput = thisSys->Setup(sysName, sysMean, sysSigma, 
				   parNamesMCMC, mcBranches, config);
      systematics.push_back(thisSys);
      if(setupOutput.size() == 0) {
	Errors::AddError("Error: Setup for " +pdfInfo+ " failed");
      }
      if(setupOutput.size() > 1) {
	//Multiple parameters, add to list to deal with separately
	for(int i=1; i < setupOutput.size(); i++) {
	  usedNames.push_back(setupOutput[i]);
	  usedNamesTargets.push_back(systematics.size()-1);
	}
      }
    } else {
      //It's in the "used names" list, need to AddPar it to the appropriate sys
      //also, no need to delete it from the list since everything has
      //a unique name

      systematics[usedNamesTargets[usedPosition]]->AddPar(sysName, sysMean,
							  sysSigma, 
							  parNamesMCMC);
    }
      
    sysNum++;
    pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Sys_", sysNum);
    
  }
    
  return succeeded;

}

Bool_t PdfParent::ExtractBkgd(ConfigFile& config, 
				vector<string> parNamesMCMC) {
  //Takes out background info from config file, sets up backgrounds
  //No need to really separate from ReadConfig, other than
  //making things look nicer

  Bool_t succeeded = true;

  //For multiple parameters
  vector<string> usedNames;
  vector<Int_t> usedNamesTargets;

  Int_t sysNum = 0;
  Int_t parNumber = -1;
  string sysName;
  string pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Bkgd_", sysNum);
  while(config.keyExists(pdfInfo+"_Name")) {
    sysName = config.read<string>(pdfInfo+"_Name");
    parNames.push_back(sysName);
    parNumber = SearchStringVector(parNamesMCMC, sysName);
    //Debug
    if(parNumber == -1) {
      parNumber = 0;
      Errors::AddError(sysName+" not in parameter list!  Impossible!");
    }
    parNums.push_back(parNumber);
    pars.push_back(0);

    //Get Max and Min, if defined
    if(config.keyExists(pdfInfo+"_Min")) {
      parHasMin.push_back(true);
      parMins.push_back(config.read<Double_t>(pdfInfo+"_Min"));
    } else {
      cout << pdfInfo << "_Min not defined, defaulting to no lower bound\n";
      parHasMin.push_back(false);
      parMins.push_back(0);
    }
    if(config.keyExists(pdfInfo+"_Max")) {
      parHasMax.push_back(true);
      parMaxes.push_back(config.read<Double_t>(pdfInfo+"_Max"));
    } else {
      cout << pdfInfo << "_Max not defined, defaulting to no upper bound\n";
      parHasMax.push_back(false);
      parMaxes.push_back(0);
    }

    if(!config.keyExists(pdfInfo+"_Mean") && \
       config.keyExists(pdfInfo+"_Sigma")) {
      Errors::AddError(pdfInfo+"_Mean not defined, but "+pdfInfo+"_Sigma is");
      succeeded = false;
      continue;
    }


    //So, either both mean and sigma defined, or sigma not defined
    if(!config.keyExists(pdfInfo+"_Sigma"))
      cout << pdfInfo << "_Sigma not defined, defaulting to no constraint\n";
    Double_t sysMean = config.read<double>(pdfInfo+"_Mean",0);
    Double_t sysSigma = config.read<double>(pdfInfo+"_Sigma",-1);

    string bkgdFilename = config.read<string>(pdfInfo+"_File","");
    string bkgdHistoName = config.read<string>(pdfInfo+"_Histo","");

    Int_t usedPosition = SearchStringVector(usedNames, sysName);
    if(usedPosition == -1) {
      //Not an already used name, set up
      vector<string> setupOutput;

      //Axes only defined for first parameter in a multi-parameter setup
      vector<string> tempAxisNamesVec;
      string tempAxisName = config.read<string>(pdfInfo+"_xAxisName","");
      if(tempAxisName != "")
	tempAxisNamesVec.push_back(tempAxisName);
      tempAxisName = config.read<string>(pdfInfo+"_yAxisName","");
      if(tempAxisName != "")
	tempAxisNamesVec.push_back(tempAxisName);
      tempAxisName = config.read<string>(pdfInfo+"_zAxisName","");
      if(tempAxisName != "")
	tempAxisNamesVec.push_back(tempAxisName);

      cout << "Number of Axes for Bkgd: " << tempAxisNamesVec.size() << endl;

      Bkgd *thisBkgd = new Bkgd();    
      //thisSys->SetUseOriginalData(useOriginalData); //Taken care of for SNO
      setupOutput = thisBkgd->Setup(sysName, sysMean, sysSigma, 
				    parNamesMCMC, tempAxisNamesVec, 
				    axisNames, 
				    xAxisBins, yAxisBins, zAxisBins, 
				    bkgdFilename, bkgdHistoName);
      backgrounds.push_back(thisBkgd);
      if(setupOutput.size() == 0) {
	Errors::AddError("Error: Setup for " +pdfInfo+ " failed");
      }
      if(setupOutput.size() > 1) {
	//Multiple parameters, add to list to deal with separately
	for(int i=1; i < setupOutput.size(); i++) {
	  usedNames.push_back(setupOutput[i]);
	  usedNamesTargets.push_back(backgrounds.size()-1);
	}
      }
    } else {
      //It's in the "used names" list, need to AddPar it to the appropriate sys
      //also, no need to delete it from the list since everything has
      //a unique name

      backgrounds[usedNamesTargets[usedPosition]]->AddPar(sysName, sysMean,
							  sysSigma, 
							  parNamesMCMC);
    }
      
    sysNum++;
    pdfInfo = ParInfoToString("Pdf_", pdfNum, "_Bkgd_", sysNum);

  }
    
    return succeeded;

}

Bool_t PdfParent::ExtractAxes(ConfigFile& config, 
			      vector<string> parNamesMCMC) {
  Bool_t succeeded = true;
  ostringstream ostr;
  string axisInfo;
  string tempString;

  for(int i=0; i < pdfDim; i++) {

    ostr << "Pdf_" << pdfNum << "_Axis_" << i;
    axisInfo = ostr.str();


    //Get Name
    if(!config.keyExists(axisInfo+"_Name")) {
      Errors::AddError("Error: "+axisInfo+"_Name not defined");
      succeeded = false;
    }
    tempString = config.read<string>(axisInfo+"_Name","");
    axisNames.push_back(tempString);


    //Get Bins
    vector<Double_t> *binsVec = NULL;
    if(i == 0)
      binsVec = &xAxisBins;
    if(i == 1)
      binsVec = &yAxisBins;
    if(i == 2)
      binsVec = &zAxisBins;

    Int_t bin = 0;
    tempString.clear();
    ostr.str(tempString);
    ostr << "Pdf_" << pdfNum;
    tempString = ostr.str() + ParInfoToString("_Axis_",i,"_Bin",bin);
    //cout << tempString << endl;
    while(config.keyExists(tempString)) {
      binsVec->push_back(config.read<double>(tempString));
      bin++;
      tempString = ostr.str() + ParInfoToString("_Axis_",i,"_Bin",bin);
      //cout << tempString << endl;
    }
    
    if(binsVec->size() < 2) {
      Errors::AddError("Error in "+axisInfo+": must have at least one bin"\
		       " (two boundaries)");
      succeeded = false;
    }

    //Make sure bins are in increasing order
    bin = 0;
    //cout << "PdfBin" << bin << " = " << binsVec->at(bin) << endl;
    for(bin = 1; bin < binsVec->size(); bin++) {
      //cout << "PdfBin" << bin << " = " << binsVec->at(bin) << endl;
      if(binsVec->at(bin-1) >= binsVec->at(bin)) {
	//cout << binsVec->at(bin-1) << " >= " << binsVec->at(bin);
	//cout << " at bin " << bin << endl;
	Errors::AddError("Error in "+axisInfo+": bins not in numerical order");
	succeeded = false;
      }
    }

    string axisName = axisNames[i];
    Int_t position = SearchStringVector(mcBranches, axisName);
    if(position == -1) {
      //axisName not in the branches in the tree, abort
      Errors::AddError("Error: "+axisName+" not in MC Branches");
      cout << "position == -1 for " << axisName << endl;
      succeeded = false;
    } else {
      axisNums.push_back(position);
      cout << axisName << " found in mcBranches at position " << position << endl;
    }
    
    axisInfo.clear();
    ostr.str(axisInfo);
  }

  cout << "Axis Names at end of ExtractAxes: ";
  for(int i=0; i < pdfDim; i++)
    cout << axisNames[i] << ", ";
  cout << endl;

  return succeeded;
}
