#include "Decider.h"

RealFunction* Decider::GenerateFunctionFromString(string className) {

  if(className == "AddConst") {
    return new AddConst();
  } else if(className == "MultiplyConst") {
    return new MultiplyConst();
  } else if(className == "Linear") {
    return new Linear();
  } else if(className == "Resolution") {
    return new Resolution();
  } else {
    return NULL;
  }

}
