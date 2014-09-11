// evioCat.cc

// concatenates multiple evio input files into one output file

// still to do:
//  what about dictionaries?

//  ejw, 16-Aug-2012


#include <string>
#include <queue>

#include "evioUtil.hxx"
#include "evioFileChannel.hxx"


using namespace std;
using namespace evio;


void decode_command_line(int argc, char**argv);

static int maxev             = 0;
static int maxevFile         = 0;
static bool debug            = false;
static bool done             = false;
static int maxbuf            = 250000;
static int nfile             = 0;
static int nevent            = 0; 
static int neventFile        = 0; 
static int nword             = 0;
static int nwordFile         = 0;
static string outputFileName = "eviocat.evio";

static queue<string> inputFileNames;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  
  int length;


  // decode command line arguments
  decode_command_line(argc,argv);


  try {

    // open output file
    evioFileChannel out(outputFileName,"w",maxbuf);
    out.open();
    

    // loop over input files
    while(!done&&!inputFileNames.empty()) {

      // open next input file
      string fileName = inputFileNames.front();
      if(debug)cout << "...opening file " << fileName << endl;
      inputFileNames.pop();
      evioFileChannel in(fileName,"r",maxbuf);
      in.open();
      
      // loop over events, copy to output
      neventFile=0;
      nwordFile=0;
      while(!done&&in.readNoCopy()) {
        length=(in.getNoCopyBuffer())[0]+1;
        nevent++;
        neventFile++;
        nword += length;
        nwordFile += length;
        if(debug)cout << "writing event " << nevent << ", nwords is " << length << endl;
        out.write(in.getNoCopyBuffer());
        if((maxevFile>0)&&(neventFile>=maxevFile))break;
        if((maxev>0)&&(nevent>=maxev))done=true;
      }

      // close file
      if(debug)cout << "...closing file " << fileName << " after " << neventFile << " events, " << nwordFile << " words" << endl;
      in.close();
    }


    // close output file
    out.close();

  } catch (evioException e) {
    cerr << e.toString() << endl;

  } catch (...) {
    cerr << "?unknown exception" << endl;
  }
  

  cout << endl << endl << " *** Copied " << nevent << " events from " << nfile << " files, total words " << nword << " ***" << endl << endl;
}


//-------------------------------------------------------------------------------


void decode_command_line(int argc, char**argv) {
  
  string help = 
    "\nusage:\n\n  evioCat [-maxev maxEvent] [-maxevFile maxEventFile] [-debug] -o outputFile  file1 file2 file3 ...\n";
  int i;
    
    
  if(argc<2) {
    cout << help << endl;
    exit(EXIT_SUCCESS);
  } 


  /* loop over arguments */
  i=1;
  while (i<argc) {
    if (strncasecmp(argv[i],"-h",2)==0) {
      cout << help << endl;
      exit(EXIT_SUCCESS);
      
    } else if (strncasecmp(argv[i],"-maxevfile",10)==0) {
      maxevFile=atoi(argv[i+1]);
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-maxev",6)==0) {
      maxev=atoi(argv[i+1]);
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-debug",6)==0) {
      debug=true;
      i=i+1;
      
    } else if (strncasecmp(argv[i],"-maxbuf",7)==0) {
      maxbuf=atoi(argv[i+1]);
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-o",2)==0) {
      outputFileName = argv[i+1];
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-",1)!=0) {
      inputFileNames.push(argv[i]);
      i=i+1;
      
    } else {
      cout << "\n  ?unknown command line arg: %s\n\n" << argv[i] << endl;
      exit(EXIT_FAILURE);
    }
  }


  // check for input files
  nfile=inputFileNames.size();
  if(nfile<=0) {
    cout << endl << "?no input files specified" << endl << endl << help << endl;
    exit(EXIT_SUCCESS);
  }


  return;
}

//-------------------------------------------------------------------------------
