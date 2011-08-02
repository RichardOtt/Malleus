/******
A class that sets up and runs the Markov Chain Monte Carlo

This class very straightforward, and meant to be flexible - it only
calls CalcLikelihood, SetParameters and ReadConfig from PdfParent,
and doesn't interact directly with any other class except Errors.

It's really meant to be a singleton class, and I've disabled the
copy constructor (both to ensure this and because it'd be messy)

It's designed to read in a config file (I didn't make another way to
set it up, it'd be too complex), the details of which I'll put in
a readme for the whole program.

To use it:
Feed in a config file, either at creation time with
MCMC nameOfMCMC("configfile")
or later with ReadConfig

Then, when you're ready to run it, just call Run

If you'd prefer, you can have it take a single step (increment parameters,
calculate likelihood, decide whether to keep the new value, write to tree)
by calling TakeStep()

PrintState() will print out the step number, the chain length (unchanging),
and the current log likelihood and parameter values
******/

#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TRandom3.h"
#include "TMath.h"
#include "PdfParent.h"
#include "Pdf1D.h"
#include "Pdf3D.h"
#include "ConfigFile.h"
#include "Tools.h"
using namespace std;

#ifndef _MCMC_H_
#define _MCMC_H_

class MCMC {

 private:
  Int_t step;  //Current step
  Int_t chainLength;  //Total number of steps
  Int_t printFreq;  //How often to print current status to screen
  Int_t skipSteps;  //Drop the first skipSteps steps; burn-in; need for graphs?
  Int_t dynamicSteps; //Number of dynamic steps-size steps to take
  //Bool_t skip; 

  Int_t nPdfs;
  PdfParent **pdfs;
  TRandom3 *rand;

  TTree *resultsTree; //Tree to store results
  string outDir;  //Location of output files
  TFile *resultsFile;


  //Parameter info
  Double_t logLikelihood;
  vector<string> parNames;
  Int_t nPars;
  Double_t *pars;  //Parameters that are passed to pdfs
  Double_t *parSigmas;  //Step sizes for parameters
  vector<Double_t> parDynamicSigmas; //Step sizes when varied
  vector<Int_t> pureParNums; //Which pars are handled by MCMC directly
  vector<Double_t> pureParMeans;
  vector<Double_t> pureParSigmas; //Sigma for constraint calc
  Bool_t useAsymm;  //Using asymmetry (i.e. Day/Night)
  Bool_t saveProposed; //Include proposed values, default is true
  Bool_t saveUnvaried; //Include Width=-1 values, default is true
  vector<Sys*> asymmetries;
  vector<Sys*> logLikelihoodFormulas;
  vector<Bool_t> parHasMax;
  vector<Double_t> parMaxes;
  vector<Bool_t> parHasMin;
  vector<Double_t> parMins;


  //Temporary things that need to be recorded (i.e. proposed values)
  Double_t propLogLikelihood;
  Double_t *propPars;
  
  //Internal helper functions
  MCMC(const MCMC &old_MCMC); //Explictly disabling copy constructor
  string ParInfoToString(string firstStr, int firstInt, string secondStr,
			 int secondInt);
  Bool_t ReadParInfo(ConfigFile& config, vector<Double_t>& parInit,
		     vector<Double_t>& parSigmasExtract, Int_t pdfNum,
		     string parType, vector<string>& keysUsed);
  Bool_t ReadMCMCSys(ConfigFile &config, vector<string> keysUsed);
  Bool_t ReadLogLikelihoodFormulas(ConfigFile &config);

 public:
  //Setup stuff
  MCMC();
  MCMC(ConfigFile config);
  ~MCMC();
  Bool_t ReadConfig(ConfigFile config);
  void Initialize();
  Bool_t Initialize(ConfigFile& init);
  void TakeStep();
  void TakeDynamicStep(Int_t parNumber, Double_t gamma, 
		       Double_t sigmaMin, Double_t sigmaMax);
  void Run();
  void IncrementParameters();
  void ApplyAsymm();
  Double_t CalcParLogLikelihood();
  void PrintState();
  void PrintSetup();
  void PrintPdf(Int_t pdfNumber);
  void Draw(string outputFile);

};



#endif
