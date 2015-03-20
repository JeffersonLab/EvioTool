//  example program reads and writes to buffer

//  ejw, 12-apr-2012




#include "evioUtil.hxx"
#include "evioBufferChannel.hxx"


using namespace evio;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  
  float x[10] = {1,2,3,4,5,6,7,8,9,10};
  uint32_t ix[10] = {1,2,3,4,5,6,7,8,9,10};
  string dictXML = "<dict>\n"
    "<dictEntry name=\"fred\"   tag=\"1\" num=\"0\"/> \n"
    "<dictEntry name=\"wilma\"  tag=\"4\" num=\"11\"/>\n"
    "<dictEntry name=\"barney\" tag=\"5\" num=\"12\"/>\n"
    "<dictEntry name=\"betty\"  tag=\"6\" num=\"13\"/>\n"
    "<bank name=\"b1\"     tag=\"7\" num=\"14\">\n"
    "<bank name=\"b2\"    tag=\"8\" num=\"15\">\n"
    "<leaf name=\"l21\"   tag=\"9\" num=\"16\"/>\n"
    "<leaf name=\"l22\"        tag=\"10\" num=\"17\"/>\n"
    "</bank>\n"
    "<leaf name=\"l13\"        tag=\"11\" num=\"18\"/>\n"
    "</bank>\n"
    "</dict>\n";


  // evio buffer channel
  evioBufferChannel * chan;


  try {

    // create dictionary from XML string
    // to create a dictionary from a file use:
    //      evioDictionary dict(ifstream("myfile.xml"));
    //  (can you guess why evioDictionary dict("myfile.xml") won't work?)
    evioDictionary dict(dictXML);


    // create stream buffer
    int streamBufLen=100000;
    uint32_t streamBuf[streamBufLen];


    // create output buffer channel and specify dictionary
    chan = new evioBufferChannel(streamBuf,streamBufLen,&dict,"w");
    //    chan = new evioBufferChannel(streamBuf,streamBufLen,"w");


    // open the channel, dictionary gets written to stream buffer header now
    chan->open();


    // create event tree
    evioDOMTree event1(1, 0);
    event1.addBank(4, 11, ix, 4);
    event1.addBank(5, 12, x, 5);
    event1.addBank(6, 13, ix, 6);
    event1.addBank(7, 14, x, 2);
    event1.addBank(8, 15, ix, 3);
    event1.addBank(9, 16, x, 4);
    event1.addBank(10, 17, ix, 8);
    event1.addBank(11, 18, x, 7);
    cout << endl << "created event tree: " << endl << event1.toString(dict) << endl << endl;


    // write event to stream buffer
    // must get number of words written BEFORE closing channel!
    chan->write(event1);
    cout << endl << "wrote " << chan->getEVIOBufferLength() << " total words into stream buffer" << endl << endl;


    // write the event a few times more
    for(int i=0; i<5; i++) {
      chan->write(event1);
      cout << endl << "wrote " << chan->getEVIOBufferLength() << " total words into stream buffer" << endl << endl;
    }



    // close buffer channel
    // int streamBufSize = chan->getEVIOBufferLength();
    chan->close();
    delete(chan);


    // dump streamBuf
    // cout << endl << endl;
    // for(int i=0; i<streamBufSize; i++) {
    //   cout << hex << "0x" << streamBuf[i] << "  ";
    // }
    // cout << dec << endl;



    // test some new api functions
    //   evioDOMNodeListP l1 = event1.root->getChildren();
    //   cout << "l1 size is " << l1->size() << endl;
    //   evioDOMNodeListP l2 = event1.root->getChildren(tagNumEquals(4,11));
    //   cout << "l2 size is " << l2->size() << endl;
    //   cout << endl << endl;
    


    // create input buffer channel from original stream buffer
    // dictionary was written to buffer it gets read in automatically
    // N.B. you can supply a different dictionary if desired
    chan = new evioBufferChannel(streamBuf,streamBufLen,"r");


    // open channel, dictionary gets read in
    chan->open();


    // get the dictionary
    const evioDictionary *d = chan->getDictionary();
    if(d!=NULL)cout << endl << "found dictionary in stream buffer : " << endl << d->getDictionaryXML() << endl << endl;


    // read and dump all events in stream buffer
    while(chan->readNoCopy()) {
      evioDOMTree event2(chan);
      cout << endl << "found event: " << endl << event2.toString() << endl << endl;
    }


    // done
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
