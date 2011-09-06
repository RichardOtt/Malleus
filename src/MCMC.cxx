
/*****
Implementation of the Markov Chain Monte Carlo process.
See MCMC.h and readme.txt for info on how to use it
*****/

#include "MCMC.h"

MCMC::MCMC() {
  //Basic setup for empty MCMC

  step = 0;
  chainLength = 0;
  printFreq = 1;
  logLikelihood = -1e100;
  propLogLikelihood = 0;
  nPdfs = 0;
  pdfs = NULL;
  pars = NULL;
  parSigmas = NULL;
  propPars = NULL;
  //propPdfs = NULL;
  outDir = ".";
  useAsymm = false;
  
  resultsFile = NULL;
  //Random number with random seed
  rand = new TRandom3(0);

}

MCMC::MCMC(ConfigFile config) {
  //Setup including read-in of config file

  step = 0;
  chainLength = 0;
  printFreq = 1;
  logLikelihood = 0;
  propLogLikelihood = 0;
  nPdfs = 0;
  pdfs = NULL;
  pars = NULL;
  parSigmas = NULL;
  propPars = NULL;
  //propPdfs = NULL;
  outDir = ".";
  useAsymm = false;

  resultsFile = NULL;
  //Random number with random seed
  rand = new TRandom3(0);

  if(!ReadConfig(config)) {
    Errors::Exit("Config file setup for MCMC failed");
  }
}

MCMC::~MCMC() {

  //Rely on vector destructor to clean that part up
  //Clean up allocated memory

  resultsFile->Close();

  for(int i=0; i < nPdfs; i++) {
    if(pdfs[i] != NULL)
      delete pdfs[i];
  }
  delete [] pdfs;

  //delete resultsTree;

  delete [] pars;
  delete [] parSigmas;
  delete [] propPars;

  delete rand;
  delete resultsFile;

}

