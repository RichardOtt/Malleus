/*******
This is a simple function to replace TF1, which is painfully slow

It has three functions, that operate the same as the corresponding
functions in TF1:

SetParameter(i, value) sets parameter i to value value (the function
will keep an internal list of parameters)

GetParameter(i) returns the value of parameter i

Eval(x) returns the value of the function, evaluated at x

The goal here is to be fast.  The actual functions used for my systematics
will inherit from this one, and will be their own classes (so I'll have
a bunch of classes).  Eval will be pure virtual.

*******/

#include "TROOT.h"

#ifndef _REALFUNCTION_H_
#define _REALFUNCTION_H_

class RealFunction {
  
 protected:
  Int_t nPars;
  Double_t *parameters;

 public:
  RealFunction();
  virtual ~RealFunction();
  virtual Double_t Eval() = 0;
  virtual Double_t Eval(Double_t x) { return Eval(); };
  void SetParameter(Int_t parNum, Double_t parValue);
  Double_t GetParameter(Int_t parNum);
  Int_t GetNPars() { return nPars; };

};

#endif
