/******
This contains the derived classes that define the functions I'll use for my
calculations/systematics.  They'll all be inline, so no need for a .cxx file,
I think.  Or maybe I'll need an empty one.  Not clear.
******/

#include "RealFunction.h"
#include "TMath.h"
#include "TRandom3.h"

#ifndef _FUNCTIONDEFS_H_
#define _FUNCTIONDEFS_H_

class AddConst : public RealFunction {
 public:
  AddConst() {
    nPars = 1;
    parameters = new Double_t[nPars];
  }
  
  Double_t Eval() {
    return (parameters[0]);
  }
};

class MultiplyConst : public RealFunction {
 public:
  MultiplyConst() {
    nPars = 2;
    parameters = new Double_t[nPars];
  }

  Double_t Eval() {
    return (parameters[0]*parameters[1]);
  }
};


#endif