Bool_t MCMC::ReadConfig(ConfigFile config) {
  //Reads in information relevant to MCMC from config file
  //Also sets up and initializes those parts that need it
  //Returns true if successful, false if problems occurred
  Bool_t succeeded = true;

  //Extract out actual MCMC settings
  chainLength = config.read<int>("MCMC_ChainLength",-1);
  if(chainLength < 1) {
    cout << "MCMC_ChainLength either < 1 or ";
    cout << "not in config file, defaulting to 1000\n";
    chainLength = 1000;
  }
  printFreq = config.read<int>("MCMC_PrintFrequency",-1);
  if(printFreq < 1) {
    cout << "MCMC_PrintFrequency either < 1 or ";
    cout << "not in config file, defaulting to 1\n";
    printFreq = 1;
  }
  skipSteps = config.read<int>("MCMC_SkipSteps",-1);
  if(skipSteps < 0) {
    cout << "MCMC_SkipSteps either negative or ";
    cout << "not in config file, defaulting to 0\n";
    skipSteps = 0;
  }
  useAsymm = config.read<bool>("MCMC_UseAsymmetry",false);
  if(useAsymm == false)
    cout << "MCMC_UseAsymmetry false or not present, not using asymmetry\n";

  dynamicSteps = config.read<int>("MCMC_DynamicSteps",0);
  if(dynamicSteps == 0)
    cout << "MCMC_DyanamicSteps zero or not present, not using dynamic steps\n";

  saveProposed = config.read<bool>("MCMC_SaveProposed",true);
  if(saveProposed == true)
    cout << "MCMC_SaveProposed true or not present, including Pro values in Tmcmc\n";

  saveUnvaried = config.read<bool>("MCMC_SaveUnvaried",true);
  if(saveUnvaried == true)
    cout << "MCMC_SaveUnvaried true or not present, including Width=-1 values in Tmcmc\n";

  //Seed random numbers, they're set-up in the constructor by default
  int seed = config.read<int>("RandomSeed",0);
  if(seed == 0) {
    cout << "Using random seed for random number generator\n";
  } else {
    rand->SetSeed(seed);
  }

  //Get number of pdfs, insist on zero-based counting and no skips
  int n = 0;
  string pdfdim;
  ostringstream ostr;
  ostr << "Pdf_" << n << "_Dimension";
  pdfdim = ostr.str();
  while(config.keyExists(pdfdim)) {
    n++;
    //Reset string
    pdfdim.clear();
    ostr.clear();
    ostr.str(pdfdim);
    ostr << "Pdf_" << n << "_Dimension";
    pdfdim = ostr.str();
  }
  nPdfs = n;
  if(nPdfs == 0) {
    Errors::AddError("Pdf_0_Dimension not defined.  "
		     "Please use zero-based counting");
  } else {
    //Define pdfs, setup later
    int dim;
    pdfs = new PdfParent*[nPdfs];
    //propPdfs = new PdfParent*[nPdfs];
    for(n = 0; n < nPdfs; n++) {
      pdfdim.clear();
      ostr.clear();
      ostr.str(pdfdim);
      ostr << "Pdf_" << n << "_Dimension";
      pdfdim = ostr.str();
      dim = config.read<int>(pdfdim);
      if(dim == 1) {
	pdfs[n] = new Pdf1D;
      } else if(dim == 3) {
	pdfs[n] = new Pdf3D;
      } else {
	//Only have 1D and 3D pdfs right now
	pdfs[n] = NULL;
	Errors::AddError("Error: " + pdfdim + " must be 1 or 3");
      }
    }
  }

  //Extract out parameters & sigmas, probably in a vector
  //that gets turned in to an array at the end
  vector<Double_t> parInit;
  vector<Double_t> parSigmasExtract;
  vector<string> keysUsed; //For Asymmetries later
  //Faster to do each pdf in order, rather than fluxes, then sys, then bkgd?
  for(n=0; n < nPdfs; n++) {
    succeeded &= ReadParInfo(config, parInit, parSigmasExtract, n, "Flux",
			     keysUsed);
  }
  for(n=0; n < nPdfs; n++) {
    succeeded &= ReadParInfo(config, parInit, parSigmasExtract, n, "Sys",
			     keysUsed);
  }
  for(n=0; n < nPdfs; n++) {
    succeeded &= ReadParInfo(config, parInit, parSigmasExtract, n, "Bkgd",
			     keysUsed);
  }
  //Read "pure" parameters
  succeeded &= ReadParInfo(config, parInit, parSigmasExtract, -1, "",
			   keysUsed);

  //Read Asymm definitions, if needed
  if(useAsymm)
    succeeded &= ReadMCMCSys(config, keysUsed);


  cout << "Starting ReadLogLikelihoodFormulas\n";
  cout.flush();
  succeeded &= ReadLogLikelihoodFormulas(config);

  //Debug thing:
  if(parNames.size() != parInit.size() || 
     parNames.size() != parSigmasExtract.size())
    Errors::Exit("Something went really wrong!  Num of parNames != "
		 "Num of parInits or Num of parSigmas");

  //Copy things in to their long-term storage locations
  nPars = parNames.size();
  pars = new Double_t[nPars];
  parSigmas = new Double_t[nPars];
  for(n=0; n < nPars; n++) {
    pars[n] = parInit[n];
    parSigmas[n] = parSigmasExtract[n];
  }

  //Should set initial proposed values to current values, just to be safe
  propPars = new Double_t[nPars];
  for(n=0; n < nPars; n++)
    propPars[n] = pars[n];


  //Set up pdfs
  for(n=0; n < nPdfs; n++) {
    succeeded = true;
    string pdfName;
    pdfName.clear();
    ostr.clear();
    ostr.str(pdfName);
    ostr << "Pdf_" << n;
    pdfName = ostr.str();
    //If it's NULL, there's already a problem, so skip it
    if(pdfs[n] != NULL)
      succeeded = (pdfs[n])->ReadConfig(config, n, parNames);
    if(!succeeded)
      Errors::AddError("Error: setup of " + pdfName + " failed");
  }


  //Setup output file and output tree
  if(!config.keyExists("OutputDirectory")) {
    cout << "OutputDirectory not in config file, defaulting to current dir\n";
  } else {
    outDir = config.read<string>("OutputDirectory");
  }
  if(!config.keyExists("OutputFilename")) {
    cout << "OutputFilename not in config file, defaulting to results.root\n";
  }
  string filename = config.read<string>("OutputFilename","results.root");
  filename = outDir + "/" + filename;
  resultsFile = new TFile(filename.c_str(), "RECREATE");
  if(resultsFile->IsZombie()) {
    Errors::AddError("Error: "+filename+" didn't open properly");
  } else {
    resultsFile->cd();
  }
  //Following Blair's tree format, seems reasonable
  resultsTree = new TTree("Tmcmc","MCMC results");
  resultsTree->Branch("Step",&step,"Step/I");
  resultsTree->Branch("ProLogL",&propLogLikelihood,"ProLogL/D");
  resultsTree->Branch("AccLogL",&logLikelihood,"AccLogL/D");
  for(n=0; n < nPars; n++) {
    if(saveUnvaried == false && parSigmas[n] == -1)
      continue;
    //A little messy, setting up a branch named "ProParName" of type double
    if(saveProposed) {
      resultsTree->Branch(("Pro"+parNames[n]).c_str(),&(propPars[n]),
			  ("Pro"+parNames[n]+"/D").c_str());
    }
    //A little messy, setting up a branch named "AccParName" of type double
    resultsTree->Branch(("Acc"+parNames[n]).c_str(),&(pars[n]),
			("Acc"+parNames[n]+"/D").c_str());
    if(dynamicSteps > 0) {
      resultsTree->Branch(("Sigma"+parNames[n]).c_str(),&(parSigmas[n]),
			  ("Sigma"+parNames[n]+"/D").c_str());
    }

  }

  //Debugging
  cout << "ReadConfig in MCMC finished!\n";

  //Return false if errors setting up, true otherwise
  if(Errors::GetNErrors() > 0)
    succeeded = false;

  return succeeded;
}

