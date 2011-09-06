/*****
Systematics
*****/

#include "Sys.h"

Sys::Sys() {
  sysFunc = NULL;
  sysFormula = NULL;
  name = "";
  type = '\0';
  dataTarget = -1;
  useOriginalData = true;
  fluxType = -1;

}

Sys::~Sys() {

  if(sysFunc != NULL)
    delete sysFunc;

}

vector<string> Sys::Setup(string sysName, Double_t sysMean, Double_t sysSigma,
			  vector<string> mcmcParNames, 
			  vector<string> branchNames,
			  ConfigFile& config, string sysTitle) {
  //Returns list of names of parameters "used" by this sys,
  //i.e. to avoid double counting, multi-parameter sys take all
  //out of the list
  //Returns empty vector (and Error) if problem
  //sysTitle is the string identifying this Sys in the config file,
  //will look something like Pdf_1_Sys_0
  
  name = sysName;
  mean.push_back(sysMean);
  sigma.push_back(sysSigma);
  Int_t location = SearchStringVector(mcmcParNames, name);
  if(location == -1) {
    //This shouldn't ever happen
    Errors::AddError(name+" not in list of parameters");
  }
  parNums.push_back(location);
  pars.push_back(0); //So everything has the same size and is ready to go

  vector<string> usedPars;
  usedPars = ReadConfig(mcmcParNames, branchNames, config, sysTitle);

  if(usedPars.size() == 0)
    Errors::AddError("Error in sys "+name+": setup failed");
  
  return usedPars;

}

vector<string> Sys::ReadConfig(vector<string> mcmcParNames, 
			       vector<string> branchNames, ConfigFile& config, 
			       string sysTitle) {
  vector<string> neededNames;
  neededNames.push_back(name); //The Sys variable name
  branchNames.push_back("InternalUseWeight");
  Bool_t found = true;
  
  if(config.keyExists(sysTitle+"_Function")) {
    string funcName = config.read<string>(sysTitle+"_Function");
    sysFunc = Decider::GenerateFunctionFromString(funcName);
    if(sysFunc == NULL) {
      Errors::AddError("Error: Function "+funcName+" requested by Sys "+\
		       name+" not in FunctionDefs.h");
    } else {
      sysFunctionName = funcName;
    }
  } else {
    Errors::AddError("Error: "+sysTitle+"_Function not defined");
    found = false;
  }

  if(config.keyExists(sysTitle+"_Target")) {
    string targetName = config.read<string>(sysTitle+"_Target");
    dataTarget = SearchStringVector(branchNames, targetName);
    if(dataTarget == -1) {
      Errors::AddError("Error: "+sysTitle+"_Target not valid");
      found = false;
    }
  } else {
    Errors::AddError("Error: "+sysTitle+"_Target not defined");
    found = false;
  }

  if(!config.keyExists(sysTitle+"_UseMultiply"))
    cout << sysTitle << "_UseMultiply not defined, defaulting to false\n";
  useMultiply = config.read<bool>(sysTitle+"_UseMultiply", false);

  if(!config.keyExists(sysTitle+"_UseOriginalData"))
    cout << sysTitle << "_UseOriginalData not defined, defaulting to true\n";
  useOriginalData = config.read<bool>(sysTitle+"_UseOriginalData",true);

  int keyNum = 0;
  ostringstream ostr;
  string tempKey;
  ostr << sysTitle << "_AddFluxAffected_";
  ostr.width(2);
  ostr.fill('0');
  ostr << keyNum;
  tempKey = ostr.str();
  while(config.keyExists(tempKey)) {
    AddFluxAffected(config.read<int>(tempKey));
    keyNum++;
    tempKey.clear();
    ostr.str(tempKey);
    ostr << sysTitle << "_AddFluxAffected_";
    ostr << setw(2) << setfill('0') << keyNum;
    tempKey = ostr.str();
  }
  if(fluxesAffected.size() == 0) {
    cout << "No calls to AddFluxAffected for Sys " << name;
    cout << ", defaulting to affecting all fluxes\n";
  }

  keyNum = 0;
  tempKey.clear();
  ostr.str(tempKey);
  ostr << sysTitle << "_MCMCParameterValue_";
  ostr << setw(2) << setfill('0') << keyNum;
  tempKey = ostr.str();
  while(config.keyExists(tempKey)) {
    SetMCMCPar(config.read<string>(tempKey),mcmcParNames);
    keyNum++;
    tempKey.clear();
    ostr.str(tempKey);
    ostr << sysTitle << "_MCMCParameterValue_";
    ostr << setw(2) << setfill('0') << keyNum;
    tempKey = ostr.str();
  }

  keyNum = 0;
  tempKey.clear();
  ostr.str(tempKey);
  ostr << sysTitle << "_MCBranchValue_";
  ostr << setw(2) << setfill('0') << keyNum;
  tempKey = ostr.str();
  while(config.keyExists(tempKey)) {
    SetBranchPar(config.read<string>(tempKey),branchNames);
    keyNum++;
    tempKey.clear();
    ostr.str(tempKey);
    ostr << sysTitle << "_MCBranchValue_";
    ostr << setw(2) << setfill('0') << keyNum;
    tempKey = ostr.str();
  }

  return neededNames;

}

