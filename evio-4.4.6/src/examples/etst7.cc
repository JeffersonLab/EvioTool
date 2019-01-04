//  example of evio tree and bank manipulation

//  ejw, 11-jan-2007


#include <vector>
#include "evioUtil.hxx"
#include "evioFileChannel.hxx"

using namespace evio;
using namespace std;



uint16_t tag;
uint8_t num;
int len;
ContainerType cType;

vector<uint32_t> uVec1,uVec2;
double dBuf[100];
int iBuf[100];
vector<int32_t> lVec;
evioDOMNodeP parent;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  

  // fill vectors
  for(int i=0; i<10; i++) uVec1.push_back(i),uVec2.push_back(2*i),dBuf[i]=i*10.0,lVec.push_back(100*i),iBuf[i]=100*i+i;



  try {

    // create a tree and root node
    evioDOMTree event(tag=1, num=1);


    // create a container node and add to tree
    evioDOMNodeP cn1 = evioDOMNode::createEvioDOMNode(tag=3, num=7);
    event << cn1;


    // create another leaf node
    evioDOMNodeP ln2 = evioDOMNode::createEvioDOMNode(tag=2, num=6, uVec1); 


    // test operator==()
    if(*ln2==2) {
      cout << endl << "test *ln2==2 succeeded" << endl << endl;
    } else {
      cerr << endl << "test *ln2==2 failed" << endl << endl;
    }
    if(*ln2==tagNum(2,6)) {
      cout << endl << "test *ln2==(2,6) succeeded" << endl << endl;
    } else {
      cerr << endl << "test *ln2==(2,6) failed" << endl << endl;
    }
    
    
    // hang leaf nodes off cn1, note dereferencing of pointer cn1
    evioDOMNodeP ln3 = evioDOMNode::createEvioDOMNode(tag=2, num=7, dBuf, 10);
    evioDOMNodeP ln4 = evioDOMNode::createEvioDOMNode(tag=2, num=8, lVec);
    *cn1 << ln2 << ln3 << ln4;
    
    
    // add some more data to ln2
    *ln2 << uVec2 << uVec1;
    
    
    // create another sub-tree
    evioDOMNodeP cn3 = evioDOMNode::createEvioDOMNode(tag=30, num=35, cType=BANK);
    ln2 = evioDOMNode::createEvioDOMNode(tag=31, num=36, uVec1);
    ln3 = evioDOMNode::createEvioDOMNode(tag=32, num=37, lVec);
    ln4 = evioDOMNode::createEvioDOMNode(tag=10, num=20, iBuf, 10);
    evioDOMNodeP ln5 = evioDOMNode::createEvioDOMNode(tag=11, num=21, dBuf, 5);
    evioDOMNodeP ln6 = evioDOMNode::createEvioDOMNode(tag=12, num=22, lVec);
    *cn3 << ln2 << ln3 << ln4 << ln5 << ln6;
    

    // hang cn3 off cn1
    *cn1 << cn3;


    // test move
    ln2->move(cn3);


    // test cut
    ln2->cutAndDelete();


    // test cutAndDelete
    cn3->cutAndDelete();


    // print tree
    cout << event.toString() << endl;

    
  } catch (evioException e) {
    cerr << e.toString() << endl;
  }

}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
