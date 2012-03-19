/******
This contains the derived classes that define the functions used for
Sys.  They're all be inline, so no need for a .cxx file, but I put an
empty one to be safe.

The user will most likely need to alter this file at some point.
Unfortunately, there's no error checking here, other than the usual
compiler stuff - this is actual code.  Ideally, one day I'll get away
from this and handle it in a more user-friendly way.

The couple of places you can trip up here are:
1. Forgetting to give the class (i.e. your function) a unique name
2. Forgetting to change the constructor name to that class name
3. Not updating nPars to match the actual number of parameters in Eval
4. Skipping values in the parameters array - i.e. having a function
   that looks like   parameters[1] + parameters[3], which is missing
   both parameters[0] and parameters[2].  While technically OK (as long as
   nPars is 4), this can get you in trouble if you *think* you're only using
   2 or 3 parameters, and don't call the right number of MCMCParameterValue
   and MCBranchValue in the config file (it should complain at run time).

The ones named "AddConst", "MultiplyConst", "Linear" and "Resolution" come
with the system, and work correctly.  Use them as models if you're not sure.

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

class Linear : public RealFunction {
 public:
  Linear() {
    nPars = 3;
    parameters = new Double_t[nPars];
  }

  Double_t Eval() {
    return (parameters[0] + parameters[1]*parameters[2]);
  }
};

class Resolution : public RealFunction {
 public:
  Resolution() {
    nPars = 3;
    parameters = new Double_t[nPars];
  }

  Double_t Eval() {
    return parameters[0]*(parameters[1] - parameters[2]);
  }
};

#endif
