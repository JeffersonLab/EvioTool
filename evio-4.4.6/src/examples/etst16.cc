//  example program tests new buffer interface

//  ejw, 14-Feb-2013




#include "evioBankIndex.hxx"
#include "evioUtil.hxx"
#include "evioFileChannel.hxx"


using namespace evio;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  

  // int bufferCount = 0;
  evioFileChannel *chan;


  try {

    if(argc>1) {
      chan = new evioFileChannel(argv[1],"r");
    } else {
      chan = new evioFileChannel("fakeEvents.dat","r");
    }
    chan->open();


    // loop over all buffers in file
    while(chan->readNoCopy()) {

      // get buffer
      const uint32_t *buffer = chan->getNoCopyBuffer();

      // create bank indices for different levels
      evioBankIndex b0(buffer,0);  // all banks
      evioBankIndex b1(buffer,1);  // main bank and its immediate children
      evioBankIndex b2(buffer,2);  // main bank down to its grandchildren
      evioBankIndex b3(buffer,3);  // etc.
      evioBankIndex b4(buffer,4);


      // number of banks at each level
      cout << "b0 has " << b0.tagNumMap.size() << " banks" << endl;
      cout << "b1 has " << b1.tagNumMap.size() << " banks" << endl;
      cout << "b2 has " << b2.tagNumMap.size() << " banks" << endl;
      cout << "b3 has " << b3.tagNumMap.size() << " banks" << endl;
      cout << "b4 has " << b4.tagNumMap.size() << " banks" << endl;
      cout << endl;


      // check if some tag,num's exist
      bankIndex b;
      b = b0.getBankIndex(tagNum(12,22));
      cout << "12,22:             " << b0.tagNumExists(tagNum(12,22)) << endl;
      cout << "12,22 count:       " << b0.tagNumCount(tagNum(12,22)) << endl;
      cout << "12,22 container:   " << b.containerType << endl;
      cout << "12,22 content:     " << b.contentType << endl;
      cout << "12,22 bank length: " << b.bankLength << endl;
      cout << "12,22 data length: " << b.dataLength << endl;
      if(b.dataLength>0) {
        cout << " full bank: ";
        for(unsigned int i=0; i<b.bankLength; i++) cout << ((uint32_t*)b.bankPointer)[i] << "  ";
        cout << endl << endl;
        cout << " data only: ";
        for(unsigned int i=0; i<b.dataLength; i++) cout << ((uint32_t*)b.data)[i] << "  ";
        cout << endl;
      }
      cout << endl;


      b = b0.getBankIndex(tagNum(30,35));
      cout << "30,35:             " << b0.tagNumExists(tagNum(30,35)) << endl;
      cout << "30,35 count:       " << b0.tagNumCount(tagNum(30,35)) << endl;
      cout << "30,35 container:   " << b.containerType << endl;
      cout << "30,35 content:     " << b.contentType << endl;
      cout << "30,35 bank length: " << b.bankLength << endl;
      cout << "30,35 data length: " << b.dataLength << endl;
      if(b.dataLength>0) {
        cout << " full bank: ";
        for(unsigned int i=0; i<b.bankLength; i++) cout << ((uint32_t*)b.bankPointer)[i] << "  ";
        cout << endl << endl;
        cout << " data only: ";
        for(unsigned int i=0; i<b.dataLength; i++) cout << ((uint32_t*)b.data)[i] << "  ";
        cout << endl;
      }
      cout << endl;


      b = b0.getBankIndex(tagNum(11,21));
      cout << "11,21:             " << b0.tagNumExists(tagNum(11,21)) << endl;
      cout << "11,21 count:       " << b0.tagNumCount(tagNum(11,21)) << endl;
      cout << "11,21 container:   " << b.containerType << endl;
      cout << "11,21 content:     " << b.contentType << endl;
      cout << "11,21 bank length: " << b.bankLength << endl;
      cout << "11,21 data length: " << b.dataLength << endl;
      if(b.dataLength>0) {
        cout << " full bank: ";
        for(unsigned int i=0; i<b.bankLength; i++) cout << ((uint32_t*)b.bankPointer)[i] << "  ";
        cout << endl << endl;
        cout << " data only: ";
        for(unsigned int i=0; i<b.dataLength; i++) cout << ((double*)b.data)[i] << "  ";
        cout << endl;
      }
      cout << endl;


      cout << "12,99: "       << b0.tagNumExists(tagNum(12,99)) << endl;
      cout << "12,99 count: " << b0.tagNumCount(tagNum(12,99)) << endl;
      cout << endl;



      // test appending structure to buffer

      // create test tree, serialize to buffer
      evioDOMTree tTest(1,0);
      tTest << evioDOMNode::createEvioDOMNode(1,2);
      tTest << evioDOMNode::createEvioDOMNode(3,4);
      uint32_t *testBuf = new uint32_t[10000];
      tTest.toEVIOBuffer(testBuf,10000);

      // append serialized tree to original buffer
      uint32_t *newBuf = evioUtilities::appendToBuffer(buffer,BANK,testBuf,BANK);
      
      // deserialize new buffer and print
      evioDOMTree tNew(newBuf);
      cout << tNew.toString() << endl;
      
      break;
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
