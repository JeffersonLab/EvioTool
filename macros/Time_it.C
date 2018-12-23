R__LOAD_LIBRARY(libEvioTool);

void Time_it(){
  //  gSystem->Load("Release/libEvioTool.so");
  EVIO_Event_t *evt=new EVIO_Event_t;
  EvioTool *ev=new EvioTool("/data/HPS/data/engrun2015/evio/hps_005772.evio.0");
  ev->fDebug=0;
  int i=0;
  TStopwatch st;
  st.Start();
  //  for(int k=0;k<7;k++) evt.SVT[i].data.reserve(10);
  while(ev->read() && i<1000000){ 
    ev->parse(evt);
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
