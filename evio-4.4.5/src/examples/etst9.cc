//  example program reads and processes EVIO events 

//  ejw, 14-nov-2006




#include "evioUtil.hxx"
#include "evioFileChannel.hxx"
#include <vector>

// doesn't work on Solaris
//  #include <boost/function.hpp>




using namespace evio;
using namespace std;



//-------------------------------------------------------------------------------


template <typename T> void pfun(T t) { cout << t << "  "; }



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
    

    // loop over all events in file
    while(true) {
      try {
        nread++;
        if(chan->readNoCopy()) {
          cout << endl << " --- read event " << nread << " ---" << endl;
          evioDOMTree tree(chan);

          vector<float> *v1 = tree.getVectorUnique<float>();
          if(v1!=NULL) {
            cout << "found vector<float>" << endl;
            for_each(v1->begin(),v1->end(),pfun<float>);
            cout << endl;
          } else {
            cout << "?did not find vector<float>" << endl;
          }

          vector<uint32_t> *v2 = tree.getVectorUnique<uint32_t>(tagNumEquals(2,9));
          if(v2!=NULL) {
            cout << "found vector<uint32_t> tag 2, num 9" << endl;
          } else {
            cout << "?did not find vector<uint32_t> tag 2, num 9" << endl;
          }

          vector<int32_t> *v3 = tree.getVectorUnique<int32_t>(tagNumEquals(2,9));
          if(v3!=NULL) {
            cout << "found vector<int32_t> tag 2, num 9" << endl;
          } else {
            cout << "?did not find vector<int32_t> tag 2, num 9" << endl;
          }

//           vector<double> *v4 = tree.getVectorUnique<double>();
//           if(v4!=NULL) {
//             cout << "found vector<double>" << endl;
//           } else {
//             cout << "?did not find vector<double>" << endl;
//           }

        } else {
          break;
        }
      } catch (evioException e) {
        cerr << endl << "?processing exception on event " << nread << endl << e.toString() << endl;
        continue;
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
