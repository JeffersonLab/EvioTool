//
//  Arguments.h
//  EvioTool
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef Arguments_h
#define Arguments_h

#include <iostream>
#include <vector>
#include <cstring>
#include <string>
using namespace std;

struct Arguments_t {
  vector<string> filenames;
  string trigger_name="FEE";
  bool   exclusive=false;
  string output_name="";
  string et_name;
  string et_host_name;
  int    et_port=0;
  int    debug=0;
  bool   analyze=false;
  bool   nooutput=false;
  int    quiet=0;
  bool   use_et=0;
  bool   et_block=false;
  bool   show_head=false;
  bool   show_svt=false;
  bool   show_ecal=false;
  bool   print_evt=false;
  bool   auto_add=false;

  void Print_Usage(string name=""){
    cout << name << " <options>  EVIO_file \n";
    cout << "\n Options: \n";
    cout << "  -q                 Quiet \n";
    cout << "  -a  -analyze       Analyze triggers in addition to fileter.\n";
    cout << "  -x  -nooutput      Do not write an output file, analyze only.\n";
    cout << "  -d  -debug         Debug \n";
    cout << "  -o  -output  name  Output file. (If not provided, output is <infile>_FEE.evio)\n";
    cout << "  -T  -trigger name -bits bitpat  Filter on trigger name (default: FEE) or bit pattern.\n";
    cout << "                     Bit pattern is an integer ie: 16 or 0x10 or 0b010000000 \n";
    cout << "  -E  -exclusive     Use exclusive filtering = only the exact bit pattern passes.\n";
    cout << "  -et                Use ET ring \n";
    cout << "  -f  -et_name name  Attach ET to process with file <name>\n";
    cout << "  -H  -host    host  Attach ET to host\n";
    cout << "  -p  -et_port port  Attach ET to port \n";
    cout << "\n\n";
    cout << "NOTES: \n";
    cout << "You cannot use both the -T and -B switches to set the bit pattern. -B will override -T.\n";
    cout << "For the -analyze switch, this code will count the triggers of each type and print\n";
    cout << "the result to the screen, including the total number of events processed.\n";
    cout << "This can be used with the -x switch if you want only to analyze. \n";
    cout << "NOTE: The trigger analyzer will count one event with two trigger bits set once for each bit,\n";
    cout << "so the number of triggers counted will be larger than the number of events.\n";
    
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
        }else if(strcmp(argv[i],"-analyze")==0 || strcmp(argv[i],"-a")==0){
          analyze=true;
        }else if(strcmp(argv[i],"-nooutput")==0 || strcmp(argv[i],"-x")==0){
          nooutput=true;
        }else if(strcmp(argv[i],"-debug")==0 || strcmp(argv[i],"-d")==0){
          debug++;
        }else if(strcmp(argv[i],"-o")==0 || strcmp(argv[i],"-output")==0){
          I_PLUS_PLUS;
          output_name=argv[i];
          //        G_N_Events = ii;
          REMOVE_ONE;
        }else if(strcmp(argv[i],"-T")==0 || strcmp(argv[i],"-trigger")==0 ||
                 strcmp(argv[i],"-bits")==0){
          I_PLUS_PLUS;
          trigger_name = argv[i];
          //        G_N_Events = ii;
          REMOVE_ONE;
        }else if(strcmp(argv[i],"-E")==0 || strcmp(argv[i],"-exclusive")==0){
          exclusive = true;
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
      std::cout << "Please supply at least one EVIO file for input.\n";
      exit(1);
    }
    
    for(int i=1;i<(*argc);++i){
      filenames.push_back(argv[i]);
    }
    
    if(debug){
      cout << "Debug set to: " << debug << endl;
      if(use_et){
        cout << "Opening a channel to the ET system. Won't work. Not yet implemente. Blame Maurik." << endl;
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