void MCMC::TakeStep() {
  //This takes one step in the MCMC
  //Run will, unsurprisingly, iterate over this

  //Create proposal pars
  IncrementParameters();

  propLogLikelihood = 0;
  propLogLikelihood += CalcParLogLikelihood();

  if(propLogLikelihood > -1e199) {
    for(int i = 0; i < nPdfs; i++) {
      (pdfs[i])->SetParameters(nPars, propPars);
      //The pdfs should keep track of any penalties for
      //stepping out of bounds
      propLogLikelihood += (pdfs[i])->CalcLogLikelihood();
      //cout << "Current propLogLikelihood = " << propLogLikelihood << endl;
    }
  }
  
  Double_t probAccept = TMath::Exp(propLogLikelihood - logLikelihood);
  //cout << "probAccept = " << probAccept << endl;
  probAccept = (probAccept>1) ? 1 : probAccept;
  
  if(rand->Rndm() < probAccept) {
    //Accept step
    for(int i = 0; i < nPars; i++) {
      pars[i] = propPars[i];
    }
    logLikelihood = propLogLikelihood;
  }

  resultsFile->cd();
  resultsTree->Fill();

  step++;
}

void MCMC::TakeDynamicStep(Int_t parNumber, Double_t gamma, 
			   Double_t sigmaMin, Double_t sigmaMax) {
  //This takes one step in the MCMC, with variable step size
  //Run will, unsurprisingly, iterate over this

  //Create proposal pars
  IncrementParameters();


  propLogLikelihood = 0;
  propLogLikelihood += CalcParLogLikelihood();

  //cout << "Penalties for pure pars give " << propLogLikelihood << endl;

  if(propLogLikelihood > -1e199) {
    for(int i = 0; i < nPdfs; i++) {
      (pdfs[i])->SetParameters(nPars, propPars);
      //The pdfs should keep track of any penalties for
      //stepping out of bounds
      propLogLikelihood += (pdfs[i])->CalcLogLikelihood();
    }
  }
  
  Double_t probAccept = TMath::Exp(propLogLikelihood - logLikelihood);
  probAccept = (probAccept>1) ? 1 : probAccept;
  
  if(rand->Rndm() < probAccept) {
    //Accept step
    for(int i = 0; i < nPars; i++) {
      pars[i] = propPars[i];
      logLikelihood = propLogLikelihood;
    }
  }

  //Dynamic part: adjust sigma for parNumber, if sigma[parNumber] > 0
  if(parSigmas[parNumber] > 0) {
    Double_t newSigma = parSigmas[parNumber] + gamma*(probAccept - 0.234);
    if(newSigma < sigmaMin)
      newSigma = sigmaMin;
    if(newSigma > sigmaMax)
      newSigma = sigmaMax;
    parSigmas[parNumber] = newSigma;
  }
  
  resultsFile->cd();
  resultsTree->Fill();

  step++;  
}

