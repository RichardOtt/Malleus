/*****
This class will be a simple way to deal with errors.

Instead of printing out one error and exiting, tests (during set-up)
will push an error on to the stack in this program, and continue forward
as best as possible.  On particularly egregious errors, it may abort
execution of the code anyway, and print out errors that have built up
to that point.

At the end of set-up, if any errors have occurred, they will be printed
out and the program will exit.

Unfortunately, that means that this class will contain a few "static"
variables.

Useage:
At any time, call Errors::AddError(string) to push error message string
on to the list

call Errors::Exit() to print out the list of errors and exit

call Errors::Exit(string) to both push string on to the list and exit as above

call Errors::GetNErrors() to return the number of errors (zero if none added)
*****/

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
using namespace std;

#ifndef _ERRORS_H_
#define _ERRORS_H_

class Errors {

 private:
  static vector<string> errorList;
  static int numErrors;

 public:
  static void AddError(string toAdd);
  static void Exit();
  static void Exit(string toOutput);
  static int GetNErrors();

};

#endif