Bool_t Sys::AddPar(string sysName, Double_t sysMean, Double_t sysSigma,
		   vector<string> mcmcParNames) {
  //Includes extra parameters that this one is in charge of,
  //mostly for situations where something needs a covariance matrix

  pars.push_back(0);
  parNums.push_back(SearchStringVector(mcmcParNames,sysName));
  mean.push_back(sysMean);
  sigma.push_back(sysSigma);
  
  return true;
}

void Sys::SetParameters(Int_t nPars, Double_t *parameters) {
  //Takes pars from MCMC class high above and extracts out what it needs
  //This updates both pars and mcmcPars

  //nPars is reduntant, included for completeness.  Maybe I should add
  //a safety check to make sure nothing is asking for too large a parNum?

  Int_t parsTotal = pars.size();
  for(int i = 0; i < parsTotal; i++) {
    pars[i] = parameters[parNums[i]];
  }

  parsTotal = mcmcParNums.size();
  for(int i = 0; i < parsTotal; i++) {
    sysFunc->SetParameter(i, parameters[mcmcParNums[i]]);
  }

}

void Sys::Apply(const vector<Double_t>& data, vector<Double_t>& difference) {
  //Note that mcmcParNums stuff is updated by UpdatePars, so no
  //need to try to redo it here.  Assume it's updated.
  Int_t dataParsTotal = dataParNums.size();
  Int_t mcmcParsTotal = mcmcParNums.size();
  for(int i = 0; i < dataParsTotal; i++) {
    if(useOriginalData) {
      sysFunc->SetParameter(i+mcmcParsTotal, data[dataParNums[i]]);
    } else {
      sysFunc->SetParameter(i+mcmcParsTotal, 
			   data[dataParNums[i]]+difference[dataParNums[i]]);
    }
  }
  if(!useMultiply) {
    difference[dataTarget] += sysFunc->Eval();
  } else {
    difference[dataTarget] *= sysFunc->Eval();
  }

}

void Sys::ApplyFormula(const vector<Double_t>& data, 
		       vector<Double_t>& difference) {

  //Note that mcmcParNums stuff is updated by UpdatePars, so no
  //need to try to redo it here.  Assume it's updated.
  Int_t dataParsTotal = dataParNums.size();
  Int_t mcmcParsTotal = mcmcParNums.size();
  for(int i = 0; i < dataParsTotal; i++) {
    if(useOriginalData) {
      sysFormula->SetParameter(i+mcmcParsTotal, data[dataParNums[i]]);
    } else {
      sysFormula->SetParameter(i+mcmcParsTotal, 
			   data[dataParNums[i]]+difference[dataParNums[i]]);
    }
  }
  if(!useMultiply) {
    difference[dataTarget] += sysFormula->Eval(fluxType);
  } else {
    difference[dataTarget] *= sysFormula->Eval(fluxType);
  }

}

void Sys::ApplyFormulaOnce(Int_t nPars, Double_t *parameters) {
  //This is for when there is only one systematic, to simplify that
  //situation a bit.  Directly alters the data
  Int_t dataParsTotal = dataParNums.size();
  Int_t mcmcParsTotal = mcmcParNums.size();
  for(int i = 0; i < dataParsTotal; i++) {
      sysFormula->SetParameter(i+mcmcParsTotal, parameters[dataParNums[i]]);
  }

  if(!useMultiply) {
    parameters[dataTarget] += sysFormula->Eval(fluxType);
  } else {
    parameters[dataTarget] *= sysFormula->Eval(fluxType);
  }

}  



