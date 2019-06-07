//
//  evioconcat.c
//  evio-5.1
//
//  Created by Maurik Holtrop on 6/7/19.
//

#include <stdio.h>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <vector>
#include "evio.h"

using namespace std;
int debug_flag=1;



int open_output_file(string filestub,int seq_number){
  /* open evio output file */
  int status;
  int output_handle;
  string outfile;
  
  outfile=filestub;
  outfile.append("_"+to_string(seq_number)+".evio");
  
  if((status=evOpen(const_cast<char *>(outfile.c_str()),const_cast<char *>("w"),&output_handle))!=0) {
    cout << "Unable to open output file " << outfile << " status=" << status << endl;
    exit(EXIT_FAILURE);
  }

  return(output_handle);
}


int main (int argc, char **argv)
{
  int c;
  string outfile="evioconcat.evio";
  vector<string> infile_list;
  int output_handle=0;
  int input_handle=0;
  int status;
  unsigned int event_total=0;
  unsigned int max_event=25000;
  unsigned int file_num=0;
  unsigned int max_file=1000000;

  uint32_t bufsize;
  const uint32_t *buf;
  
  while (1)
  {
    static struct option long_options[] =
    {
      /* These options set a flag. */
      {"debug",    no_argument,       &debug_flag, 2},
      {"quiet",    no_argument,       &debug_flag, 0},
      /* These options don’t set a flag.
       We distinguish them by their indices. */
      {"outfile",  required_argument, 0, 'o'},
      {"maxevt",   required_argument, 0, 'm'},
      {"maxfile",  required_argument, 0, 'M'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    
    c = getopt_long (argc, argv, "o:m:M:h",
                     long_options, &option_index);
    
    /* Detect the end of the options. */
    if (c == -1)
      break;
    
    switch (c)
    {
      case 0:
        cout << "Case 0:" << endl;
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;
        
      case 'o':
        outfile=optarg;
        // Remove a trailing .evio, if there is one.
        outfile=outfile.substr(0,outfile.find(".evio",outfile.find_last_of(".")));
        break;
        
      case 'm':
        max_event = stoi(optarg);
        break;

      case 'M':
        max_file = stoi(optarg);
        break;
        
        
      default:
        cout << "eviosplit <options> file1 file2 ... filen \n\n";
        cout << "This program will read the input file(s) and produce an set of output files with \n";
        cout << "with a maximum of mexevt events. The outputfiles will be named: outfile_xxx.evio \n\n";
        cout << "Options: (short option) \n";
        cout << "  --outfile name (-o)  : Specify the output file name base. [eviosplit] \n";
        cout << "  --maxevt  num  (-m)  : Maximum number of events in output file. \n";
        cout << "  --maxfile num  (-M)  : Maximum number of files that will be created.\n";
        cout << "  --debug              : Set debug flag to 2\n";
        cout << "  --quiet              : Be really quiet about all this, set debug flag to 0.\n";
        exit (0);
    }
  }
  
  /* Instead of reporting ‘--verbose’
   and ‘--brief’ as they are encountered,
   we report the final status resulting from them. */
  if (debug_flag)
    cout << "Debug flag is set to "<< debug_flag << endl;
  
  /* The remaining command line arguments are the input files. */
  if (optind < argc){
    int nwrite=0;

    output_handle = open_output_file(outfile,file_num);
    
    while (optind < argc){
        /* open evio input file */
        if((status=evOpen(const_cast<char *>(argv[optind]),const_cast<char *>("r"),&input_handle))!=0) {
          cout << "Unable to open input file " << argv[optind] << ", status=" << status << endl;
          exit(EXIT_FAILURE);
        }
      
      // File copy
      int nevent=0;
      while ((status=evReadNoCopy(input_handle,&buf,&bufsize))==0) {
        nevent++;
        event_total++;
        nwrite++;
        if(nwrite>max_event){
          evClose(output_handle);
          file_num++;
          if(file_num>=max_file){
            if(debug_flag) cout << "Reached maximum of " << max_file <<" output files.\n";
            evClose(input_handle);
            exit(0);
          }
          output_handle = open_output_file(outfile,file_num);
          if(debug_flag) cout << "Opened " << outfile <<"_"<<file_num<<".evio \n";
          nwrite=0;
        }
        status=evWrite(output_handle,buf);
        if(status!=0) {
          cout << "evWrite error output file "<<outfile << " status= " << status;
          exit(EXIT_FAILURE);
        }
        event_total++;
      }
      
      /* done */
      if(debug_flag) cout << "File: " << argv[optind] << ", read  " << nevent << " events, copied " << nwrite << " << events, total = " << event_total << endl;
      evClose(input_handle);
      
      optind++;
      }
    evClose(output_handle);
  }else{
    cout << "Please specify input files. \n";
  }
  exit (0);
}
