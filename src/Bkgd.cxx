/*****

*****/

#include "Bkgd.h"

Bkgd::Bkgd() {

  type = '\0';
  bkgdPdf = NULL;
  bkgdFunc = NULL;
  pdfDim = -1;
  bkgdSize = 1;
  bkgdSizeLocation = -1;


}

Bkgd::~Bkgd() {

}

vector<string> Bkgd::Setup(string bkgdName, Double_t bkgdMean,
			   Double_t bkgdSigma, vector<string> mcmcParNames,
			   vector<string> axisNamesIn,
			   vector<string> pdfAxisNames,
			   vector<Double_t> pdfXAxisBins,
			   vector<Double_t> pdfYAxisBins,
			   vector<Double_t> pdfZAxisBins,
			   string filename /*= ""*/, 
			   string histoName /*= ""*/) {
  //Very similar to the setup function for Sys
  //Returns a vector of the names that it needs from the paramter list,
  //i.e. the pars that it handles.
  //It returns a zero-length vector if something goes wrong

  name = bkgdName;
  mean.push_back(bkgdMean);
  sigma.push_back(bkgdSigma);
  Int_t location = Tools::SearchStringVector(mcmcParNames, name);
  if(location == -1) {
    //This shouldn't ever happen
    Errors::AddError(name+" not in list of parameters");
  }
  parNums.push_back(location);
  pars.push_back(0); //So everything has the same size and is ready to go
  pdfDim = pdfAxisNames.size();
  axisNames = pdfAxisNames;

  vector<string> neededNames;

  if(axisNames.size() != axisNamesIn.size()) {
    cout << axisNames.size() << " versus " << axisNamesIn.size() << " axes\n";
    Errors::AddError("Error in "+bkgdName+": wrong number of input axes");
    neededNames.clear();
    return neededNames;
  }
  //Need to make sure that axes names of histogram match those of pdf
  Bool_t succeeded = true;
  for(int i=0; i < axisNamesIn.size(); i++) {
    if(Tools::SearchStringVector(axisNames, axisNamesIn[i]) == -1) {
      Errors::AddError("Error in "+bkgdName+": axis "+axisNamesIn[i]+\
		       " not a pdf axis");
      succeeded = false;
    }
  }
  if(succeeded == false) {
    neededNames.clear();
    return neededNames;
  }


  //For functions and some pre-defined things, this will alter axisNamesIn
  neededNames = LookupName(mcmcParNames, axisNamesIn);
  if(neededNames.size() == 0) {
    Errors::AddError("Error in " + bkgdName + ": name not in list");
    return neededNames;
  }

  if(type == 'h') {
    //Have a histogram, need to extract from file
    if(filename == "" || histoName == "") {
      Errors::AddError("Error in " + bkgdName + \
		       ": need filename and histogram name");
      neededNames.clear();
      return neededNames;
    }
    TFile *histoFile = new TFile(filename.c_str());
    if(histoFile->IsZombie()) {
      Errors::AddError("Error in " + bkgdName + ": couldn't open " + filename);
      neededNames.clear();
      return neededNames;
    }
    TH1* tempHisto = dynamic_cast<TH1*>(histoFile->Get(histoName.c_str()));
    if(tempHisto == NULL) {
      Errors::AddError("Error in "+bkgdName+": couldn't find " + histoName);
      neededNames.clear();
      return neededNames;
    }
    string className = tempHisto->ClassName();
    if(className != "TH1D" && className != "TH2D" && className != "TH3D") {
      Errors::AddError("Error in "+bkgdName+": "+histoName+\
		       " not a TH1D/TH2D/TH3D");
      neededNames.clear();
      return neededNames;
    }
    if(className == "TH1D" && pdfDim != 1) {
      Errors::AddError("Error in "+bkgdName+": "+histoName+\
		       " is a TH1D, not a 1-D pdf");
      neededNames.clear();
      return neededNames;
      
    }
    if(className == "TH2D" && pdfDim != 2) {
      Errors::AddError("Error in "+bkgdName+": "+histoName+\
		       " is a TH2D, not a 2-D pdf");
      neededNames.clear();
      return neededNames;
    }
    if(className == "TH3D" && pdfDim != 3) {
      Errors::AddError("Error in "+bkgdName+": "+histoName+\
		       " is a TH3D, not a 3-D pdf");
      neededNames.clear();
      return neededNames;
    }
    //Ok, histogram is read from file, and is a TH1D/TH2D/TH3D matching pdfDim
    //Now build the internal one, re-ordering the axis as needed


    //This also fills up xAxisBins, etc in the process
    CopyHistogram(tempHisto, axisNamesIn);
    

    for(Int_t axisNum=0; axisNum < pdfDim; axisNum++) {
      //check to see if these boundaries match input ones
      vector<Double_t> axisVector;
      vector<Double_t> axisVectorIn;
      if(axisNum == 0) {
	axisVector = xAxisBins;
	axisVectorIn = pdfXAxisBins;
      } else if(axisNum == 1) {
	axisVector = yAxisBins;
	axisVectorIn = pdfYAxisBins;
      } else if(axisNum == 2) {
	axisVector = zAxisBins;
	axisVectorIn = pdfZAxisBins;
      }


      if(axisVector.size() != axisVectorIn.size()) {
	Errors::AddError("Error in "+bkgdName+": "+histoName+\
			 " axis doesn't have same number of bins as Pdf");
	neededNames.clear();
	return neededNames;
      }
      Int_t bin;
      for(bin=0; bin < axisVector.size(); bin++) {
	if( !Tools::DoublesAreCloseEnough(axisVector[bin], axisVectorIn[bin]) )
	  break;
      }
      if(bin < axisVector.size()) {
	Errors::AddError("Error in "+bkgdName+": "+histoName+\
			 " axis binning doesn't match Pdf axis binning");
	neededNames.clear();
	return neededNames;
      }
      //Ok, binnings are same, move on
      histoFile->Close();
    }
    
  } else if(type == 'f') {
    //Have a function, need to get the correct histogram set up
    //Function should have been filled by LookupName
    if(bkgdFunc == NULL) {
      Errors::AddError("Error in "+bkgdName+": bkgd function not defined");
      neededNames.clear();
      return neededNames;
    }

    xAxisBins = pdfXAxisBins;
    yAxisBins = pdfYAxisBins;
    zAxisBins = pdfZAxisBins;
    
    SetupHistoFromFunction(axisNamesIn);
    RebuildHisto();

  }

  return neededNames;
  
}