Double_t Sys::CalcLogLikelihood() {
  //This will need to be updated to include covariance matricies later

  Double_t logLikelihood = 0;

  Int_t nPars = pars.size();
  for(int i=0; i < nPars; i++) {
    if(sigma[i] <= 0)
      continue;

    logLikelihood -= (pars[i]-mean[i])*(pars[i]-mean[i])/2/sigma[i]/sigma[i];
    logLikelihood -= TMath::Log(sigma[i])/2;
  }
  return logLikelihood;
}

Double_t Sys::FormulaValue(const vector<Double_t>& data) {
  //Note that mcmcParNums stuff is updated by UpdatePars, so no
  //need to try to redo it here.  Assume it's updated.
  Int_t dataParsTotal = dataParNums.size();
  Int_t mcmcParsTotal = mcmcParNums.size();
  for(int i = 0; i < dataParsTotal; i++) {
    sysFormula->SetParameter(i+mcmcParsTotal, data[dataParNums[i]]);
  }
  return sysFormula->Eval(fluxType);

}

vector<string> Sys::LookupName(string nameToGet, vector<string> mcmcParNames,
			       vector<string> branchNames) {
  //Effectively a huge switch statement for pre-defined names
  //Entirely for my convenience, later will be controlled by config file
  //Note that the defined range of a TF1 may be a limitation, but it doesn't
  //seem to matter for the eval function, which is what I care about
  //Note: mcmcPars are first, then data (in terms of numbering for function)


  vector<string> neededNames;

  neededNames.push_back(nameToGet);
  Bool_t found = true;

  //This is for the extra weight branch on the data output vector,
  //No other class needs to know about it
  branchNames.push_back("InternalUseWeight");

  if (nameToGet == "XMean"){
    sysFunc = new AddConst();
    found &= LookupNameChecker("XShift",mcmcParNames,'p');
    dataTarget = SearchStringVector(branchNames,"X");
    useMultiply=false;
    useOriginalData = true;
  } else if (nameToGet == "YMean"){
    sysFunc = new AddConst();
    found &= LookupNameChecker("YShift",mcmcParNames,'p');
    dataTarget = SearchStringVector(branchNames,"Y");
    useMultiply=false;
    useOriginalData = true;
  } else if (nameToGet == "ZMean"){
    sysFunc = new AddConst();
    found &= LookupNameChecker("ZShift",mcmcParNames,'p');
    dataTarget = SearchStringVector(branchNames,"Z");
    useMultiply=false;
    useOriginalData = true;
  } else {
    //Name not found
    neededNames.clear();
  }

  if(found == false)
    neededNames.clear();

  return neededNames;

}

