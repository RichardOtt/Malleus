
#include <iostream>
#include <string>
#include <fstream>
#include "MCMC.h"
#include "ConfigFile.h"
#include "Errors.h"
#include "TStopwatch.h"
//#include "/usr/include/valgrind/callgrind.h"
using namespace std;

void print_help();

int main(int argc, char *argv[]) {

  string filename = "config.txt";
  string metafilename = "";
  string tempString;

  for(int i=1; i < argc; i++) {
    tempString = argv[i];
    if(tempString == "-h" || tempString == "--help") {
      print_help();
      return 0;
    }
    if(tempString == "-c" || tempString == "--config") {
      if(i+1 >= argc) {
	cout << "Please specify a file after -c or --config\n";
	return 2;
      } else {
	filename = argv[i+1];
      }
    }
    if(tempString == "-m" || tempString == "--meta") {
      if(i+1 >= argc) {
	cout << "Please specify a file after -m or --meta\n";
	return 2;
      } else {
	metafilename = argv[i+1];
      }
    }
  }

  if(metafilename == "") {
    //No meta, use config
    ifstream configfiletest;
    configfiletest.open(filename.c_str());
    if(configfiletest.fail()) {
      cout << "Unable to open file " << filename << ", exiting\n";
      return 2;
    }
    configfiletest.close();
  } else {
    //Meta supercedes config
    
  }

  MCMC mcmc;

  ConfigFile config(filename);

  Bool_t succeeded = true;
  succeeded = mcmc.ReadConfig(config);

  //mcmc.PrintPdf(0);

  cout << "Number of errors: " << Errors::GetNErrors() << endl;
  if(!succeeded)
    Errors::Exit();

  mcmc.Initialize();
  //mcmc.PrintState();
  //mcmc.PrintSetup();

  TStopwatch timer;

  //CALLGRIND_START_INSTRUMENTATION
  timer.Start();
  //mcmc.TakeStep();
  mcmc.Run();
  timer.Stop();

  timer.Print();

  //mcmc.PrintPdf(0);

  return 0;
}

void print_help() {
  cout << "Available options:\n";
  cout << "-c filename\n";
  cout << "       Reads filename as a config file, default is config.txt\n";
  cout << "--config filename\n";
  cout << "       Same as -c filename\n";
  cout << "-m filename\n";
  cout << "       Reads filename as a meta file, overrides config file.";
  cout << "  No default\n";
  cout << "--meta filename\n";
  cout << "       Same as -m filename\n";
  cout << "-h or --help\n";
  cout << "       Prints this message\n";
  cout << "Please see documentation for usage details\n";
}
