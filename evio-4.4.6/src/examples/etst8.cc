
//  ejw, 18-aug-2009




#include <vector>
#include "evioUtil.hxx"
#include "evioFileChannel.hxx"

using namespace evio;
using namespace std;


// misc
uint8_t num;
uint16_t tag;
int len;
vector<uint32_t> ulvec;
vector<float> fvec;

int32_t ibuf[100];
int64_t lbuf[100];
double dbuf[100];


int main(int argc, char **argv) {
  
  // fill fake data buffers
  for(int i=0; i<10; i++)ulvec.push_back(i),ibuf[i]=-i,lbuf[i]=2*i,dbuf[i]=10.*i,fvec.push_back(i/10.);


  // generate fake events
  for(int i=0; i<1; i++) {
    
    // create an event tree, root node has (tag=1,num=0)
    evioDOMTree event(tag=1, num=0);
    
    
    // add banks to event in a single level below root node
    event.addBank(tag=2, num=9,  ulvec);
//     event.addBank(tag=3, num=10, ibuf,  len=8);
//     event.addBank(tag=4, num=11, dbuf,  len=6);
//     event.addBank(tag=5, num=12, fvec);
//     event.addBank(tag=6, num=13, dbuf,  len=10);
//     event.addBank(tag=7, num=14, lbuf,  len=8);
    

    event << evioDOMNode::createEvioDOMNode(1,2,3,"i,2f,2i",4,5,ulvec);
    event << evioDOMNode::createEvioDOMNode(4,5,6,"i,2(i,f)",10,11,ulvec);
    event << evioDOMNode::createUnknownEvioDOMNode(10,11,ulvec);


    cout << event.toString() << endl;

  }
  
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
