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

struct Arguments_t {
   string filename;
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

   auto etool = new EvioTool();

   int stat = 0;
   if(args.use_et){
      stat = etool->OpenEt("EvioToolTest", args.et_name, args.et_host_name, args.et_port);
   }else{
      stat = etool->Open(args.filename.c_str());
   }
   if( stat != 0 ){
      cout << "Error opening data. \n";
      return(3);
   }
   etool->tag_masks[0]=0;

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

   // Setup the Event structure
   etool->fAutoAdd=false;
   etool->fChop_level=1;
//  etool->tags={136,132,130,129};  // Parse HPS physics events only.
   Leaf<unsigned int> *Header = etool->AddLeaf<unsigned int>("Header",49152,0,"Header bank");
//   Bank *ECAL = etool->AddBank("Ecal",{37,39},0,"Ecal banks");
//   Leaf<FADCdata> *FADC = ECAL->AddLeaf<FADCdata>("FADC",57601,0,"FADC mode 1 data");
//   Bank *SVTraw = etool->AddBank("SVT",{51,52,53,54,55,56,57,58,59,60,61,62,63,64,65},0,"SVT banks");
//   Leaf<unsigned int> *SVTint = SVTraw->AddLeaf<unsigned int>("SVTint",3,0,"SVT unparsed");

   etool->fAutoAdd = args.auto_add;

   cout << "Debug set to " << etool->fDebug << " Auto add = " << etool->fAutoAdd << endl;

//   etool->PrintBank(5);

   long evt_count=0;
   long totalCount=0;
   std::chrono::microseconds totalTime(0);

   auto start = std::chrono::system_clock::now();
   auto time1 = start;

   bool keep_reading_events = true;
   while(keep_reading_events){
      int stat = etool->Next();
      if(stat != S_SUCCESS){
         if(stat == ET_ERROR_READ) {
            cout << "Event with bad magic number. ";
            continue;
         }else{
            break;
         }
      }
      if(args.debug>2) cout<<"EVIO Event " << evt_count << " Data event: " << Header->data[0] << endl;
      evt_count++;
      if(args.debug>2) cout << "Header: size= " << Header->Size() << endl;
      if(args.print_evt) {
         etool->PrintBank(10);
      }
      if(!args.quiet && evt_count%100000 ==0 ){
//      /* statistics */
         auto time2 = std::chrono::system_clock::now();
         std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
         totalTime += delta_t;
         double rate = 1000000.0 * ((double) evt_count) / delta_t.count();
         totalCount += evt_count;
         double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
         if(Header->size()>0) {
            printf("%s: %3.4g kHz,  %3.4g kHz Avg. Event: %6d\n", argv[0], rate / 1000., avgRate / 1000.,
                   Header->data[0]);
         }else{
            printf("%s: %3.4g kHz,  %3.4g kHz Avg. Event: n/a\n", argv[0], rate / 1000., avgRate / 1000.);
         }
         evt_count = 0;
         time1 = std::chrono::system_clock::now();
      }
   }
   auto time2 = std::chrono::system_clock::now();
   std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
   totalTime += delta_t;
   totalCount += evt_count;
   double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
   if( Header->data.size()>0)
      printf("Last event: %6d\n",Header->data[0]);
   printf("Final: %3.4g kHz \n", avgRate/1000.);
   return 0;
}

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
   p_arg->filename="";
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
   if( (*argc) != 2 && !p_arg->use_et){
      fprintf(stderr,"\nPlease supply one and only one file name\n");
      exit(1);
   }
   if((*argc) >= 2) p_arg->filename = argv[1];
   if(p_arg->debug){
      cout << "Debug set to: " << p_arg->debug << endl;
      if(p_arg->use_et){
         cout << "Opening a channel to the ET system." << endl;
      }else{
         cout << "File to open: " << p_arg->filename << endl;
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


