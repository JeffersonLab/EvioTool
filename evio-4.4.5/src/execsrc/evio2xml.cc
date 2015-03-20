// evio2xml.cc

// reads and dumps EVIO buffers in XML format
// uses dictionary

// still to do:
//   add command line args from evio2xml.c
//   i.e. modify toString to accept more config options

//  ejw, 15-Feb-2012



#include "evioUtil.hxx"
#include "evioFileChannel.hxx"
#include <fstream>


using namespace std;
using namespace evio;


string filename;
string dictFileName;
string dictFileXML;
evioDictionary *dictFile = NULL;

bool debug          = false;
bool no_dump        = false;
bool done           = false;
int evcount         = 0;
int wordcount       = 0;
int max_event       = 0;
int skip            = 0;
bool dumpDict       = false;
bool evioPause      = false;
int maxbuf          = 100000;   // ??? no error


// set in toStringConfig()
bool xtod           = false;
bool no_data        = false;
int max_depth       = 0;
int indent          = 0;
bool verbose        = false;


// for selectively dumping banks
vector<uint16_t> bankOk;
vector<uint16_t> noBank;
vector<string> bankNameOk;
vector<string> noBankName;


// misc prototypes
void decode_command_line(int argc, char**argv);



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  

  int bufferCount = 0;
  evioFileChannel *chan;


  // decode command line arguments
  decode_command_line(argc,argv);
  if(filename.empty())filename="fakeEvents.dat";


  try {

    // read dictionary file if specified on command line, overrides dictionary in data file
    if(!dictFileName.empty()) {
      ifstream ifs(dictFileName.c_str());
      dictFile = new evioDictionary(ifs);
    }


    // create and open channel using dictionary if specified
    chan = new evioFileChannel(filename,dictFile,"r",maxbuf);
    chan->open();


    // check for conflict
    string fileDictXML = chan->getFileXMLDictionary();
    if(!dictFileName.empty()) {
      
      if(!fileDictXML.empty()) cout << endl << "<!-- *** Using user-specified dictionary from file named " << dictFileName 
                                    << ", not using dictionary in input data file *** -->" << endl << endl;
      
      if(dumpDict) cout << endl << "<!-- dictionary from file " << dictFileName << endl << dictFile->getDictionaryXML() 
                        << endl << "-->" << endl << endl;
      
    } else if(!fileDictXML.empty()) {
      
      cout << endl << "<!-- *** Using dictionary found in input data file *** -->" << endl << endl;
      if(dumpDict) cout << endl << "<!-- dictionary from input data file" << endl << fileDictXML << endl << "-->" << endl << endl;
    }


    // create toStringConfig to implement command-line options
    evioToStringConfig config;
    config.noData     = no_data;
    config.maxDepth   = max_depth;
    config.bankNameOk = bankNameOk;
    config.noBankName = noBankName;
    config.bankOk     = bankOk;
    config.noBank     = noBank;
    config.verbose    = verbose;
    config.xtod       = xtod;


    // dump data
    if(!no_dump)cout << "<!-- xml boilerplate needs to go here -->\n\n<evio-data>\n"; 


    // loop over all buffers in file
    if(debug)cout << "file open, about to read event" << endl;
       while(!done && ((max_event<=0)||(evcount<max_event)) && chan->read()) {
      evcount++;
      if((skip>0)&&(evcount<=skip))continue;


      // create event tree from channel contents
      if(debug)cout << "event read, about to make tree" << endl;
      evioDOMTree tree(chan);
      
      
      // dump tree
      if(debug)cout << "tree created, about to dump" << endl;
      bufferCount++;
      wordcount+=(chan->getBuffer())[0]+1;
      if(!no_dump) {
          cout << "\n\n<!-- ===================== Buffer " << bufferCount << " contains " << (chan->getBuffer())[0]+1 << " words (" 
               << (chan->getBuffer())[0]*4+4 <<  " bytes) ===================== -->\n";
        cout << tree.toString(config);
        cout << "<!-- ================== end buffer " << bufferCount << " containing " << (chan->getBuffer())[0]+1 << " words  ================== -->\n\n";
        if(debug)cout << "dump complete" << endl;
      }
      
      
      if(!no_dump && evioPause) {
        char s[100];
        cout << endl << "Hit return to continue, q to quit: ";
        fgets(s,sizeof(s),stdin);
        done=tolower(s[strspn(s," \t")])=='q';
      }
    }


    // done...close channel
    if(!no_dump)cout << "</evio-data>\n\n"; 
    chan->close();
    delete(chan);
    cout << endl << "<!-- total events read(skipped) is " << evcount << "(" << skip << "), total words dumped is " 
         << wordcount << " -->" << endl << endl;



  } catch (evioException e) {
    cerr << e.toString() << endl;

  } catch (...) {
    cerr << "?unknown exception" << endl;
  }
  
}


//-------------------------------------------------------------------------------


void decode_command_line(int argc, char**argv) {

  static string help = 
    "\nUsage:\n\n  evio2xml [-max max_event] [-pause] [-skip skip_event]\n"
    "           [-dict dictfilename] [-dumpDict]\n"
    "           [-xtod] [-max_depth max_depth] [-no_data] [-no_dump]\n"
    "           [-indent indent_size] [-maxbuf maxbuf] [-verbose] [-debug]\n"
    "           [-bankTag bankTag] [-noBankTag bankTag] [-bankName bankName] [-noBankName bankName]\n"
    "           filename\n";
    

  if(argc<2) {
    cout << help << endl;
    exit(EXIT_SUCCESS);
  } 


  /* loop over arguments */
  int i=1;
  while (i<argc) {
    if (strncasecmp(argv[i],"-h",2)==0) {
      cout << help << endl;
      exit(EXIT_SUCCESS);

    } else if (strncasecmp(argv[i],"-pause",6)==0) {
      evioPause=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-maxbuf",7)==0) {
      maxbuf=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-dumpDict",9)==0) {
      dumpDict=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-verbose",8)==0) {
      verbose=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-debug",6)==0) {
      debug=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-no_data",8)==0) {
      no_data=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-no_dump",8)==0) {
      no_dump=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-max_depth",10)==0) {
      max_depth=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-indent",7)==0) {
      indent=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-max",4)==0) {
      max_event=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-skip",5)==0) {
      skip=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-dict",5)==0) {
      dictFileName=string(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-xtod",5)==0) {
      xtod=true;
      i=i+1;

    } else if (strncasecmp(argv[i],"-banktag",8)==0) {
      bankOk.push_back(atoi(argv[i+1]));
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-nobanktag",10)==0) {
      noBank.push_back(atoi(argv[i+1]));
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-bankname",9)==0) {
      bankNameOk.push_back(argv[i+1]);
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-nobankname",11)==0) {
      noBankName.push_back(argv[i+1]);
      i=i+2;
      
    } else if (strncasecmp(argv[i],"-",1)==0) {
      cout << "\n  ?unknown command line arg: %s\n\n" << argv[i] << endl;
      exit(EXIT_FAILURE);
      
    } else {
      break;
    }

   }
  
//   /* last arg better be filename */
  filename=argv[argc-1];

  return;
}

//-------------------------------------------------------------------------------