Int_t Sys::SearchStringVector(vector<string> parNameList, 
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

Bool_t Sys::LookupNameChecker(string testName, 
			     vector<string> testList, char typeOfName) {
    Int_t parNum = -1;
    string listType;
    if(typeOfName == 'p' || typeOfName == 'P') {
      // p = parameters, mcmcParNames
      listType = "parameter set";
    }
    if(typeOfName == 'd' || typeOfName == 'D') {
      // d = data, branchNames
      listType = "data";
    }
    parNum = SearchStringVector(testList, testName);
    if(parNum == -1) {
      Errors::AddError("Error in "+name+" Sys:" + testName + 
		       " not in " + listType);
    } else {
      if(typeOfName == 'p' || typeOfName == 'P') {
	mcmcParNums.push_back(parNum);
      }
      if(typeOfName == 'd' || typeOfName == 'D') {
	dataParNums.push_back(parNum);
      }
    }

    if(parNum == -1)
      return false;
    else
      return true;
}

void Sys::PrintState() {
  //Dump everything to screen, for debugging

  cout << "Name: " << name << endl;
  //  sysFunc->Print();
  if(sysFunc != NULL)
    cout << "sysFunction used: " << sysFunctionName << endl;
  cout << "-----";
  cout << "parNums:\n";
  for(int i=0; i < parNums.size(); i++)
    cout << i << ": " << parNums[i] << endl;
  cout << "pars:\n";
  for(int i=0; i < pars.size(); i++)
    cout << i << ": " << pars[i] << endl;
  cout << "mean:\n";
  for(int i=0; i < mean.size(); i++)
    cout << i << ": " << mean[i] << endl;
  cout << "sigma:\n";
  for(int i=0; i < sigma.size(); i++)
    cout << i << ": " << sigma[i] << endl;
  cout << "mcmcParNums:\n";
  for(int i=0; i < mcmcParNums.size(); i++)
    cout << i << ": " << mcmcParNums[i] << endl;
  cout << "dataParNums:\n";
  for(int i=0; i < dataParNums.size(); i++)
    cout << i << ": " << dataParNums[i] << endl;
  cout << "dataTarget: " << dataTarget << endl;
  cout << "useOriginalData: " << useOriginalData << endl;
  cout << "useMultiply: " << useMultiply << endl;
  cout << "fluxesAffected:\n";
  for(list<Int_t>::iterator flux=fluxesAffected.begin(); 
      flux != fluxesAffected.end(); flux++) {
    cout << *flux << endl;
  }
  
  
}

Bool_t Sys::SetMCMCPar(string parName, vector<string> mcmcParNames) {
  return LookupNameChecker(parName, mcmcParNames, 'p');
}

Bool_t Sys::SetBranchPar(string parName, vector<string> branchNames) {
  return LookupNameChecker(parName, branchNames, 'd');
}

Bool_t Sys::SetTarget(string parName, vector<string> branchNames) {
  dataTarget = SearchStringVector(branchNames, parName);
  if(dataTarget == -1) {
    Errors::AddError("Error in "+name+" Sys: "+parName+\
		     " not a valid branch to target");
    return false;
  }
  return true;
}

Bool_t Sys::SetTFormula(string formula) {
  if(sysFormula != NULL) {
    delete sysFormula;
    sysFormula = NULL;
  }
  sysFormula = new TFormula((name+"Formula").c_str(), formula.c_str());

  Bool_t succeeded = !sysFormula->Compile();
  if(!succeeded)
    Errors::AddError("Error: Sys formula has errors");

  return succeeded;

}

Bool_t Sys::CheckIntegrity() {
  //Simple function to make sure that a Sys has everything it needs, in
  //terms of parameters and a valid TFormula or RealFunction

  Bool_t succeeded = true;
  
  if((sysFunc == NULL) && (sysFormula == NULL)) {
    Errors::AddError("Error: Sys has neither formula or function");
    succeeded = false;
  }
  if((sysFunc != NULL) && (sysFormula != NULL)) {
    Errors::AddError("Error: Sys has both formula and function");
    succeeded = false;
  }

//   if(sysFormula != NULL) {
//     cout << sysFormula->GetTitle() << endl;
// //     if(sysFormula->Compile() != 0)
// //       succeeded = false;
//   }

  //Check ones with TFormulas
  if(sysFormula != NULL) 
    if(mcmcParNums.size() + dataParNums.size() != sysFormula->GetNpar()) {
      Int_t functionPars = mcmcParNums.size() + dataParNums.size();
      Int_t formulaPars = sysFormula->GetNpar();
      ostringstream errorText;
      errorText << "Error: Sys " << name << " has "; 
      errorText << functionPars << " pars, ";
      errorText << " formula " << sysFormula->GetTitle() << " expects ";
      errorText << formulaPars;
      Errors::AddError(errorText.str());
      succeeded = false;
    }

  //Check ones with RealFunctions
  if(sysFunc != NULL) {
    if(mcmcParNums.size() + dataParNums.size() != sysFunc->GetNPars()) {
      Int_t functionPars = mcmcParNums.size() + dataParNums.size();
      Int_t formulaPars = sysFormula->GetNpar();
      ostringstream errorText;
      errorText << "Error: Sys " << name << " has ";
      errorText << functionPars << " pars, ";
      //      errorText << " RealFunction " << functionName << " expects ";
      errorText << formulaPars << "expected";
      Errors::AddError(errorText.str());
      succeeded = false;
    }
    //Check target and parameters are reasonable
    
  }

  return succeeded;
}

void Sys::AddFluxAffected(Int_t fluxNumber) {
  fluxesAffected.push_back(fluxNumber);
  fluxesAffected.sort();
  fluxesAffected.unique();
}

Bool_t Sys::CheckIfAffected(Int_t fluxNumber) {
  //if none listed, that's the same as all
  if(fluxesAffected.size() == 0)
    return true;

  for(list<Int_t>::iterator flux=fluxesAffected.begin(); 
      flux != fluxesAffected.end(); flux++) {
    if(*flux == fluxNumber)
      return true;
  }
  return false;
}

Bool_t Sys::CheckIfAffected() {
  return CheckIfAffected(fluxType);
}

