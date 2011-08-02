
#include <iostream>
#include <string>
#include <fstream>
#include "MCMC.h"
#include "ConfigFile.h"
#include "Errors.h"
#include "TStopwatch.h"
//#include "/usr/include/valgrind/callgrind.h"
using namespace std;

int main(int argc, char *argv[]) {

  string filename = "config.txt";
  string tempString;

  for(int i=1; i < argc; i++) {
    tempString = argv[i];
    if(tempString == "-h" || tempString == "--help") {
      cout << "Help will be implemented later\n";
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
  }

  ifstream configfiletest;
  configfiletest.open(filename.c_str());
  if(configfiletest.fail()) {
    cout << "Unable to open file " << filename << ", exiting\n";
    return 2;
  }
  configfiletest.close();

  MCMC blah;

  ConfigFile config(filename);

  Bool_t succeeded = true;
  succeeded = blah.ReadConfig(config);

  //blah.PrintPdf(0);

  cout << "Number of errors: " << Errors::GetNErrors() << endl;
  if(!succeeded)
    Errors::Exit();

  blah.Initialize();
  //blah.PrintState();
  //blah.PrintSetup();

  TStopwatch timer;

  //CALLGRIND_START_INSTRUMENTATION
  timer.Start();
  //blah.TakeStep();
  blah.Run();
  timer.Stop();

  timer.Print();

  //blah.PrintPdf(0);

  return 0;
}