Double_t Bkgd::GetBinContent(Int_t xBin, Int_t yBin/*=-1*/, 
			     Int_t zBin/*=-1*/) {
  //Reads out the contents of the background histogram, appropriately
  //scaled by the input parameters
  //Should be able to handle any dimension up to 3, but I will
  //focus on 1-D and 3-D, since that's what I'll actually need

  Double_t content = 0;

  if(pdfDim == 1) {
    content = bkgdPdf->GetBinContent(xBin);
  } else if (pdfDim == 2) {
    content = bkgdPdf->GetBinContent(xBin,yBin);
  } else if (pdfDim == 3) {
    content = bkgdPdf->GetBinContent(xBin, yBin, zBin);
  }

  content *= bkgdSize;

  return content;

}

void Bkgd::SetParameters(Int_t nPars, Double_t *parameters) {
  //Sets internal parameters to new values.  The parameters list
  //is the list from the MCMC high above, just passed through Pdfx

  Int_t parsTotal = pars.size();
  for(int i = 0; i < parsTotal; i++) {
    pars[i] = parameters[parNums[i]];
  }

  bkgdSize = parameters[bkgdSizeLocation];
  //cout << "Setting bkgdSize to " << bkgdSize << endl;

  //May not be this easy
  if(type == 'f') {
    parsTotal = mcmcParNums.size();
    for(int i = 0; i < parsTotal; i++) {
      bkgdFunc->SetParameter(i, parameters[parNums[i]]);
    }
  }

}