void MCMC::Initialize() {
  logLikelihood = 0;
  //Apply asymmetry terms
  if(useAsymm == true) {
    ApplyAsymm();
  }

  logLikelihood += CalcParLogLikelihood();

  for(int i=0; i < nPars; i++) {
    pars[i] = propPars[i];
    parDynamicSigmas.push_back(parSigmas[i]);
  }

  for(int i=0; i < nPdfs; i++) {
    (pdfs[i])->SetParameters(nPars, pars);
  }

  for(int i=0; i < nPdfs; i++)
    logLikelihood += pdfs[i]->CalcLogLikelihood();

}

void MCMC::Run() {

  //Initialize
  Initialize();

  //Set up the stuff you need for dealing with dynamic steps
  //Need a set of maxes and mins and a set of gammas, all of
  //which are derived from the initial set of sigmas.  Probably
  //simplest just to preserve the initial sigmas
  vector<Double_t> initialSigmas;
  for(int i=0; i < nPars; i++)
    initialSigmas.push_back(parSigmas[i]);    
  Int_t currentParNum=0;
  vector<Int_t> sigmasToVary;
  for(int i=0; i < nPars; i++)
    if(parSigmas[i] > 0)
      sigmasToVary.push_back(i);
  cout << "Pars:\n";
  for(int i=0; i < nPars; i++) {
    cout << i << ": " << parNames[i] << endl;
  }
  cout << endl;
  cout << "Pars with varied Sigma:\n";
  for(int i=0; i < sigmasToVary.size(); i++) {
    cout << sigmasToVary[i] << endl;
  }
  cout << endl;


  while(step < dynamicSteps) {
    Int_t currentPar = sigmasToVary[currentParNum];
    Double_t sigmaInit = initialSigmas[currentPar];
    Double_t gamma;
    if(step > sigmasToVary.size())
      gamma = sigmaInit/(TMath::Power(step,0.66));
    else
      gamma = sigmaInit/(TMath::Power(sigmasToVary.size(),0.66));
    TakeDynamicStep(currentPar,gamma,
		    sigmaInit/100,sigmaInit*100);
    currentParNum++;
    if(currentParNum == sigmasToVary.size()) {
      Tools::VectorScramble(sigmasToVary);
      currentParNum = 0;
    }
    if(step % printFreq == 0) {
      PrintState();
      resultsFile->cd();
      resultsTree->AutoSave();
    }
  }

  while(step < chainLength) {
    TakeStep();
    //cout << "step " << step << " taken\n";
    if(step % printFreq == 0) {
      PrintState();
      resultsFile->cd();
      resultsTree->AutoSave();
    }
  }
  resultsTree->Write();

  //Can add any post-processing stuff here, Blair made graphs
}

void MCMC::IncrementParameters() {
  //Creates new proposal parameters

  //Negative sigma == no updating (i.e. fixed)
  for(int i = 0; i < nPars; i++) {
    if(parSigmas[i] > 0) {
      propPars[i] = pars[i] + rand->Gaus(0,parSigmas[i]);
    } else {
      propPars[i] = pars[i];
    }
  }

  //Apply asymmetry terms
  if(useAsymm == true) {
    ApplyAsymm();
  }

}

void MCMC::ApplyAsymm() {
  vector<Double_t> data(nPars,0);
  vector<Double_t> difference(nPars,0);
  for(int i=0; i < nPars; i++)
    data[i] = propPars[i];

  for(int i=0; i < asymmetries.size(); i++) {
    if(asymmetries[i] != NULL) {
//       cout << "Applied asymmetry " << i << " to " << parNames[i] << endl;
//       cout << "Started with value " << propPars[i] << ", ended with value ";
      asymmetries[i]->ApplyFormula(data, difference);
      //      cout << propPars[i] + difference[i] << endl;
    }
  }
  
  for(int i=0; i < nPars; i++)
    propPars[i] += difference[i];
  
}

