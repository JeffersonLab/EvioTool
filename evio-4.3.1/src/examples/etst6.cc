//  tests serialization
//  demonstrates use of STL algorithms and EVIO function objects

//  ejw, 11-jan-2007



#include <evioUtil.hxx>
#include "evioFileChannel.hxx"
#include <stdio.h>
#include <algorithm>


using namespace evio;
using namespace std;



static vector<int32_t> i1,i2;



//--------------------------------------------------------------
//--------------------------------------------------------------


class sObject : public evioSerializable {

public:
  void serialize(evioDOMNodeP c) const throw(evioException) {
    for(int i=0; i<8; i++) i1.push_back(i),i2.push_back(10*i);

    *c << evioDOMNode::createEvioDOMNode(1,2,i1)
       << evioDOMNode::createEvioDOMNode(2,3,i2);
  }

};


//--------------------------------------------------------------
//--------------------------------------------------------------


//  misc functions
bool myMatcher(const evioDOMNodeP node) {
  return(
         ((node->tag==1)&&(node->num==2)) || 
         ((node->tag==2)&&(node->num==9))
         );
}


//--------------------------------------------------------------
//--------------------------------------------------------------


int main(int argc, char **argv) {

  int i,maxev,nevents;
  unsigned int ui;


  evioFileChannel *chan;

  if(argc>1) {
    chan = new evioFileChannel(argv[1],"r");
  } else {
    chan = new evioFileChannel("fakeEvents.dat","r");
  }
  chan->open();
    

  // get max events
  if(argc==3) {
    maxev = atoi(argv[2]);
    printf("Read at most %d events\n",maxev);
  } else {
    maxev = 0;
  }


  try {

    // test object serialization
    cout << endl << endl << "Testing evioSerializable:" << endl;
    sObject s;
    evioDOMNodeP c2  = evioDOMNode::createEvioDOMNode(55,66,s);
    evioDOMTree t(c2);
    cout << t.toString() << endl;
    cout << "evioSerializable test done" << endl << endl;
    

    // open file
    chan->open();
  
  
    // loop over events
    nevents=0;
    while(chan->readNoCopy()) {
      nevents++;
    
      // create tree from file channel
      evioDOMTree t(chan);
    
    
      // print tree
      cout << t.toString() << endl;
      

      // get lists of different types of node pointers
      evioDOMNodeListP pList                                         = t.getNodeList();
      evioDOMNodeListP pLeafList                                     = t.getNodeList(isLeaf());
      evioDOMNodeListP pContainerList                                = t.getNodeList(isContainer());
      evioDOMNodeListP pUint                                         = t.getNodeList(typeIs<unsigned int>());


      cout << endl << endl << dec << "There are " << pList->size() << " total nodes, "
           << pLeafList->size() << " leaf nodes, "
           << pContainerList->size() << " container nodes, and " << pUint->size() << " unsigned int nodes"
           << endl << endl << endl;



      // print information for various node types
      cout << endl << endl<< "Container nodes:"  << endl << endl;
      for_each(pContainerList->begin(),pContainerList->end(),toCout());

      cout << endl << endl << "Leaf nodes:"  << endl << endl;
      for_each(pLeafList->begin(),pLeafList->end(),toCout());

      cout << endl << endl << "Unsigned int nodes:"  << endl << endl;
      for_each(pUint->begin(),pUint->end(),toCout());



      // count leaf nodes containing floats
      cout << endl << "Number of leaf nodes of type 7 is " << 
        count_if(pLeafList->begin(),pLeafList->end(),typeEquals(7)) << endl << endl;


      // count leaf nodes not containing floats (using function adapter)
      cout << endl << "Number of leaf nodes NOT of type 7 is " << 
        count_if(pLeafList->begin(),pLeafList->end(),not1(typeEquals(7))) << endl << endl;
      

      // count nodes with particular tag,num
      cout << endl << "Number of nodes with tag 2, num 0 is " << 
        count_if(pList->begin(),pList->end(),tagNumEquals(2,0)) << endl << endl;
      

      // count nodes opposite from previous (using function adapter)
      cout << endl << "Number of nodes with other than tag 2, num 0 is " << 
        count_if(pList->begin(),pList->end(),not1(tagNumEquals(2,0))) << endl << endl;


      cout << endl << "Trying getNodeList(myMatcher)" << endl << endl;
      evioDOMNodeListP p1 = t.getNodeList(myMatcher);
      for_each(p1->begin(),p1->end(),toCout());
      cout << endl << "ending getNodeList(myMatcher)" << endl << endl << endl;


      // get child lists or leaf node vectors
      evioDOMNodeList::const_iterator iter;
      for(iter=pList->begin(); iter!=pList->end(); iter++) {

        if((*iter)->isContainer()) {
          evioDOMNodeList *childList = (*iter)->getChildList();
          cout << endl << endl << "Found container node with " << childList->size() 
               << " children using getChildList.  Parent is:" << endl << endl 
               << (*iter)->toString() << endl;
          evioDOMNodeList::const_iterator cIter;
          for(cIter=childList->begin(); cIter!=childList->end(); cIter++) {
            cout << "child tag: " << (*cIter)->tag << endl;
          }
          
        } else {
          const evioDOMNodeP np = *iter;
          const vector<double> *v = np->getVector<double>();
          if(v!=NULL) {
            cout << endl << endl << "Got double node using getVector:" << endl << endl << (*iter)->toString()
                 << endl << "Double data:" << endl;
            for(ui=0; ui<v->size(); ui++) cout << showpoint << (*v)[ui] << endl;
          }
        }
      }
      


      // print all float nodes and contents by iterating over the full list and using dynamic_cast<>()
      for(iter=pList->begin(); iter!=pList->end(); iter++) {
        const evioDOMLeafNode<float> *l= dynamic_cast<const evioDOMLeafNode<float>*>(*iter);
        if(l!=NULL) {
          cout << endl << "Found a float node in the full list:" << endl << l->toString() << endl;
          cout << "vector<float> contents:" << endl;
          for(ui=0; ui<l->data.size(); ui++) cout << (l->data)[ui] << endl;
          cout << endl << endl;
        }
      }



      // another way to print float nodes using float list
      evioDOMNodeListP pFloat = t.getNodeList(typeIs<float>());
      cout << endl << endl << "Another way to dump " << pFloat->size() << " float nodes" << endl << endl;
      for_each(pFloat->begin(),pFloat->end(),toCout());
      cout << endl << endl;



      // remove tag 2 nodes from float list and print
      pFloat->erase(remove_if(pFloat->begin(),pFloat->end(),tagEquals(2)),pFloat->end());
      cout << endl << "Removed tag 2 from float node list" << endl << endl;
      for_each(pFloat->begin(),pFloat->end(),toCout());
      cout << endl << endl;



      // print unsigned int nodes
      evioDOMNodeList::const_iterator ulIter;
      for(ulIter=pUint->begin(); ulIter!=pUint->end(); ulIter++) {
        cout << endl << endl << "Unsigned int node" << endl << (*ulIter)->toString()
             << endl << "Unsigned int node data:" << endl;
        const evioDOMNodeP np = *ulIter;
        const vector<unsigned int> *lv = np->getVector<unsigned int>();
        if(lv!=NULL) {
          for(unsigned int k=0; k<lv->size(); k++) {
            cout << hex << showbase << (*lv)[k] << endl;
          }
        } else {
          cerr << "?no uint vector" << endl;
        }
      }
      

      // count nodes by tag (up to 10)
      cout << endl << endl << "There are " << dec << setw(2) << pList->size() << " total nodes" << endl;
    
      // doesn't work on Solaris
      for(int tg=0; tg<=10; tg++) {
        cout << "There are " << setw(2) 
             << count_if(pList->begin(),pList->end(),tagEquals(tg))
             << " nodes with tag " << setw(2) << tg << endl;
      }
      cout << endl << endl;
    
    
    
      // doesn't work on Solaris
      // count and print nodes satisfying user match criteria
      int count = count_if(pList->begin(),pList->end(),myMatcher);
      cout << "There are " << count << " nodes satisfying user match criteria:" << endl << endl;
   
      evioDOMNodeList::iterator resultIter1=pList->begin();
      for(i=0; i<count; i++) {
        cout << (*((resultIter1=find_if(resultIter1,pList->end(),myMatcher))++))->toString() << endl << endl;
      }
      cout << endl << endl;
    
    
    
      // another way to print nodes satisfying user match criteria
      cout << "Find leaf nodes satisfying user match criteria:" << endl << endl;
      evioDOMNodeList::iterator resultIter2=pLeafList->begin();
      while((resultIter2=find_if(resultIter2,pLeafList->end(),myMatcher))!=pLeafList->end()) {
        cout << (*resultIter2)->toString() << endl << endl;
        resultIter2++;
      }
      cout << endl << endl;
      

      if((nevents >= maxev) && (maxev != 0)) break;
      
    }


    // close file
    chan->close();
  
  } catch (evioException e) {
    cerr << endl << e.toString() << endl << endl;
    std::exit(EXIT_FAILURE);

  } catch (...) {
    cerr << endl << "?Unknown error" << endl << endl;
    std::exit(EXIT_FAILURE);
  }
  

  // done
  cout << "done...everything out of scope" << endl;
  std::exit(EXIT_SUCCESS);
}