void Bkgd::GetBinBoundaries(Int_t axisNum) {
  //Extracts bin boundaries for given axis, where 0 is x, 1 is y, 2 is z
  //Fills up the appropriate vector iAxisBins with this info
  
  TAxis *tempAxis;
  vector<Double_t> *axisVector;
  if(axisNum == 0) {
    tempAxis = bkgdPdf->GetXaxis();
    axisVector = &xAxisBins;
  } else if(axisNum == 1) {
    tempAxis = bkgdPdf->GetYaxis();
    axisVector = &yAxisBins;
  } else if(axisNum == 3) {
    tempAxis = bkgdPdf->GetZaxis();
    axisVector = &zAxisBins;
  } else {
    return; //Do nothing if unresonable axis
  }

  if(tempAxis->GetNbins() < 1)
    return;

  axisVector->push_back(tempAxis->GetBinLowEdge(1));
  for(int i=1; i <= tempAxis->GetNbins(); i++) {
    axisVector->push_back(tempAxis->GetBinUpEdge(i));
  }
 
}

void Bkgd::CopyHistogram(TH1 *tempHisto, vector<string> axisNamesIn) {
  //Copies the histogram in tempHisto to a new one in bkgdPdf
  //Assumes that pdfDim are filled, and 
  //axesNamesIn has size == pdfDim

  if(bkgdPdf != NULL)
    delete bkgdPdf;

  gROOT->cd();

  if(pdfDim == 1) {
    bkgdPdf = dynamic_cast<TH1*>(new TH1D(name.c_str(), name.c_str(),
					  4,0,1));
  } else if(pdfDim == 2) {
    bkgdPdf = dynamic_cast<TH1*>(new TH2D(name.c_str(), name.c_str(),
					  4,0,1, 4,0,1));
  } else if(pdfDim == 3) {
    cout << "Making 3D Background\n";
    bkgdPdf = dynamic_cast<TH1*>(new TH3D(name.c_str(), name.c_str(),
					  4,0,1, 4,0,1, 4,0,1));
  }
  
  xAxisBins.clear();
  yAxisBins.clear();
  zAxisBins.clear();

  vector<Int_t> binMapping;

  for(int i=0; i < axisNames.size(); i++) {
    //Set axes on bkgdPdf to match those on tempHisto, making sure to
    //match order with axesNames rather than tempHisto (i.e. which is x)
    //A lot of the complicatedness of this comes from having to map one
    //set of axes (axisNames, i.e. the pdf's axes) on to another
    //(axisNamesIn, i.e. those input by the user as the histo's axes)

    Int_t position = Tools::SearchStringVector(axisNamesIn, axisNames[i]);
    if(position == -1) {
      Errors::AddError("Error in "+name+": mismatch between histo axis names "\
		       "and pdf axis names");
      return;
    }
    vector<Double_t> *binVector = NULL;
    TAxis *tempAxis = NULL;
    Int_t numberOfBins;
    Double_t *binArray;

    binMapping.push_back(position);

    if(position == 0) {
      tempAxis = tempHisto->GetXaxis();
    } else if(position == 1) {
      tempAxis = tempHisto->GetYaxis();
    } else if(position == 2) {
      tempAxis = tempHisto->GetZaxis();
    }
 
    TAxis *bkgdAxis = NULL;
    if(i == 0) {
      binVector = &xAxisBins;
      bkgdAxis = bkgdPdf->GetXaxis();
    } else if (i == 1) {
      binVector = &yAxisBins;
      bkgdAxis = bkgdPdf->GetYaxis();
    } else if (i == 2) {
      binVector = &zAxisBins;
      bkgdAxis = bkgdPdf->GetZaxis();
    }

    numberOfBins = tempAxis->GetNbins();
    binArray = new Double_t[numberOfBins+1];

    binVector->push_back(tempAxis->GetBinLowEdge(1));
    binArray[0] = tempAxis->GetBinLowEdge(1);
    for(int bin = 1; bin <= numberOfBins; bin++) {
      binVector->push_back(tempAxis->GetBinUpEdge(bin));
      binArray[bin] = tempAxis->GetBinUpEdge(bin);
    }

    //Debug:
//     cout << "Current axis vector/array:\n";
//     for(int j=0; j < binVector->size(); j++) {
//       cout << "binVector[" << j << "] = " << binVector->at(j) << endl;
//       cout << "binArray[" << j << "] = " << binArray[j] << endl;
//     }

    bkgdAxis->Set(numberOfBins, binArray);
    bkgdAxis->SetTitle(axisNames[i].c_str());

    delete [] binArray;
  }
  //Not documented in ROOT, but you need to do this to get the histogram
  //to match to the new axes
  bkgdPdf->Rebuild();

  Double_t binContent=0;
  Int_t binNumbers[3] = {0};
  //Copy bin contents from tempHisto to bkgdPdf
  if(pdfDim == 1) {

    cout << "bkgdPdf has " << bkgdPdf->GetXaxis()->GetNbins() << " bins, ";
    cout << "tempHisto has " << tempHisto->GetXaxis()->GetNbins() << " bins\n";

    for(int xBin=1; xBin <= bkgdPdf->GetXaxis()->GetNbins(); xBin++) {
      binContent = tempHisto->GetBinContent(xBin);
      //cout << "binContent for bin " << xBin << " = " << binContent << endl;
      bkgdPdf->SetBinContent(xBin, binContent);
    }

  } else if(pdfDim == 2) {

    for(int xBin = 1; xBin <= bkgdPdf->GetXaxis()->GetNbins(); xBin++) {
      for(int yBin = 1; yBin <= bkgdPdf->GetYaxis()->GetNbins(); yBin++) {
	binNumbers[binMapping[0]] = xBin;
	binNumbers[binMapping[1]] = yBin;
	binContent = tempHisto->GetBinContent(binNumbers[0],binNumbers[1]);
	bkgdPdf->SetBinContent(xBin, yBin, binContent);
      }
    }

  } else if(pdfDim == 3) {

    for(int xBin = 1; xBin <= bkgdPdf->GetXaxis()->GetNbins(); xBin++) {
      for(int yBin = 1; yBin <= bkgdPdf->GetYaxis()->GetNbins(); yBin++) {
	for(int zBin = 1; zBin <= bkgdPdf->GetZaxis()->GetNbins(); zBin++) {
	  binNumbers[binMapping[0]] = xBin;
	  binNumbers[binMapping[1]] = yBin;
	  binNumbers[binMapping[2]] = zBin;
	  binContent = tempHisto->GetBinContent(binNumbers[0],binNumbers[1],
						binNumbers[2]);
	  bkgdPdf->SetBinContent(xBin, yBin, zBin, binContent);
	}
      }
    }

  }

  //Normalize histo:
  Double_t histoIntegral = bkgdPdf->Integral();
  bkgdPdf->Scale(1/histoIntegral);

  //Ok, bin contents copied, bin name/title set, axes set, normalized.  Done

  //Debug
//   TCanvas *myCanvas = new TCanvas(name.c_str(),name.c_str());
//   myCanvas->cd();
//   tempHisto->Draw();
//   myCanvas->SaveAs((name+".ps(").c_str());
//   bkgdPdf->Draw();
//   myCanvas->SaveAs((name+".ps)").c_str());

//   delete myCanvas;

}


