//
//  main.cpp
//  EvioTool_Test
//
//  Created by Maurik Holtrop on 12/29/18.
//  Copyright © 2018 UNH. All rights reserved.
//
//
#include <ratio>
#include <chrono>

#include <limits.h>

#include <iostream>
#include <string>
using namespace std;

#include "EvioTool.h"
#include "SVTbank.h"
#include "Header.h"

#include "TFile.h"
#include "TCanvas.h"
#include "TH1D.h"

struct Arguments_t {
  vector<string> filenames;
  string et_name;
  string et_host_name;
  int    et_port;
  int    debug;
  int    quiet;
  bool   use_et;
  bool   et_block;
  bool   show_head;
  bool   show_svt;
  bool   show_ecal;
  bool   auto_add;
  bool   print_evt;
};

void Print_Usage(const char *name);
void Parse_Args(int *argc, const char **argv, Arguments_t *p_arg);

int main(int argc, const char * argv[])
{
  Arguments_t args;
  Parse_Args(&argc,argv,&args);
  
  EvioTool *etool=new EvioTool();
  
  if(args.use_et){
    cout << "Error ET system not yet implemented. Exit. \n";
    exit(1);
  }
  
  if(args.debug==0){
    etool->fDebug = 0b000000;
  }else if(args.debug==1){
    etool->fDebug = 0b000001;
  }else if(args.debug==2){
    etool->fDebug = 0b000011;
  } else if(args.debug == 3){
    etool->fDebug = 0b000111;
  } else{
    etool->fDebug = 0xFF;
  }

  TFile root_file("Hodo.root","RECREATE");

  etool->fAutoAdd=false;
  etool->fChop_level=1;
  etool->tags={1<<7};  // Parse HPS physics events only.
  etool->tag_masks = {1<<7};

  // Setup the Event structure"
  auto head = new Header(etool,49152,0);
  auto Hodo = etool->AddBank("Ecal",{65},0,"Ecal banks");
  auto Hodo_FADC = Hodo->AddLeaf<FADCdata>("FADC",57601,0,"FADC mode 1 data");

  std::cout << "Debug set to " << etool->fDebug << std::endl;
  
  etool->PrintBank(5);

  long evt_count=0;
  long totalCount=0;
  std::chrono::microseconds totalTime(0);

  auto start = std::chrono::system_clock::now();
  auto time1 = start;
  
  vector<TH1F *> hit_histos;
  for(int i=0;i<48;++i){
    string name = "h"+to_string(i);
    string title = "Hodoscope FADC"+to_string(i);
    TH1F *h = new TH1F(name.c_str(),title.c_str(),150,0,149);
    hit_histos.push_back(h);
  }
  
  TCanvas *cc = new TCanvas("cc","HodoHits",800,1200);
  cc->Divide(4,12);
  cc->Draw();
  for(auto file: args.filenames){   // Files loop.
    etool->Open(file.c_str());
    while(etool->Next() == S_SUCCESS){ // Event loop.
      if(args.debug) cout<<"EVIO Event " << evt_count << endl;
      evt_count++;
      for(int i=0; i<(int)Hodo_FADC->size(); ++i){
        FADCdata *hit = &Hodo_FADC->data[i];
        hit_histos[i]->Clear();
        cc->cd(i);
        printf("%d - sz= %d \n",i,(int)hit->GetSampleSize());
        for(int j=0;j<hit->GetSampleSize();++j){
          printf("%d ",hit->GetSample(j));
          hit_histos[i]->SetBinContent(j,hit->GetSample(j));
        }
        printf("\n");
        hit_histos[i]->Draw();
      }
      cc->Update();
      cc->Draw();
      printf("\n");
      if(args.print_evt) {
        etool->PrintBank(10);
  //      if(args.show_head) {};
  //      if(args.show_svt)  {};
  //      if(args.show_ecal) {};
      }
      if(!args.quiet && evt_count%100000 ==0 ){
  //      /* statistics */
          auto time2 = std::chrono::system_clock::now();
          std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
          totalTime += delta_t;
          double rate = 1000000.0 * ((double) evt_count) / delta_t.count();
          totalCount += evt_count;
          double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
          printf("%s: %6.1f kHz,  %6.1f kHz Avg. Event: %9d\n", argv[0], rate/1000., avgRate/1000.,head->GetEventNumber());
          evt_count = 0;
          time1 = std::chrono::system_clock::now();
      }
    }
    cout << " ------------- \n";
    etool->Close();
  }
  auto time2 = std::chrono::system_clock::now();
  std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
  totalTime += delta_t;
  totalCount += evt_count;
  double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
  printf("Last event: %6d\n",head->GetEventNumber());
  printf("Total events: %6ld \n",totalCount);
  printf("Final: %3.4g kHz \n", avgRate/1000.);
  root_file.Write();
  root_file.Close();
  return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Argument parsing support routine.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Parse_Args(int *argc,const char **argv, Arguments_t *p_arg){
  // Read and parse all the options, leaving only input files in the
  // argv array.
  // This version, all options flags are globals.
#define REMOVE_ONE {(*argc)--;for(j=i;j<(*argc);j++)argv[j]=argv[j+1];i--;}
#define I_PLUS_PLUS if((i+1)<(*argc)){i++;}else{break;}
  int  i,j;
  
  p_arg->debug=0;
  p_arg->quiet=0;
  p_arg->auto_add=false;
  p_arg->print_evt=false;
  p_arg->use_et=0;
  p_arg->filenames.clear();
  p_arg->et_name="";
  p_arg->et_host_name="";
  p_arg->et_port=0;
  p_arg->et_block=false;
  p_arg->show_head=false;
  p_arg->show_svt=false;
  p_arg->show_ecal=false;
  
  for(i=1;i<(*argc);i++){
    if(argv[i][0]=='-'){
      if(strcmp(argv[i],"-quiet")==0 || strcmp(argv[i],"-q")==0){
        p_arg->quiet=1;
      }else if(strcmp(argv[i],"-debug")==0 || strcmp(argv[i],"-d")==0){
        p_arg->debug++;
      }else if(strcmp(argv[i],"-auto")==0 || strcmp(argv[i],"-a")==0){
        p_arg->auto_add=true;
      }else if(strcmp(argv[i],"-print")==0 || strcmp(argv[i],"-P")==0){
        p_arg->print_evt=true;
      }else if(strcmp(argv[i],"-SVT")==0 || strcmp(argv[i],"-S")==0){
        p_arg->show_svt=true;
      }else if(strcmp(argv[i],"-ECAL")==0 || strcmp(argv[i],"-E")==0){
        p_arg->show_ecal=true;
      }else if(strcmp(argv[i],"-cont")==0 || strcmp(argv[i],"-c")==0){
        p_arg->show_head=true;
      }else if(strcmp(argv[i],"-block")==0 || strcmp(argv[i],"-b")==0){
        p_arg->et_block=true;
      }else if(strcmp(argv[i],"-et")==0 || strcmp(argv[i],"-etring")==0){
        p_arg->use_et=1;
      }else if(strcmp(argv[i],"-n")==0 || strcmp(argv[i],"-numevt")==0){
        I_PLUS_PLUS;
        long int ii;
        sscanf(argv[i],"%ld",&ii);
        //        G_N_Events = ii;
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-f")==0 || strcmp(argv[i],"-et_name")==0){
        I_PLUS_PLUS;
        p_arg->et_name=argv[i];
        //        G_N_Events = ii;
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-H")==0 || strcmp(argv[i],"-host")==0){
        I_PLUS_PLUS;
        p_arg->et_host_name =argv[i];
        //        G_N_Events = ii;
        REMOVE_ONE;
        
      }else if(strcmp(argv[i],"-p")==0 || strcmp(argv[i],"-et_port")==0){
        I_PLUS_PLUS;
        sscanf(argv[i],"%d",&p_arg->et_port);
        //        G_N_Events = ii;
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-help")==0||strcmp(argv[i],"-h")==0)
      {
        Print_Usage(argv[0]);
        exit(1);
      }
      else
      {
        fprintf(stderr,"\nI did not understand the option : %s\n",argv[i]);
        Print_Usage(argv[0]);
        exit(1);
      }
      /* KILL the option from list */
      REMOVE_ONE;
    }
  }

  if( (*argc) < 2){
    std::cout << "Please supply at least one EVIO file to parse.\n";
    exit(1);
  }

  for(int i=1;i<(*argc);++i){
    p_arg->filenames.push_back(argv[i]);
  }
  
  if(p_arg->debug){
    cout << "Debug set to: " << p_arg->debug << endl;
    if(p_arg->use_et){
      cout << "Opening a channel to the ET system." << endl;
    }else{
      std::cout << "File to open: ";
      for(auto f : p_arg->filenames){
        std::cout << f << " ";
      }
      std::cout << endl;
    }
  }
}

//***********************************************************************************
//  Print Usage
//***********************************************************************************

void Print_Usage(const char *name){
  cout << name << " <options>  EVIO_file " << endl;
  cout << endl << " Options: \n";
  cout << "  -q                 Quiet \n";
  cout << "  -d  -debug         Debug \n";
  cout << "  -et                Use ET ring \n";
  cout << "  -f  -et_name name  Attach ET to process with file <name>\n";
  cout << "  -H  -host    host  Attach ET to host\n";
  cout << "  -p  -et_port port  Attach ET to port \n";
  cout << "  -c  -cont          Show content of header and bank counts.\n";
  cout << "  -S  -SVT           Show content of SVT banks\n";
  cout << "  -E  -ECAL          Show contents of ECAL banks\n";
  cout << "  -a  -auto          Auto add all encountered banks.\n";
  cout << "  -P  -print         Print entire event. \n";
}


