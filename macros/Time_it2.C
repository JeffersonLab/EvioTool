R__LOAD_LIBRARY(libEvioTool);

void Time_it2(){
  //  gSystem->Load("Release/libEvioTool.so");
  EvioTool *p = new EvioTool();
  p->Open("/data/HPS/data/engrun2015/evio/hps_005772.evio.0");

  int i=0;
  TStopwatch st;

  p->fAutoAdd=0;
  Leaf *Header *header=p->AddLeaf<unsigned int>("Header",49152,0,"Header bank");
  Bank *ECAL = p->AddBank("Ecal",{37,39},0,"Ecal banks");
  Leaf<FADCdata> *FADC = ECAL->AddLeaf<FADCdata>("FADC",57601,0,"FADC mode 1 data");

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
  fa->Draw("colz");
}
