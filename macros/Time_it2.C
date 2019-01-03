R__LOAD_LIBRARY(libEvioTool);

void Time_it2(){
  //  gSystem->Load("Release/libEvioTool.so");
  EvioParser *p = new EvioParser();p->Open("/data/HPS/data/engrun2015/evio/hps_005772.evio.0");

  int i=0;
  TStopwatch st;
  //  for(int k=0;k<7;k++) evt.SVT[i].data.reserve(10);
  p->fAutoAdd=0;
  p->Add_Leaf<unsigned int>("Header",49152,0,"Header bank");
  Bank *ECAL = p->Add_Bank("Ecal",{37,39},0,"Ecal banks");
  Leaf<FADCdata_raw> *FADC = ECAL->Add_Leaf<FADCdata_raw>("FADC",57601,0,"FADC mode 1 data");

  cout << "Start Stopwatch\n";
  st.Start();

  while(p->Next()==0  && i<1000000){ 
    if( (++i % 50000) == 0 ){
      double time=st.RealTime(); 
      cout << i << " at " << time << "  ";
      cout << (double)i/time/1000. << " kHz " << 1.e6*time/i << " micro s/event"<< endl;
      st.Continue();
    } 
  }; 
  st.Stop();
  cout << (double)i/st.RealTime()/1000. << " kHz " << 1.e6*st.RealTime()/i << " micro seconds"<< endl;
}
