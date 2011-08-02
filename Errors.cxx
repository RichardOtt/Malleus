/*****
Error reporting, see .h file for details
*****/

#include "Errors.h"

vector<string> Errors::errorList;
int Errors::numErrors = 0;

void Errors::AddError(string toAdd) {
  errorList.push_back(toAdd);
  numErrors++;
}

void Errors::Exit() {
  int errSize = errorList.size();
  if(errSize != numErrors)
    cout << "Number of errors not counted properly\n";  //Shouldn't happen
  cout << errSize << " errors in list:\n";
  for(int i = 0; i < errSize; i++)
    cout << errorList[i] << endl;

  exit(1);
}

void Errors:: Exit(string toOutput) {
  AddError(toOutput);
  Exit();
}

int Errors::GetNErrors() {
  return numErrors;
}
