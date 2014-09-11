{
  gSystem->Load("build/Release/libEvioTool.so");
  EVIO_Event_t *evt=new EVIO_Event_t;
  EvioTool *ev=new EvioTool("/data/HPS/data/hps_001360.evio.0");
  TStopwatch st;
  st.Start();
  int FADC_13_size=0;
  int F13_data_size=0;

  int FADC_15_size=0;
  int SVT_size=0;
  int SVT_temps_size=0;
  int SVT_data_size=0;
  int F15_data_size=0;
  int F15_hits_size=0;
  int F13_sample_size=0;
  
  int ev_count=0;

  ev->fDebug=6;

  while(ev->read() && ev_count<100){ 
    ev->parse(evt);
    if( (++ev_count % 1000) == 0 ){
      double time=st.RealTime(); 
      cout << ev_count << " at " << time << " evt_num: " << evt->event_number << " evt size: " << sizeof(evt) << endl;
      st.Continue();
    } 

    FADC_15_size=TMath::Max(FADC_15_size,(int)evt->FADC_15.size());
    //    cout << "FADC_15_size = " << FADC_15_size <<  " " << (int)evt->FADC_15.size() << endl;
    for(int k=0;k<evt->FADC_15.size(); k++){
      F15_data_size = TMath::Max(F15_data_size,(int)evt->FADC_15[k].data.size());
      for(int j=0;j<(int)evt->FADC_15[k].data.size();j++){
	F15_hits_size = TMath::Max(F15_hits_size,(int)evt->FADC_15[k].data[j].adc.size());
      }
    }
    SVT_size=7;
    // cout << "SVT_size     = " << SVT_size     <<  " " << (int)evt->SVT.size() << endl;
    for(int j=0;j<7; j++){
      SVT_temps_size=TMath::Max(SVT_temps_size,(int)evt->SVT[j].temps.size());
      SVT_data_size=TMath::Max(SVT_data_size,(int)evt->SVT[j].data.size());
   }
  }; 
  st.Stop();
  cout << (double)ev_count/st.RealTime() << " Hz " << 1e6*st.RealTime()/ev_count << " micro seconds"<< endl;
  cout << "FADC_13_size  = " << FADC_13_size << endl;
  cout << "FADC_15_size  = " << FADC_15_size << endl;
  cout << "F15_data_size = " << F15_data_size << endl;
  cout << "F15_hits_size = " << F15_hits_size << endl;
  cout << "SVT_size      = " << SVT_size << endl;
  cout << "SVT_temps_size= " << SVT_temps_size << endl;
  cout << "SVT_data_size = " << SVT_data_size << endl;
}
