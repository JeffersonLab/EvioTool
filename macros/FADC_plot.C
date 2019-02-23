R__LOAD_LIBRARY(libHPSEvioReader); // ROOT6 way to load a library.

void FADC_plot(){
  EvioTool *p = new EvioTool();
  // Open an EVIO data file. Modify the filename and path to where you have your data.
  p->Open("hps_005772.evio");
  int i=0;
  TStopwatch st;
  p->fAutoAdd=0;  // Do not add in all the data found in the file.
  p->Add_Leaf<unsigned int>("Header",49152,0,"Header bank"); // Add a Leaf for the Header data of type unsigned_int.
  Bank *ECAL = p->Add_Bank("Ecal",{37,39},0,"Ecal banks"); // Add the ECAL Bank, with can have tag 37 or 39
  // Then, in the ECAL bank, add the FADC data Leaf for mode 1 data.
  Leaf<FADCdata> *FADC = ECAL->Add_Leaf<FADCdata>("FADC",57601,0,"FADC mode 1 data"); 

  TH2F *fa=new TH2F("fa","FADC pulse accumulation",51,0,50,201,0,400); // Book 2-D histogram.

  cout << "Start Stopwatch\n";
  st.Start();
  
  while(p->Next()==0  && i<1000000){     // Run through the events.
    if( (++i % 50000) == 0 ){
      st.Stop();
      double time=st.RealTime(); 
      cout << i << " at " << time << "  ";
      cout << (double)i/time/1000. << " kHz " << 1.e6*time/i << " micro s/event"<< endl;
      st.Continue();
    }
    for(int ii=0;ii<FADC->size(); ++ii){            // For each bit of FADC data found
      for(int j=0;j<FADC->Get_data(ii).samples.size();++j){ // Run through the size of the Leaf
	fa->Fill(j,FADC->Get_data(ii).samples[j]);  // Fill histogram with each pulse sample.
      }
    }
  };   
  st.Stop();
  cout << "Processing for " << st.RealTime() << endl;
  cout << (double)i/st.RealTime()/1000. << " kHz " << 1.e6*st.RealTime()/i << " micro seconds/event."<< endl;
  fa->Draw("colz");
}
