#include <cstdlib>
#include "ConfigFile.h"
#include "MCMC.h"
#include "Errors.h"

void print_help() {
    cout << "usage:  drawResults configfile fitResultsFile outputFile\n";
    cout << "configfile is a config, not a meta config\n";
    cout << "fitResultsFile is the .fitResult file produced by autoFit.exe\n";
    cout << "outputFile is a .ps or .pdf file to write\n";
}

int main(int argc, char *argv[]) {

  string configFilename;
  string fitResultsFile;
  string outputFile;

  if(argc < 2) {
    print_help();
    return 2;
  }

  for(int i=1; i < argc; i++) {
    string tempString = argv[i];
    if(tempString == "-h" || tempString == "--help") {
      print_help();
      return 2;
    }
  }

  if(argc != 4) {
    print_help();
    return 2;
  }

  configFilename = argv[1];
  fitResultsFile = argv[2];
  outputFile = argv[3];

  ifstream filetest;
  filetest.open(configFilename.c_str());
  if(filetest.fail()) {
    cout << "Unable to open file " << configFilename << ", exiting\n";
    return 2;
  }
  filetest.close();

  filetest.open(fitResultsFile.c_str());
  if(filetest.fail()) {
    cout << "Unable to open file " << fitResultsFile << ", exiting\n";
    return 2;
  }
  filetest.close();

  ConfigFile config(configFilename);
  ConfigFile fitResults(fitResultsFile);
  
  //Prevent overwriting output file
  if(config.keyExists("OutputFilename"))
    config.remove("OutputFilename");
  config.add<string>("OutputFilename","dummyForDrawing3819283910.root");

  MCMC toDraw;

  Bool_t succeeded = true;
  succeeded = toDraw.ReadConfig(config);
  if(succeeded == false) {
    cout << "Config file setup failed!\n";
    Errors::Exit();
  }

  succeeded = toDraw.Initialize(fitResults);
  if(succeeded == false) {
    cout << "Fit result setup failed!\n";
    Errors::Exit();
  }

  toDraw.PrintState();

  toDraw.Draw(outputFile);

  remove("dummyForDrawing3819283910.root");

  //cout << "Made it to the end safely\n";

  return 0;
}