Double_t MCMC::CalcParLogLikelihood() {
  Double_t penalty = -1e200;
  for(int i=0; i < nPars; i++) {
    if(parHasMin[i] && propPars[i] < parMins[i])
      return penalty;
    if(parHasMax[i] && propPars[i] > parMaxes[i])
      return penalty;
  }

  Double_t pureParLogLikelihood = 0;
  Double_t sigma, mean, value;
  for(int i=0; i < pureParMeans.size(); i++) {
    mean = pureParMeans[i];
    sigma = pureParSigmas[i];
    value = propPars[pureParNums[i]];
    if(pureParSigmas[i] > 0) {
      pureParLogLikelihood -= (value-mean)*(value-mean)/2/sigma/sigma;
      //cout << "Added penalty for par " << parNames[pureParNums[i]];
      //cout << " of size " << pureParLogLikelihood << endl;
      //pureParLogLikelihood -= TMath::Log(sigma)/2;
    }
  }

  //Add in LogLikelihoodFormulas
  vector<Double_t> data(nPars,0);
  for(int i=0; i < nPars; i++)
    data[i] = propPars[i];
  for(int i=0; i < logLikelihoodFormulas.size(); i++) {
    pureParLogLikelihood += logLikelihoodFormulas[i]->FormulaValue(data);
  }


  return pureParLogLikelihood;
}

void MCMC::PrintState() {
  cout << "--------------------------------------------\n";
  cout << "Printing state of MCMC:\n";
  cout << "step: " << step << endl;
  cout << "chainLength: " << chainLength << endl;
  cout << "logLikelihood: " << logLikelihood << endl;
  cout << "Parameters:\n";
  for(int i = 0; i < nPars; i++) {
    cout << parNames[i] << ": ";
    cout << pars[i] << endl;
  }
  cout << "--------------------------------------------\n";

}

void MCMC::PrintSetup() {
  cout << "\n--------------------------------------------\n";
  cout << "Printing setup of MCMC:\n";
  cout << "chainLength: " << chainLength << endl;
  cout << "printFreq: " << printFreq << endl;
  cout << "skipSteps: " << skipSteps << endl;
  cout << "dynamicSteps: " << dynamicSteps << endl;
  cout << "nPdfs: " << nPdfs << endl;
  cout << "nPars: " << nPars << endl;
  cout << "Parameters: value, width, hasMax, max, hasMin, min\n";
  for(int i = 0; i < nPars; i++) {
    cout << "(" << i << ") " << parNames[i] << ": ";
    cout << pars[i] << ", " << parSigmas[i] << ", " << parHasMax[i] << ", ";
    cout << parMaxes[i] << ", " << parHasMin[i] << ", " << parMins[i] << endl;
  }
  cout << endl;
  cout << "PurePars: number, mean, sigma\n";
  for(int i=0; i < pureParNums.size(); i++) {
    cout << parNames[pureParNums[i]] << ": " << pureParNums[i] << ", ";
    cout << pureParMeans[i] << ", " << pureParSigmas[i] << endl;
  }
  cout << endl;
  cout << "Pdf states:\n";
  for(int i=0; i < nPdfs; i++)
    PrintPdf(i);
  cout << "--------------------------------------------\n\n";

}

void MCMC::PrintPdf(Int_t pdfNumber) {
  pdfs[pdfNumber]->PrintState();
}

string MCMC::ParInfoToString(string firstStr, int firstInt, string secondStr,
		       int secondInt) {

  //Outputs a string in the format of the config file:
  //example: ParInfoToString("Pdf_",1,"_Sys_",2) returns
  //  Pdf_1_Sys_02   as a string

  string output;
  ostringstream ostr;

  ostr.width(1);
  ostr << firstStr;
  if(firstInt >= 0)
    ostr << firstInt;
  ostr << secondStr;
  ostr.width(2);
  ostr.fill('0');
  ostr << secondInt;
  
  return ostr.str();
  
}

