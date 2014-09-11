// evioException.cc

//  Author:  Elliott Wolin, JLab, 18-feb-2010


#include "evioException.hxx"

using namespace std;
using namespace evio;


namespace evio {

  /** 
   * Returns stack trace.
   * @return String containing stack trace
   */
  string getStackTrace() {

#ifndef linux
    return("\n\n   <backtrace not supported on this platform>\n\n");
#else
    size_t dlen = 1024;
    char *dname = (char*)malloc(dlen);
    void *trace[1024];
    int status;


    // get trace messages
    int trace_size = backtrace(trace,1024);
    if(trace_size>1024)trace_size=1024;
    char **messages = backtrace_symbols(trace, trace_size);
    
    // demangle and create string
    stringstream ss;
    for(int i=0; i<trace_size; ++i) {
      
      // find first '(' and '+'
      char *ppar = strchr(messages[i],'(');
      char *pplus = strchr(messages[i],'+');
      if((ppar!=NULL)&&(pplus!=NULL)) {
        
        // replace '+' with nul, then get demangled name
        *pplus='\0';
        abi::__cxa_demangle(ppar+1,dname,&dlen,&status);
        
        // add to stringstream
        *(ppar+1)='\0';
        *pplus='+';
        ss << "   " << messages[i] << dname << pplus << endl;

      } else {
        ss << "   " << messages[i] << endl;
      }

    }
      
    free(dname);
    free(messages);
    return(ss.str());
#endif

  }
}


//-----------------------------------------------------------------------
//-------------------------- evioException ------------------------------
//-----------------------------------------------------------------------

/**
 * Constructor.
 * @param typ User-defined exception type
 * @param txt Basic text
 * @param aux Auxiliary text
 */
evioException::evioException(int typ, const string &txt, const string &aux) 
  : type(typ), text(txt), auxText(aux), trace(getStackTrace()) {
}


//--------------------------------------------------------------


/**
 * Constructor.
 * @param typ Exception type user-defined
 * @param txt Basic exception text
 * @param file __FILE__
 * @param func __FUNCTION__ (not on sun)
 * @param line __LINE__
 */
evioException::evioException(int typ, const string &txt, const string &file, const string &func, int line) 
  : type(typ), text(txt), trace(getStackTrace()) {

  ostringstream oss;
  oss <<  "    evioException occured in file " << file << ", function " << func << ", line " << line;
  auxText=oss.str();
}


//--------------------------------------------------------------


/**
 * Returns XML string listing exception object contents.
 * @return XML string listing contents
 */
string evioException::toString(void) const throw() {
  ostringstream oss;
  oss << "?evioException type = " << hex << "0x" << type << "    text = " << text << endl << endl << auxText << dec;
  if(trace.size()>0) oss << endl << endl << endl << "Stack trace:" << endl << endl << trace << endl;
  return(oss.str());
}


//-----------------------------------------------------------------------


/**
 * Returns char * listing exception object contents.
 * @return char * listing contents
 */
const char *evioException::what(void) const throw() {
  return(toString().c_str());
}


