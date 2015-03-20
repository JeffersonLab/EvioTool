//  example program tests random access i/o

//  ejw, 4-sep-2012




#include "evioUtil.hxx"
#include "evioFileChannel.hxx"

using namespace evio;
using namespace std;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  
  int num;

  try {
    evioFileChannel *chan;

    if(argc>1) {
      chan = new evioFileChannel(argv[1],"ra");
    } else {
      chan = new evioFileChannel("fakeEvents.dat","ra");
    }
    chan->open();
    

    // check out random access table
    uint32_t **table;
    uint32_t len;
    chan->getRandomAccessTable(&table,&len);
    cout << "random access table length is: " << len << endl;
    for(int i=0; i<len; i++) cout << i << " " << table[i] << endl;
    cout << endl;    



    // read some random events
    num=10;
    if(chan->readRandom(num)) {
      evioDOMTree event(chan);
      cout << endl << "Read event: " << num << endl;
      //    cout << event.toString() << endl;
    } else {
      cerr << "unable to read event " << num << endl;
    }
    
    num=20;
    if(chan->readRandom(num)) {
      evioDOMTree event(chan);
      cout << endl << "Read event: " << num << endl;
      //    cout << event.toString() << endl;
    } else {
      cerr << "unable to read event " << num << endl;
    }
    
    num=50;
    if(chan->readRandom(num)) {
      evioDOMTree event(chan);
      cout << endl << "Read event: " << num << endl;
      //    cout << event.toString() << endl;
    } else {
      cerr << "unable to read event " << num << endl;
    }
    
    num=3;
    if(chan->readRandom(num)) {
      evioDOMTree event(chan);
      cout << endl << "Read event: " << num << endl;
      //    cout << event.toString() << endl;
    } else {
      cerr << "unable to read event " << num << endl;
    }
    

    num=1;
    if(chan->readRandom(num)) {
      evioDOMTree event(chan);
      cout << endl << "Read event: " << num << endl;
      //    cout << event.toString() << endl;
    } else {
      cerr << "unable to read event " << num << endl;
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