void Bkgd::SetupHistoFromFunction(vector<string> axisNamesIn) {
  //This assumes you have a bkgdFunc, and sets up the corresponding histogram
  //so that you can fill it using RebuildHisto
  //Assumes LookupName has already ran
  //This function is really just to make Setup look a little cleaner
  
  //Remap x,y,z
  axisRemap.clear();
  for(int i=0; i < axisNames.size(); i++) {
    Int_t position = Tools::SearchStringVector(axisNamesIn, axisNames[i]);
    axisRemap.push_back(position);
  }
  
  if(bkgdPdf != NULL)
    delete bkgdPdf;
  
  if(pdfDim == 1) {
    bkgdPdf = dynamic_cast<TH1*>(new TH1D(name.c_str(), name.c_str(),
					  4,0,1));
  } else if(pdfDim == 2) {
    bkgdPdf = dynamic_cast<TH1*>(new TH2D(name.c_str(), name.c_str(),
					  4,0,1, 4,0,1));
  } else if(pdfDim == 3) {
    bkgdPdf = dynamic_cast<TH1*>(new TH3D(name.c_str(), name.c_str(),
					  4,0,1, 4,0,1, 4,0,1));
  }
  
  TAxis *tempAxis = NULL;
  vector<Double_t> binVector;

  for(int dim=0; dim < pdfDim; dim++) {
    if(dim == 0) {
      tempAxis = bkgdPdf->GetXaxis();
      binVector = xAxisBins;
    } else if(dim == 1) {
      tempAxis = bkgdPdf->GetYaxis();
      binVector = yAxisBins;
    } else if(dim == 2) {
      tempAxis = bkgdPdf->GetZaxis();
      binVector = zAxisBins;
    }

    Int_t nBins = binVector.size() - 1;
    Double_t *binArray = new Double_t[nBins+1];

    for(int i=0; i <= nBins; i++) {
      binArray[i] = binVector[i];
    }
    
    tempAxis->Set(nBins, binArray);
    tempAxis->SetTitle(axisNames[dim].c_str());

    delete [] binArray;
  }
  

}

