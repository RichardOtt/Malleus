
/*****
A class that keeps track of and implements systematics

Need to document how it works and how to use
*****/

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include "TROOT.h"
#include "TFormula.h"
#include "Errors.h"
#include "TMath.h"
#include "ConfigFile.h"
#include "FunctionDefs.h"
#include "RealFunction.h"
#include "Decider.h"
using namespace std;

#ifndef _SYS_H_
#define _SYS_H_

class Sys {

 private:
  string name;
  vector<Int_t> parNums; //where pars are in MCMC list of pars
  vector<Double_t> pars; //values it is in charge of
  RealFunction *sysFunc;
  TFormula *sysFormula;
  Char_t type;  //May need to do resolution separately
  vector<Double_t> mean;
  vector<Double_t> sigma; //For giving likelihood contribution
  vector<Int_t> mcmcParNums;  //Values it needs
  vector<Int_t> dataParNums;  //Values it needs
  Int_t dataTarget;
  Bool_t useOriginalData;
  Bool_t useMultiply;
  Int_t fluxType;
  list<Int_t> fluxesAffected;

  //Internal helper functions
  vector<string> LookupName(string nameToGet, vector<string> mcmcParNames,
			    vector<string> branchNames);
  Bool_t LookupNameChecker(string testName, 
				vector<string> testList, char typeOfName);

 public:
  Sys();
  ~Sys();
  vector<string> Setup(string sysName, Double_t sysMean, Double_t sysSigma,
		       vector<string> mcmcParNames, 
		       vector<string> branchNames, ConfigFile& config,
		       string sysTitle);
  vector<string> ReadConfig(vector<string> mcmcParNames, 
			    vector<string> branchNames, ConfigFile& config,
			    string sysTitle);
  Bool_t AddPar(string sysName, Double_t sysMean, Double_t sysSigma,
		vector<string> mcmcParNames);
  void SetParameters(Int_t nPars, Double_t *parameters);
  void Apply(const vector<Double_t>& data, vector<Double_t>& difference);
  void ApplyFormula(const vector<Double_t>& data, 
		    vector<Double_t>& difference);
  void ApplyFormulaOnce(Int_t nPars, Double_t *parameters);
  Double_t CalcLogLikelihood();
  Double_t FormulaValue(const vector<Double_t>& data);
  Int_t SearchStringVector(vector<string> parNameList, 
			   string targetName);
  void PrintState();
  void SetUseOriginalData(Bool_t toSet) {useOriginalData = toSet;};
  Bool_t GetUseOriginalData() { return useOriginalData; };
  void SetFluxType(Int_t fluxTypeIn) { fluxType = fluxTypeIn;};
  Int_t GetFluxType() { return fluxType; };
  string GetName() { return name; };
  void SetName(string nameToSet) { name = nameToSet; };
  Bool_t SetMCMCPar(string parName, vector<string> mcmcParNames);
  Bool_t SetBranchPar(string parName, vector<string> branchNames);
  Bool_t SetTarget(string parName, vector<string> branchNames);
  void SetUseMultiply(Bool_t use) { useMultiply = use; };
  Bool_t GetUseMultiply() { return useMultiply; };
  Bool_t SetTFormula(string formula);
  Bool_t CheckIntegrity();
  void AddFluxAffected(Int_t fluxNumber);
  Bool_t CheckIfAffected(Int_t fluxNumber);
  Bool_t CheckIfAffected();
  Bool_t AddMCMCParameterValue(string key);
  Bool_t AddMCBranchValue(string key);
};


#endif