Bool_t MCMC::ReadParInfo(ConfigFile& config, vector<double>& parInit,
			 vector<double>& parSigmasExtract, Int_t pdfNum,
			 string parType, vector<string>& keysUsed) {
  //Reads in parameters (sys, flux, bkgd) from config file
  //It looks for Pdf_pdfNum_parType_xx, where xx increments from 00
  //until it runs out.  Also note that parType should be "Sys",
  //"Flux" or "Bkgd" or blank (if it's a Parameter)

  parType = "_" + parType + "_";
  string parInfo;
  int sysNum = 0;
  Bool_t succeeded = true;
  parInfo = ParInfoToString("Pdf_",pdfNum,parType,sysNum);
  if(pdfNum == -1) {
    parInfo = ParInfoToString("Parameter_",pdfNum,"",sysNum);
  }
  while(config.keyExists(parInfo+"_Name")) {
    cout << "Getting " << parInfo << endl;
    //If it isn't already in the parNames list, add it
    int i=0;
    string tempName;
    tempName = config.read<string>(parInfo+"_Name");
    cout << "Found " << tempName << endl;
    for(i = 0; i < parNames.size(); i++)
      if(parNames[i] == tempName)
	break;
    if(i == parNames.size()) {
      parNames.push_back(tempName);
      keysUsed.push_back(parInfo);  //For Asymm extraction later
      if(pdfNum == -1) {
	pureParNums.push_back(parNames.size()-1);
	//Read in and store mean, sigma
	if(config.keyExists(parInfo+"_Mean") !=
	   config.keyExists(parInfo+"_Sigma")) {
	  Errors::AddError("Error: "+parInfo+
			   " has one of _Mean and _Sigma, needs both or neither");
	  pureParMeans.push_back(0);
	  pureParSigmas.push_back(-1);
	} else {
	  pureParMeans.push_back(config.read<double>(parInfo+"_Mean",0));
	  pureParSigmas.push_back(config.read<double>(parInfo+"_Sigma",-1));
	}

      }
      if(!config.keyExists(parInfo+"_Init")) {
	Errors::AddError("Error: "+parInfo+"_Init not defined");
	parInit.push_back(0);
	succeeded = false;
      } else {
	parInit.push_back(config.read<double>(parInfo+"_Init"));
      }
      //Default to no variation if _Width not defined
      if(!config.keyExists(parInfo+"_Width")) {
	cout << parInfo+"_Width " << "not defined, defaulting to ";
	cout << "no variation\n";
      }
      parSigmasExtract.push_back(config.read<double>(parInfo+"_Width",-1));
      if(config.keyExists(parInfo+"_Min")) {
	parHasMin.push_back(true);
	parMins.push_back(config.read<Double_t>(parInfo+"_Min"));
      } else {
	cout << parInfo << "_Min not defined, defaulting to no lower bound\n";
	parHasMin.push_back(false);
	parMins.push_back(0);
      }
      if(config.keyExists(parInfo+"_Max")) {
	parHasMax.push_back(true);
	parMaxes.push_back(config.read<Double_t>(parInfo+"_Max"));
      } else {
	cout << parInfo << "_Max not defined, defaulting to no upper bound\n";
	parHasMax.push_back(false);
	parMaxes.push_back(0);
      }

    } else {
      //Not allowing repeated par names for now, may upgrade later
      Errors::AddError("Error: "+parInfo+"_Name not unique");
      succeeded = false;
    }
    sysNum++;
    parInfo = ParInfoToString("Pdf_",pdfNum,parType,sysNum);
    if(pdfNum == -1) {
      parInfo = ParInfoToString("Parameter_",pdfNum,"",sysNum);
    }
  }

  return succeeded;

}

Bool_t MCMC::ReadMCMCSys(ConfigFile& config, vector<string> keysUsed) {

  cout << "Calling ReadMCMCSys\n";
  cout << "Contents of keysUsed:\n";
  for(int i=0; i < keysUsed.size(); i++)
    cout << keysUsed[i] << endl;

  Bool_t succeeded = true;

  for(int keyNum=0; keyNum < keysUsed.size(); keyNum++) {
    string parInfo = keysUsed[keyNum];
    cout << "Getting Asymm for " << parInfo << endl;

    //Extract Asymm definitions
    if(!config.keyExists(parInfo+"_AsymmFunc")) {
      asymmetries.push_back(NULL);
    } else {
      Sys *tempSys = new Sys();
      string tempFormula = config.read<string>(parInfo+"_AsymmFunc");
      if(tempSys->SetTFormula(tempFormula) == false) {
	succeeded = false;
	Errors::AddError("Error: "+parInfo+"_AsymmFunc not a valid TFormula");
      }
      //Read formula parameters
      Int_t funParNum = 0;
      string funParNumStr;
      ostringstream ostr;
      ostr << "_AsymmPar_";
      ostr.width(2);
      ostr.fill('0');
      ostr << funParNum;
      funParNumStr = ostr.str();
      while(config.keyExists(parInfo+funParNumStr)) {
	string tempParName = config.read<string>(parInfo+funParNumStr);
	succeeded &= tempSys->SetBranchPar(tempParName, parNames);
	    
	//Clear and reset string
	funParNumStr.clear();
	ostr.str(funParNumStr);
	funParNum++;
	ostr << "_AsymmPar_";
	ostr.width(2);
	ostr.fill('0');
	ostr << funParNum;
	funParNumStr = ostr.str();
      }
      string tempName = config.read<string>(parInfo+"_Name","");
      succeeded &= tempSys->SetTarget(tempName,parNames);
      tempSys->SetUseMultiply(config.read<bool>(parInfo+"_AsymmUseMultiply",false));
      //Let UseOriginalData default to true
				  
      if(tempSys->CheckIntegrity() == false) {
	Errors::AddError("Error: Asymmetry function for "+
			 tempName +
			 " is either badly formatted or missing parameters");
	succeeded = false;
      }
      asymmetries.push_back(tempSys);
    }
  }
  
  return succeeded;
  
}