void Bkgd::RebuildHisto() {
  //Assumes everything has already been set up, axes are configured properly
  //Just takes the input function parameters and uses them to create a histo
  //Remember that the function has the "wrong" axes, and the histo has the
  //pdf's axes, and axisRemap[0] gives the axis in the function that 
  //corresponds to the histo's x axis, etc.
  //Uses function's value at bin center to get the bin value, due to sloth
  //Integrating might be something for a later version
  
  Double_t binContent;
  Double_t binCenter[3];
  if(pdfDim == 1) {
    //No need to mess with axes
    for(int xBin=1; xBin < xAxisBins.size(); xBin++) {
      binCenter[0] = (xAxisBins[xBin] - xAxisBins[xBin-1]) / 2;
      binContent = bkgdFunc->Eval(binCenter[0]);
      bkgdPdf->SetBinContent(xBin, binContent);
    }

  } else if(pdfDim == 2) {

    for(int xBin=1; xBin < xAxisBins.size(); xBin++) {
      for(int yBin=1; yBin < yAxisBins.size(); yBin++) {
	binCenter[axisRemap[0]] = (xAxisBins[xBin] - xAxisBins[xBin-1]) / 2;
	binCenter[axisRemap[1]] = (yAxisBins[yBin] - yAxisBins[yBin-1]) / 2;
	binContent = bkgdFunc->Eval(binCenter[0],binCenter[1]);
	bkgdPdf->SetBinContent(xBin,yBin,binContent);
      }
    }

  } else if(pdfDim == 3) {

    for(int xBin=1; xBin < xAxisBins.size(); xBin++) {
      for(int yBin=1; yBin < yAxisBins.size(); yBin++) {
	for(int zBin=1; zBin < zAxisBins.size(); zBin++) {
	  binCenter[axisRemap[0]] = (xAxisBins[xBin] - xAxisBins[xBin-1]) / 2;
	  binCenter[axisRemap[1]] = (yAxisBins[yBin] - yAxisBins[yBin-1]) / 2;
	  binCenter[axisRemap[2]] = (zAxisBins[zBin] - zAxisBins[zBin-1]) / 2;
	  binContent = bkgdFunc->Eval(binCenter[0],binCenter[1]);
	  bkgdPdf->SetBinContent(xBin,yBin,binContent);
	}
      }
    }

  }

  //Normalize
  Double_t histoIntegral = bkgdPdf->Integral();
  bkgdPdf->Scale(histoIntegral);


}

