#include <string>
#include "RealFunction.h"
#include "FunctionDefs.h"
#include "Errors.h"
using namespace std;

#ifndef _DECIDER_H_
#define _DECIDER_H_

class Decider {

 public:
  static RealFunction* GenerateFunctionFromString(string className);
  
};

#endif