Bool_t MCMC::ReadLogLikelihoodFormulas(ConfigFile& config) {

  Bool_t succeeded = true;
  Int_t formulaNum = 0;
  string parInfo;
  Sys *tempSys;

  parInfo = ParInfoToString("LogLikelihoodFormula_",-1,"",formulaNum);

  while(config.keyExists(parInfo+"_Formula")) {
    tempSys = new Sys();
    string tempFormula = config.read<string>(parInfo+"_Formula");
    if(tempSys->SetTFormula(tempFormula) == false) {
      succeeded = false;
      Errors::AddError("Error: "+parInfo+"_Formula not a valid TFormula");
    }
    //Read formula parameters
    Int_t funParNum = 0;
    string funParNumStr;
    ostringstream ostr;
    ostr << "_Par_";
    ostr.width(2);
    ostr.fill('0');
    ostr << funParNum;
    funParNumStr = ostr.str();
    while(config.keyExists(parInfo+funParNumStr)) {
      string tempParName = config.read<string>(parInfo+funParNumStr);
      succeeded &= tempSys->SetBranchPar(tempParName, parNames);
      
      //Clear and reset string
      funParNumStr.clear();
      ostr.str(funParNumStr);
      funParNum++;
      ostr << "_Par_";
      ostr.width(2);
      ostr.fill('0');
      ostr << funParNum;
      funParNumStr = ostr.str();
    }

    if(tempSys->CheckIntegrity() == false) {
      Errors::AddError("Error:  LogLikelihoodFormula "+ parInfo +
		       " is either badly formatted or missing parameters");
      succeeded = false;
    }
    logLikelihoodFormulas.push_back(tempSys);
    
    cout << "Finished with " << parInfo << endl;
    cout.flush();
    
    formulaNum++;
    parInfo = ParInfoToString("LogLikelihoodFormula_",-1,"",formulaNum);
  }

  return succeeded;
  
}




void MCMC::Draw(string outputFile) {
  
  string tempFilename;

  //The ! indicates to the pdf draw function that it is the only pdf
  if(nPdfs == 1)
    tempFilename = outputFile+"!";
  else
    tempFilename = outputFile+"(";

  (pdfs[0])->Draw(tempFilename);

  for(int i=1; i < nPdfs-1; i++)
    (pdfs[i])->Draw(outputFile);

  if(nPdfs > 1)
    (pdfs[nPdfs-1])->Draw(outputFile+")");

}

Bool_t MCMC::Initialize(ConfigFile& init) {

  Double_t value;
  Bool_t succeeded = true;

  for(int i=0; i < parNames.size(); i++) {
    if(!init.keyExists(parNames[i])) {
      Errors::AddError("Error: Parameter "+parNames[i]+\
			" is not in initialization file");
      pars[i] = 0;
      propPars[i] = 0;
      succeeded = false;
    } else {
      value = init.read<double>(parNames[i]);
      pars[i] = value;
      propPars[i] = value;
    }
  }

  //Apply asymmetry terms
  if(useAsymm == true) {
    ApplyAsymm();
  }

  for(int i=0; i < nPars; i++) {
    pars[i] = propPars[i];
  }

  for(int i=0; i < nPdfs; i++) {
    (pdfs[i])->SetParameters(nPars, pars);
  }

  for(int i=0; i < nPdfs; i++)
    logLikelihood += pdfs[i]->CalcLogLikelihood();


  if(succeeded == false) {
    return false;
  } else {
    Initialize();
    return true;
  }

}
