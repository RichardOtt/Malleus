

{

  TFile myfile("fakeData.root","RECREATE");
  myfile.cd();
  TTree *mytree = new TTree("fakeData","Flat dist");
  Float_t energy;
  mytree->Branch("X",&energy,"X/F");
  for(int i=0; i < 800; i++) {
    energy = gRandom->Rndm()*2;
    // if(energy < 0 || energy > 4) {
    //   i--;
    //   continue;
    // }
    if(energy > -1 && energy < 3)
      mytree->Fill();
  }

  myfile.cd();
  mytree->Write();
  delete mytree;

  myfile.Close();

  TFile myfile2("fakeMC.root","RECREATE");

  mytree = new TTree("fakeMC","Flat Dist");
  mytree->Branch("X",&energy,"X/F");
  for(int i=0; i < 40000; i++) {
    energy = gRandom->Rndm()*2;
    
    mytree->Fill();
  }
  myfile2.cd();
  mytree->Write();
  delete mytree;

  myfile2.Close();

}