vector<string> Bkgd::LookupName(vector<string> mcmcParNames, 
				vector<string>& axisNamesIn) {

  vector<string> neededNames;

  if(name == "Spike") {
    neededNames.push_back(name);
    bkgdSizeLocation = Tools::SearchStringVector(mcmcParNames, name);
    mcmcParNums.push_back(bkgdSizeLocation);
    type = 'h';
  }

  return neededNames;
}

Double_t Bkgd::CalcLogLikelihood() {
  //This will need to be updated to include covariance matricies later

  Double_t logLikelihood = 0;

  Int_t nPars = pars.size();
  for(int i=0; i < nPars; i++) {
    if(sigma[i] <= 0)
      continue;

    logLikelihood -= (pars[i]-mean[i])*(pars[i]-mean[i])/2/sigma[i]/sigma[i];
    logLikelihood -= TMath::Log(sigma[i])/2;
  }
  return logLikelihood;
}

Bool_t Bkgd::AddPar(string sysName, Double_t sysMean, Double_t sysSigma,
		    vector<string> mcmcParNames) {
  //Includes extra parameters that this one is in charge of,
  //mostly for functions

  pars.push_back(0);
  parNums.push_back(Tools::SearchStringVector(mcmcParNames,sysName));
  mean.push_back(sysMean);
  sigma.push_back(sysSigma);
  
  return true;
}

void Bkgd::PrintState() {
  cout << "-------------------------------------\n";
  cout << "Bkgd " << name << endl;
  cout << "pdfDim = " << pdfDim << endl;

  for(int i = 0; i < parNames.size(); i++)
    cout << "parNames[" << i << "] = " << parNames[i] << endl;

  for(int i = 0; i < parNums.size(); i++)
    cout << "parNums[" << i << "] = " << parNums[i] << endl;

  for(int i = 0; i < pars.size(); i++)
    cout << "pars[" << i << "] = " << pars[i] << endl;

  for(int i = 0; i < mean.size(); i++)
    cout << "mean[" << i << "] = " << mean[i] << endl;

  for(int i = 0; i < sigma.size(); i++)
    cout << "sigma[" << i << "] = " << sigma[i] << endl;
  
  for(int i = 0; i < mcmcParNums.size(); i++)
    cout << "mcmcParNums[" << i << "] = " << mcmcParNums[i] << endl;

  cout << "bkgdSize = " << bkgdSize << endl;
  cout << "bkgdSizeLocation = " << bkgdSizeLocation << endl;

  for(int i = 0; i < axisNames.size(); i++)
    cout << "axisNames[" << i << "] = " << axisNames[i] << endl;

  for(int i = 0; i < xAxisBins.size(); i++)
    cout << "xAxisBins[" << i << "] = " << xAxisBins[i] << endl;

  for(int i = 0; i < yAxisBins.size(); i++)
    cout << "yAxisBins[" << i << "] = " << yAxisBins[i] << endl;

  for(int i = 0; i < zAxisBins.size(); i++)
    cout << "zAxisBins[" << i << "] = " << zAxisBins[i] << endl;

  for(int i = 0; i < axisRemap.size(); i++)
    cout << "axisRemap[" << i << "] = " << axisRemap[i] << endl;
  
  //cout << "bkgdPdf info:\n" << *bkgdPdf << endl;
  if(pdfDim == 1) {
    for(int i = 0; i < bkgdPdf->GetXaxis()->GetNbins(); i++) {
      cout << "bin " << i << " content = " << bkgdPdf->GetBinContent(i) << endl;
    }
  }
  if(pdfDim == 3) {
    for(int xBin=1; xBin < bkgdPdf->GetXaxis()->GetNbins(); xBin++) {
      for(int yBin=1; yBin < bkgdPdf->GetYaxis()->GetNbins(); yBin++) {
	for(int zBin=1; zBin < bkgdPdf->GetZaxis()->GetNbins(); zBin++) {
	  cout << "Bin(" << xBin << "," << yBin << "," << zBin << ") = ";
	  cout << bkgdPdf->GetBinContent(xBin,yBin,zBin) << endl;
	}
      }
    }
  }


}
