
/**********************
This is a program that reads in a simplified config file,
and writes out the config file that rottSigEx.exe needs to run

usage: metaConfig.exe inFile outFile
It will read in inFile, which is the meta config file,
process it to a normal config file, then write that to outFile
**********************/

#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include "Errors.h"
#include "Tools.h"
using namespace std;

int stringToTokens(string toBreak, vector<string>& toFill);
bool isNumber(string toCheck);

int main(int argc, char *argv[]) {

  if(argc != 3) {
    cout << "usage:   metaConfig.exe inputFile outputFile\n";
    return 1;
  }

  string inFileName = argv[1];
  string outFileName = argv[2];

  //Test to make sure inFile exists
  ifstream inFile;
  inFile.open(inFileName.c_str());
  if(inFile.fail()) {
    cout << "Unable to open file " << inFileName << ", exiting\n";
    return 2;
  }

  //Test to make sure can write to outFile
  ofstream outFile;
  outFile.open(outFileName.c_str());
  if(outFile.fail()) {
    cout << "Unable to open file " << outFileName << ", exiting\n";
    return 2;
  }

  //Read in file.  Two types of commands - new and =
  //new <thing> creates a new <thing>, parameter=value sets parameter to value
  //MCMC parameters are just copied
  vector<string> MCMCcommands;
  MCMCcommands.push_back("MCMC_PrintFrequency");
  MCMCcommands.push_back("MCMC_ChainLength");
  MCMCcommands.push_back("MCMC_SkipSteps");
  MCMCcommands.push_back("MCMC_UseAsymmetry");
  MCMCcommands.push_back("MCMC_DynamicSteps");
  MCMCcommands.push_back("MCMC_SaveProposed");
  MCMCcommands.push_back("MCMC_SaveUnvaried");
  MCMCcommands.push_back("RandomSeed");
  MCMCcommands.push_back("OutputDirectory");
  MCMCcommands.push_back("OutputFilename");
  MCMCcommands.push_back("OutputTreeName");
  
  vector<string> validNew;
  validNew.push_back("Pdf");
  validNew.push_back("Sys");
  validNew.push_back("Bkgd");
  validNew.push_back("Flux");
  validNew.push_back("Axis");
  validNew.push_back("Parameter");
  validNew.push_back("LogLikelihoodFormula");

  vector<string> validPdf;
  validPdf.push_back("Name");
  validPdf.push_back("Dimension");
  validPdf.push_back("MCBranch");
  validPdf.push_back("DataFile");
  validPdf.push_back("DataTree");

  vector<string> validBkgd;
  validBkgd.push_back("Name");
  validBkgd.push_back("Min");
  validBkgd.push_back("Max");
  validBkgd.push_back("Sigma");
  validBkgd.push_back("Mean");
  validBkgd.push_back("Width");
  validBkgd.push_back("Init");
  validBkgd.push_back("File");
  validBkgd.push_back("Histo");
  validBkgd.push_back("xAxisName");
  validBkgd.push_back("yAxisName");
  validBkgd.push_back("zAxisName");
  validBkgd.push_back("AsymmFunc");
  validBkgd.push_back("AsymmUseMultiply");
  validBkgd.push_back("AsymmPar");

  vector<string> validSys;
  validSys.push_back("Name");
  validSys.push_back("Min");
  validSys.push_back("Max");
  validSys.push_back("Sigma");
  validSys.push_back("Mean");
  validSys.push_back("Width");
  validSys.push_back("Init");
  validSys.push_back("AsymmFunc");
  validSys.push_back("AsymmUseMultiply");
  validSys.push_back("AsymmPar");

  vector<string> validFlux;
  validFlux.push_back("Name");
  validFlux.push_back("Min");
  validFlux.push_back("Max");
  validFlux.push_back("File");
  validFlux.push_back("Tree");
  validFlux.push_back("TimesExpected");
  validFlux.push_back("Sigma");
  validFlux.push_back("Mean");
  validFlux.push_back("Width");
  validFlux.push_back("Init");
  validFlux.push_back("FluxNumber");
  validFlux.push_back("AsymmFunc");
  validFlux.push_back("AsymmUseMultiply");
  validFlux.push_back("AsymmPar");

  vector<string> validAxis;
  validAxis.push_back("Name");
  validAxis.push_back("Bin");

  vector<string> validParameter;
  validParameter.push_back("Name");
  validParameter.push_back("Init");
  validParameter.push_back("Width");
  validParameter.push_back("Mean");
  validParameter.push_back("Sigma");
  validParameter.push_back("Max");
  validParameter.push_back("Min");
  validParameter.push_back("AsymmFunc");
  validParameter.push_back("AsymmUseMultiply");
  validParameter.push_back("AsymmPar");

  vector<string> validLogLikelihoodFormula;
  validLogLikelihoodFormula.push_back("Formula");
  validLogLikelihoodFormula.push_back("Par");


  string line;
  int position = -1;
  vector<string> tokens;
  vector<string> names;
  vector<int> values;
  int pdfPosition = -1;
  int binNum=0, MCBranchNum=0, asymmParNum = 0, lLFParNum=0;
  double lastBin = 0;
  while(!inFile.eof()) {
    getline(inFile,line);
    //cout << "Read in line: " << line << endl;
    
    int nTokens = stringToTokens(line, tokens);

    //Now for the real work - evaluate the tokens, keep track of
    //everything as you go
    if(nTokens == 0)
      continue; //blank line
    if((tokens[0])[0] == '#')
      continue; //Comment, ignore

    //MCMC parameters, check all valid ones
    if(Tools::SearchStringVector(MCMCcommands,tokens[0]) != -1) {
      //It's an MCMC command, check formatting and copy
      if(nTokens < 3) {
	Errors::AddError(tokens[0]+" doesn't have enough parameters");
	continue;
      }
      if(nTokens > 3 && (tokens[3])[0] != '#') {
	Errors::AddError(tokens[0]+" has too many parameters");
	continue;
      }
      if(tokens[1] == "=") {
	outFile << tokens[0] << tokens[1] << tokens[2] << endl;
	continue;
      }
    }

    if(tokens[0] == "new" || tokens[0] == "New" || tokens[0] == "NEW") {
      //Everything lives under a pdf, so need to keep track of
      //current pdf, and what I'm dealing with (Sys, Axis, Bkgd, Flux)
      //Edit: Parameters do not live under a pdf, need to be read first
      //Edit: Same with LogLikelihoodFormulas
      if(nTokens < 2) {
	Errors::AddError(tokens[0]+" doesn't have enough parameters");
	continue;
      }
      if(nTokens > 2 && (tokens[2])[0] != '#') {
	Errors::AddError(tokens[0]+" has too many parameters");
	continue;
      }
      if(tokens[1] != "Pdf" && pdfPosition == -1 && tokens[1] != "Parameter" && tokens[1] != "LogLikelihoodFormula") {
	Errors::AddError("Fatal error: " + tokens[1]+\
			 " needs to have a Pdf defined first");
	Errors::Exit();
      }
      //Make sure it's a valid thing to be making a "new" one of
      if(Tools::SearchStringVector(validNew,tokens[1]) == -1) {
	Errors::AddError(tokens[1]+" is not a valid 'new' candidate");
	continue;
      }
      position = Tools::SearchStringVector(names,tokens[1]);
      if(position == -1) {
	names.push_back(tokens[1]);
	values.push_back(0);
	position = Tools::SearchStringVector(names,tokens[1]);
	if(tokens[1] == "Pdf")
	  pdfPosition = position;
      } else {
	values[position] += 1;
      }
      if(tokens[1] == "Pdf") {
	//Additionally, reset everything else in the stack to zero,
	//starting a new Pdf so new counting
	//Edit: Except the Parameters and LogLikelihoodFormulas!
	int tempInt = values[position];
	int parameterPosition = Tools::SearchStringVector(names,"Parameter");
	int lLFPosition = Tools::SearchStringVector(names,"LogLikelihoodFormula");
	for(int i=0; i < values.size(); i++) {
	  if(i == parameterPosition || i == lLFPosition)
	    continue;
	  values[i] = -1;
	}
	values[position] = tempInt;
	MCBranchNum = 0;
	binNum = 0;
      }
      if(tokens[1] == "Axis") {
	binNum = 0;
      }
      asymmParNum = 0;
      lLFParNum=0;

      continue;
    }

    //Just need values now
    if(nTokens == 3 && tokens[1] == "=") {
      if(position == -1 || 
	 (pdfPosition == -1 && names[position] != "Parameter" && names[position] != "LogLikelihoodFormula")) {
	Errors::AddError("Command "+line+" not valid, or in invalid position");
	cout << line << " not valid, or in invalid position.  Skipping\n";
	continue;
      }

      ostringstream outLine;

      bool notValid = false;
      notValid |= names[position] == "Pdf" &&\
	Tools::SearchStringVector(validPdf,tokens[0]) == -1;
      notValid |= names[position] == "Sys" &&\
	Tools::SearchStringVector(validSys,tokens[0]) == -1;
      notValid |= names[position] == "Bkgd" &&\
	Tools::SearchStringVector(validBkgd,tokens[0]) == -1;
      notValid |= names[position] == "Flux" &&\
	Tools::SearchStringVector(validFlux,tokens[0]) == -1;
      notValid |= names[position] == "Axis" &&\
	Tools::SearchStringVector(validAxis,tokens[0]) == -1;
      notValid |= names[position] == "Parameter" &&\
	Tools::SearchStringVector(validParameter,tokens[0]) == -1;
      notValid |= names[position] == "LogLikelihoodFormula" &&\
	Tools::SearchStringVector(validLogLikelihoodFormula,tokens[0]) == -1;

      if(notValid) {
	Errors::AddError(tokens[0] + " is not a valid option for " +
			 names[position]);
      }

      //Handle Parameters separately
      if(names[position] != "Parameter" && names[position] != "LogLikelihoodFormula") {
	outLine.width(1);
	outLine << "Pdf_" << values[pdfPosition] << "_";
      }
      
      if(names[position] != "Pdf" && names[position] != "Axis") {
	outLine << names[position] << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << values[position] << "_";
	outLine << tokens[0];
      }
      if(names[position] == "Pdf")
	outLine << tokens[0];
      if(names[position] == "Axis") {
	outLine << names[position] << "_";
	outLine.width(1);
	outLine << values[position] << "_";
	outLine << tokens[0];
      }
      
      //MCBranches and Bin (in Axis) need counters, everything else
      //just goes in normally
      //Edit: AsymmPar needs a counter as well, as does Par in LogLikelihoodFormula
      if(tokens[0] == "MCBranch") {
	outLine << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << MCBranchNum;
	MCBranchNum++;
      } else if(tokens[0] == "Bin") {
	outLine.width(2);
	outLine.fill('0');
	outLine << binNum;
	double currentBin = atof(tokens[2].c_str());
	if(binNum != 0 && lastBin >= currentBin) {
	  ostringstream tempString;
	  tempString << "Bins not in order at bin " << binNum;
	  tempString << " for Pdf " << values[pdfPosition] << " ";
	  tempString << names[position];
	  tempString << " " << values[position];
	  Errors::AddError(tempString.str());
	}
	lastBin = currentBin;
	binNum++;
      } else if(tokens[0] == "AsymmPar") {
	outLine << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << asymmParNum;
	asymmParNum++;
      } else if(tokens[0] == "Par") {
	outLine << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << lLFParNum;
	lLFParNum++;
      }

      outLine << "=" << tokens[2];
      outFile << outLine.str() << endl;
      continue;
    }
    

    //That should cover all valid options
    //If we're here, something went wrong
    cout << line << " is not valid, skipping\n";
    Errors::AddError(line+" is not a valid entry");

   
  } //End of loop over file

  if(Errors::GetNErrors() > 0)
    Errors::Exit();

  return 0;

}

int stringToTokens(string toBreak, vector<string>& toFill) {
  string temp;
  toFill.clear();
  size_t position1=0, position2=0;
  
  position1 = toBreak.find_first_not_of(" \n\r",position2);
  while(position1 != string::npos) {
    if(toBreak.at(position1) == '=')
      position2 = toBreak.find_first_of(" \n\r",position1);
    else
      position2 = toBreak.find_first_of(" \n\r=",position1);
    if(position2 == string::npos) {
      temp = toBreak.substr(position1);
    } else {
      temp = toBreak.substr(position1, position2-position1);
    }
    //Deal with splitting up a=b
    if(temp[0] == '=' && temp != "=") {
      toFill.push_back("=");
      toFill.push_back(temp.substr(1));
    } else {
      toFill.push_back(temp);
    }

    position1 = toBreak.find_first_not_of(" \n\r",position2);

  }

  return toFill.size();
  
}

bool isNumber(string toCheck) {
  size_t position;

  position = toCheck.find_first_not_of("0123456789.+-e");
  if(position == string::npos)
    return true;
  else
    return false;
}
