#include "metaReader.h"

int metaReader::ReadMetaFile(string metaFilename) {
  ifstream inFile;
  inFile.open(metaFilename.c_str());
  if(inFile.fail()) {
    cout << "Unable to open file " << metaFilename << endl;
    Errors::AddError("Unable to open file"+metaFilename);
    return 2;
  } else {
    return ReadMetaFile(inFile);
  }
}

int metaReader::ReadMetaFile(ifstream& inFile) {
  //Read in file.  Two types of commands - new and =
  //new <thing> creates a new <thing>, parameter=value sets parameter to value
  //MCMC parameters are just copied
  vector<string> MCMCcommands;
  MCMCcommands.push_back("MCMC_PrintFrequency");
  MCMCcommands.push_back("MCMC_ChainLength");
  MCMCcommands.push_back("MCMC_SkipSteps");
  MCMCcommands.push_back("MCMC_UseAsymmetry");
  MCMCcommands.push_back("MCMC_DynamicSteps");
  //MCMCcommands.push_back("MCMC_ForceAcceptSteps");
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
  validSys.push_back("Function");
  validSys.push_back("Target");
  validSys.push_back("UseMultiply");
  validSys.push_back("UseOriginalData");
  validSys.push_back("AddFluxAffected");
  validSys.push_back("MCMCParameterValue");
  validSys.push_back("MCBranchValue");

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
  validAxis.push_back("LowerBound");
  validAxis.push_back("UpperBound");
  validAxis.push_back("nBins");
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
  int fluxAffectedNum=0, MCMCParameterValueNum=0, MCBranchValueNum=0;
  int makeBinsCounter = 1;
  int nBins = 0;
  double binMin = 0, binMax = 0;
  double lastBin = 0;
  bool extendLine = false;
  while(!inFile.eof()) {
    if(!extendLine) {
      getline(inFile,line);
    } else {
      string tempString;
      getline(inFile,tempString);
      line.append(tempString);
      extendLine = false;
    }
    //cout << "Read in line: " << line << endl;
    size_t lastChar = line.find_last_not_of(" \n\r");
    size_t firstChar = line.find_first_not_of(" \n\r");
    if(line[lastChar] == '\\' && line[firstChar] != '#') {
      //extend non-comment line ending in a backslash
      extendLine = true;
      continue;
    }
    
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
	configData.push_back(tokens[0]+tokens[1]+tokens[2]);
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
	fluxAffectedNum = 0;
	MCMCParameterValueNum = 0;
	MCBranchValueNum = 0;
      }
      if(tokens[1] == "Axis") {
	binNum = 0;
	makeBinsCounter = 1;
      }
      if(tokens[1] == "Sys") {
	fluxAffectedNum = 0;
	MCMCParameterValueNum = 0;
	MCBranchValueNum = 0;
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
	if(tokens[0] != "LowerBound" && tokens[0] != "UpperBound" &&
	   tokens[0] != "nBins")
	  outLine << tokens[0];
      }

      //Deal with expanding nBins
      if(tokens[0] == "nBins") {
	nBins = atoi(tokens[2].c_str());
	makeBinsCounter *= 2;
      }
      if(tokens[0] == "LowerBound" && names[position] == "Axis") {
	binMin = atof(tokens[2].c_str());
	makeBinsCounter *= 3;
      }
      if(tokens[0] == "UpperBound" && names[position] == "Axis") {
	binMax = atof(tokens[2].c_str());
	makeBinsCounter *= 5;
      }
      if(makeBinsCounter == 30) {
	for(int bin = 0; bin < nBins+1; bin++) {
	  lastBin = (binMax - binMin)/nBins*bin + binMin;
	  ostringstream tempString;
	  tempString << "Bin";
	  tempString.width(2);
	  tempString.fill('0');
	  tempString << binNum;
	  tempString << "=" << lastBin;
	  configData.push_back(outLine.str()+tempString.str());
	  binNum++;
	}
	makeBinsCounter = 1;
      }

      
      //MCBranches and Bin (in Axis) need counters, everything else
      //just goes in normally
      //Edit: AsymmPar needs a counter as well, as does Par in LogLikelihoodFormula
      //Edit: now also have bins generated by nBins + min + max, handled
      //separately above
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
      } else if(tokens[0] == "AddFluxAffected") {
	outLine << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << fluxAffectedNum;
	fluxAffectedNum++; 
      } else if(tokens[0] == "MCMCParameterValue") {
	outLine << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << MCMCParameterValueNum;
	MCMCParameterValueNum++; 
      } else if(tokens[0] == "MCBranchValue") {
	outLine << "_";
	outLine.width(2);
	outLine.fill('0');
	outLine << MCBranchValueNum;
	MCBranchValueNum++; 
      }

      if(tokens[0] == "nBins" || tokens[0] == "LowerBound" || 
	 tokens[0] == "UpperBound")
	continue;
      outLine << "=" << tokens[2];
      configData.push_back(outLine.str());
      continue;
    }
    

    //That should cover all valid options
    //If we're here, something went wrong
    cout << line << " is not valid, skipping\n";
    Errors::AddError(line+" is not a valid entry");

   
  } //End of loop over file

  return Errors::GetNErrors();
}

int metaReader::PrintConfigToFile(string filename) {
  ofstream outFile;
  outFile.open(filename.c_str());
  return PrintConfigToFile(outFile);
}

int metaReader::PrintConfigToFile(ofstream& outFile) {

  if(outFile.fail()) {
    cerr << "Unable to open output file\n";
    return 2;
  }

  for(int i=0; i < configData.size(); i++) {
    outFile << configData[i] << endl;
  }

  outFile.close();

  return 0;

}

string metaReader::GetConfigLine() {
  if(lineNumber < configData.size()) {
    lineNumber++;
    return configData[lineNumber-1];
  } else {
    return "";
  }
}

bool metaReader::SetLineNumber(int setLineNumberTo) {
  if(setLineNumberTo >= configData.size())
    return false;
  else
    lineNumber = setLineNumberTo;
  return true;
}

int metaReader::stringToTokens(string toBreak, vector<string>& toFill) {
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
