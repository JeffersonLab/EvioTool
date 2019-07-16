//
//  Arguments.h
//  EvioTool
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef Arguments_h
#define Arguments_h

struct Arguments_t {
  vector<string> filenames;
  string trigger_config_file;
  string et_name;
  string et_host_name;
  int    et_port=0;
  int    debug=0;
  int    quiet=0;
  bool   use_et=0;
  bool   et_block=false;
  bool   show_head=false;
  bool   show_svt=false;
  bool   show_ecal=false;
  bool   print_evt=false;
  bool   auto_add=false;

  void Print_Usage(string name=""){
    cout << name << " <options>  EVIO_file " << endl;
    cout << endl << " Options: \n";
    cout << "  -q                 Quiet \n";
    cout << "  -d  -debug         Debug \n";
    cout << "  -et                Use ET ring \n";
    cout << "  -f  -et_name name  Attach ET to process with file <name>\n";
    cout << "  -H  -host    host  Attach ET to host\n";
    cout << "  -p  -et_port port  Attach ET to port \n";
    cout << "  -T  -trigger file  Use file for trigger config file instead of waiting for a '17' event.";
    cout << "  -c  -cont          Show content of header and bank counts.\n";
    cout << "  -S  -SVT           Show content of SVT banks\n";
    cout << "  -E  -ECAL          Show contents of ECAL banks\n";
    cout << "  -a  -auto          Auto add any unknown banks. Slows things down.\n";
    cout << "  -P  -print         Print entire event. \n";
  };
  void Parse_Args(int *argc, const char **argv){
    
    // Read and parse all the options, leaving only input files in the
    // argv array.
    // This version, all options flags are globals.
#define REMOVE_ONE {(*argc)--;for(j=i;j<(*argc);j++)argv[j]=argv[j+1];i--;}
#define I_PLUS_PLUS if((i+1)<(*argc)){i++;}else{break;}
    int  i,j;
    
    for(i=1;i<(*argc);i++){
      if(argv[i][0]=='-'){
        if(strcmp(argv[i],"-quiet")==0 || strcmp(argv[i],"-q")==0){
          quiet=1;
        }else if(strcmp(argv[i],"-debug")==0 || strcmp(argv[i],"-d")==0){
          debug++;
        }else if(strcmp(argv[i],"-print")==0 || strcmp(argv[i],"-P")==0){
          print_evt=true;
        }else if(strcmp(argv[i],"-auto")==0 || strcmp(argv[i],"-a")==0){
          auto_add=true;
        }else if(strcmp(argv[i],"-SVT")==0 || strcmp(argv[i],"-S")==0){
          show_svt=true;
        }else if(strcmp(argv[i],"-ECAL")==0 || strcmp(argv[i],"-E")==0){
          show_ecal=true;
        }else if(strcmp(argv[i],"-cont")==0 || strcmp(argv[i],"-c")==0){
          show_head=true;
        }else if(strcmp(argv[i],"-block")==0 || strcmp(argv[i],"-b")==0){
          et_block=true;
        }else if(strcmp(argv[i],"-et")==0 || strcmp(argv[i],"-etring")==0){
          use_et=1;
        }else if(strcmp(argv[i],"-n")==0 || strcmp(argv[i],"-numevt")==0){
          I_PLUS_PLUS;
          long int ii;
          sscanf(argv[i],"%ld",&ii);
          //        G_N_Events = ii;
          REMOVE_ONE;
        }else if(strcmp(argv[i],"-f")==0 || strcmp(argv[i],"-et_name")==0){
          I_PLUS_PLUS;
          et_name=argv[i];
          //        G_N_Events = ii;
          REMOVE_ONE;
        }else if(strcmp(argv[i],"-H")==0 || strcmp(argv[i],"-host")==0){
          I_PLUS_PLUS;
          et_host_name =argv[i];
          //        G_N_Events = ii;
          REMOVE_ONE;
          
        }else if(strcmp(argv[i],"-p")==0 || strcmp(argv[i],"-et_port")==0){
          I_PLUS_PLUS;
          sscanf(argv[i],"%d",&et_port);
          //        G_N_Events = ii;
          REMOVE_ONE;
        }else if(strcmp(argv[i],"-T")==0 || strcmp(argv[i],"-trigger")==0){
          I_PLUS_PLUS;
          trigger_config_file = argv[i];
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
      filenames.push_back(argv[i]);
    }
    
    if(debug){
      cout << "Debug set to: " << debug << endl;
      if(use_et){
        cout << "Opening a channel to the ET system." << endl;
      }else{
        std::cout << "File to open: ";
        for(auto f : filenames){
          std::cout << f << " ";
        }
        std::cout << endl;
      }
    }
  }
};


#endif /* Arguments_h */
