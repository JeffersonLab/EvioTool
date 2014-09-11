//  example program reads and dumps DANA event data from evio files

//  ejw, 4-may-2010




#include "evioUtil.hxx"
#include "evioFileChannel.hxx"
#include <vector>

using namespace evio;
using namespace std;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  

  try {

    evioFileChannel *chan = new evioFileChannel(argv[1],"r");
    chan->open();
    

    // loop over all events in file
    while(chan->read()) {


      // create event tree from channel contents
      evioDOMTree event(chan);


      // get list of bank pointers with a particular tag and num
      // see dictionary for complete list (10300 is DMCThrown)
      evioDOMNodeListP bList   = event.getNodeList(tagNumEquals(10300,0));


      // loop over all banks in list (should only be one DMCThrown bank)
      evioDOMNodeList::const_iterator iter1;
      for(iter1=bList->begin(); iter1!=bList->end(); iter1++) {

        // get list of child banks
        evioDOMNodeList *childList = (*iter1)->getChildList();

        // loop over all child banks and dump some info
        evioDOMNodeList::const_iterator iter2;
        for(iter2=childList->begin(); iter2!=childList->end(); iter2++) {
          
          cout << "bank type,tag,num are: " << hex << "  0x" << (*iter2)->getContentType() << dec 
               << "  "  << (*iter2)->tag << "  " << (int)((*iter2)->num) << endl;
          
          // dump float data
          const vector<float> *vec = (*iter2)->getVector<float>();
          if(vec!=NULL) {
            for(unsigned int i=0; i<vec->size(); i++) cout  << "   " << (*vec)[i];
            cout << endl;
          } else {
            cout << "  ...bank does not contain floats" << endl;
          }
        }
        
      }
    }
      
    // done...close channel
    chan->close();
    delete(chan);
    
    
  } catch (evioException e) {
    cerr << e.toString() << endl;
  } catch (...) {
    cerr << "?unknown exception" << endl;
  }
  
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
