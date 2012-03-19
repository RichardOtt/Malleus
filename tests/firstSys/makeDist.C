

{

  TFile myfile("fakeData.root","RECREATE");
  myfile.cd();
  TTree *mytree = new TTree("fakeData","Broadened step");
  Float_t energy;
  mytree->Branch("Energy",&energy,"Energy/F");
  for(int i=0; i < 500; i++) {
    energy = gRandom->Rndm()*1.5 + 0.5; //Produces flat between 0.5 and 2
    energy += gRandom->Gaus(0,0.3); 
    //Above acts to broaden, effectively finite resolution

    if(energy > -1 && energy < 3)
      mytree->Fill();
  }

  myfile.cd();
  mytree->Write();
  delete mytree;

  myfile.Close();

  TFile myfile2("fakeMC.root","RECREATE");

  Float_t trueEnergy;
  mytree = new TTree("fakeMC","Broadened step");
  mytree->Branch("Energy",&energy,"Energy/F");
  mytree->Branch("TrueEnergy",&trueEnergy,"TrueEnergy/F");
  for(int i=0; i < 50000; i++) {
    trueEnergy = gRandom->Rndm()*1.5 + 0.5;

    energy = trueEnergy + gRandom->Gaus(0,0.2);
    
    mytree->Fill();
  }
  myfile2.cd();
  mytree->Write();
  delete mytree;

  myfile2.Close();

}
