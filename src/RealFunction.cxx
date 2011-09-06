/******
Virtual class, parent to my classes to calculate function values
******/

#include "RealFunction.h"

RealFunction::RealFunction() {
  nPars = 0;
  parameters = NULL;
}

RealFunction::~RealFunction() {
  if(parameters != NULL)
    delete [] parameters;
}

void RealFunction::SetParameter(Int_t parNum, Double_t parValue) {
    parameters[parNum] = parValue;
}

Double_t RealFunction::GetParameter(Int_t parNum) {
  return parameters[parNum];
}
