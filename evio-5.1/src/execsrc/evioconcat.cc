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

int main (int argc, char **argv)
{
  int c;
  string outfile="evioconcat.evio";
  vector<string> infile_list;
  int output_handle;
  int input_handle;
  int status;
  int event_total=0;
  
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
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    
    c = getopt_long (argc, argv, "o:h",
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
        break;
        
      default:
        cout << "evioconcat <options> file1 file2 ... filen \n\n";
        cout << "Options: \n";
        cout << "  --outfile  (-o)  : Specify the output file name. [evioconcat.evio] \n";
        cout << "  --debug          : Set debug flag to 2\n";
        cout << "  --quiet          : Be really quiet about all this.\n";
        exit (0);
    }
  }
  
  /* Instead of reporting ‘--verbose’
   and ‘--brief’ as they are encountered,
   we report the final status resulting from them. */
  if (debug_flag)
    cout << "Debug flag is set to "<< debug_flag << endl;

  /* open evio output file */
  if((status=evOpen(const_cast<char *>(outfile.c_str()),const_cast<char *>("w"),&output_handle))!=0) {
    cout << "Unable to open output file " << outfile << " status=" << status << endl;
    exit(EXIT_FAILURE);
  }

  /* The remaining command line arguments are the input files. */
  if (optind < argc){
    while (optind < argc){
        /* open evio input file */
        if((status=evOpen(const_cast<char *>(argv[optind]),const_cast<char *>("r"),&input_handle))!=0) {
          cout << "Unable to open input file " << argv[optind] << ", status=" << status << endl;
          exit(EXIT_FAILURE);
        }
      
      // File copy
      int nevent=0;
      int nwrite=0;
      while ((status=evReadNoCopy(input_handle,&buf,&bufsize))==0) {
        nevent++;
        event_total++;
        nwrite++;
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
