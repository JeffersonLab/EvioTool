//  example program reads and processes EVIO events 

//  ejw, 14-nov-2006




#include "evioUtil.hxx"
#include "evioFileChannel.hxx"
#include <vector>

using namespace evio;
using namespace std;



//-------------------------------------------------------------------------------


void myProcessingFunction(const evioDOMNodeP pNode) {

  // can implement arbitrary node processing algorithm here
  cout << hex << left << "content type:  0x" << setw(6) << pNode->getContentType() << "   tag:  0x" << setw(6) << pNode->tag 
       << "   num:  0x" << setw(6) << pNode->num << endl;
  cout << pNode->toString() << endl;
}


//-------------------------------------------------------------------------------


bool myNodeChooser(const evioDOMNodeP pNode) {

  // can implement arbitrary selection criteria here
  return((pNode->tag==2)&&(pNode->num==9));
}


//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  


  int nread=0;
  try {
    evioFileChannel *chan;

    if(argc>1) {
      chan = new evioFileChannel(argv[1],"r");
    } else {
      chan = new evioFileChannel("fakeEvents.dat","r");
    }
    chan->open();
    

    // check for dictionary
    const evioDictionary *dict = chan->getDictionary();
    if(dict!=NULL) {
      cout << endl << "Dictionary is:" << endl << endl << dict->getDictionaryXML() << endl;
    } else {
      cout << endl << "No dictionary in file" << endl << endl;
    }


    // loop over all events in file
    // uint32_t *buffer;
    // int len;
    // while(chan->readAlloc(&buffer,&len)) {
    while(chan->readNoCopy()) {

      cout << endl << " --- processing event " << ++nread << " ---" << endl;


      // create event tree from channel contents
      //      evioDOMTree event(buffer);
      evioDOMTree event(chan);


      // dump event
      cout << endl << "Dumping event:" << endl;
      cout << event.toString() << endl;



      // get lists of pointers to various types of nodes in event tree
      // note:  there are many functions available to select nodes, and it is easy to 
      //        write additional ones...contact EJW or see the EVIO manual for more information
      evioDOMNodeListP fullList     = event.getNodeList();
      evioDOMNodeListP uintList     = event.getNodeList(typeIs<uint32_t>());
      evioDOMNodeListP intList      = event.getNodeList(typeIs<int32_t>());
      evioDOMNodeListP floatList    = event.getNodeList(typeIs<float>());
      evioDOMNodeListP doubleList   = event.getNodeList(typeIs<double>());
      evioDOMNodeListP myList       = event.getNodeList(myNodeChooser);
      evioDOMNodeListP int64List    = event.getNodeList(typeIs<int64_t>());
      evioDOMNodeListP stringList   = event.getNodeList(typeIs<string>());


      // test getFirstNode()
      evioDOMNodeP n = event.getFirstNode(tagNumEquals(6,13));
      if(n!=NULL) {
        cout << "Found node:" << endl << endl;
        cout << n->toString() << endl;
      } else {
        cout << "?unable to find string node" << endl;
      }



      // apply myProcessingFunction to all float nodes using STL for_each() algorithm
      cout << endl << endl << "Applying myProcessingFunction to all float nodes:" << endl << endl;
      for_each(floatList->begin(),floatList->end(),myProcessingFunction);
      

      // dump all double nodes to cout using toCout()
      cout << endl << endl<< "Dumping double nodes using toCout:" << endl << endl;
      for_each(doubleList->begin(),doubleList->end(),toCout());


      // dump all nodes selected by myNodeChooser
      cout << endl << endl<< "Dumping nodes selected by myNodeChooser using toCout:" << endl << endl;
      for_each(myList->begin(),myList->end(),toCout());


      // print out data from all <uint32_t> nodes manually
      cout << endl << endl<< "Dumping uint nodes manually:" << endl << endl;
      evioDOMNodeList::const_iterator iter;
      for(iter=uintList->begin(); iter!=uintList->end(); iter++) {
        cout << "bank type,tag,num are: " << hex << "  0x" << (*iter)->getContentType() << dec << "  "  << (*iter)->tag
             << "  " << (int)((*iter)->num) << endl;

        const evioDOMNodeP np = *iter;                              // solaris bug...
        const vector<uint32_t> *vec = np->getVector<uint32_t>();
        if(vec!=NULL) {
          for(unsigned int i=0; i<vec->size(); i++) cout  << "   " << (*vec)[i];
          cout << endl;
        } else {
          cerr << "?getVector<uint32_t> returned NULL" << endl;
        }
      }
      cout << endl << endl;
      
      
      // print out data from all <int32_t> nodes manually
      cout << endl << endl<< "Dumping int32_t nodes manually:" << endl << endl;
      for(iter=intList->begin(); iter!=intList->end(); iter++) {
        cout << "bank type,tag,num are: " << hex << "  0x" << (*iter)->getContentType() << dec << "  "  << (*iter)->tag
             << "  " << (int)((*iter)->num) << endl;

        const evioDOMNodeP np = *iter;                              // solaris bug...
        const vector<int32_t> *vec = np->getVector<int32_t>();
        if(vec!=NULL) {
          for(unsigned int i=0; i<vec->size(); i++) cout  << "   " << (*vec)[i];
          cout << endl;
        } else {
          cerr << "?getVector<int32_t> returned NULL" << endl;
        }
      }
      cout << endl << endl;
      
      
      // print out data from all int64_t nodes manually
      cout << endl << endl<< "Dumping int64_t nodes manually:" << endl << endl;
      for(iter=int64List->begin(); iter!=int64List->end(); iter++) {
        cout << "bank type,tag,num are: " << hex << "  0x" << (*iter)->getContentType() << dec << "  "  << (*iter)->tag
             << "  " << (int)((*iter)->num) << endl;

        const evioDOMNodeP np = *iter;                              // solaris bug...
        const vector<int64_t> *vec = np->getVector<int64_t>();
        if(vec!=NULL) {
          for(unsigned int i=0; i<vec->size(); i++) cout  << "   " << (*vec)[i];
          cout << endl;
        } else {
          cerr << "?getVector<int64_t> returned NULL" << endl;
        }
      }
      cout << endl << endl;
      
      
      // print out data from all string nodes manually
      // dump all double nodes to cout using toCout()
      cout << endl << endl<< "Dumping string nodes using toCout:" << endl << endl;
      for_each(stringList->begin(),stringList->end(),toCout());
      cout << endl << endl;
      
      
      // get child list from container node
      evioDOMNodeList *list = event.root->getChildList();
      if(list!=NULL) {
        cout << "Root child list length is " << list->size() << endl;
        cout << endl << endl;
      } else {
        cout << "Root node is not a container" << endl;
        cout << endl << endl;
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
