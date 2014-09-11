// parses buffer, uses toEVIOBuffer to create another buffer, then compares to original

// ejw, 8-dec-2006



#define MAXBUFLEN  4096

#include <iostream>
#include <stdio.h>
#include <evioUtil.hxx>
#include "evioFileChannel.hxx"

using namespace evio;
using namespace std;


//--------------------------------------------------------------
//--------------------------------------------------------------


class myHandler : public evioStreamParserHandler {


  void *containerNodeHandler(int bankLength, int containerType, int contentType, unsigned short tag, unsigned char num, 
                             int depth, const uint32_t *bankPointer, int payloadLength, const uint32_t *payload, void *userArg) {

    printf("node   depth %2d   type,tag,num,length:  %6d %6d %6d %6d\n",
           depth,contentType,tag,num,bankLength);
    return(NULL);
  }
  
  
//--------------------------------------------------------------


  void *leafNodeHandler(int bankLength, int containerType, int contentType, unsigned short tag, unsigned char num, 
                        int depth, const uint32_t *bankPointer, int dataLength, const void *data, void *userArg) {
    
    int *i;
    short *s;
    char *c;
    float *f;
    double *d;
    int64_t *ll;
    
    printf("leaf   depth %2d   type,tag,num,length:  %6d %6d %6d %6d     data:   ",
           depth,contentType,tag,num,bankLength);
    
    switch (contentType) {

    case 0x0:
    case 0x1:
    case 0xb:
      i = (int *)data;
      printf("%d %d\n",i[0],i[1]);
      break;


    case 0x2:
      f=(float*)data;
      printf("%f %f\n",f[0],f[1]);
      break;

    case 0x3:
      c=(char*)data;
      printf("%s\n",c);
      break;

    case 0x6:
    case 0x7:
      c=(char*)data;
      printf("%d %d\n",c[0],c[1]);
      break;

    case 0x4:
    case 0x5:
      s=(short*)data;
      printf("%hd %hd\n",s[0],s[1]);
      break;

    case 0x8:
      d=(double*)data;
      printf("%f %f\n",d[0],d[1]);
      break;

    case 0x9:
    case 0xa:
      ll=(int64_t*)data;
      printf("%lld %lld\n",ll[0],ll[1]);
      break;

    }  

    return((void*)NULL);
  } 

};




//--------------------------------------------------------------
//--------------------------------------------------------------


void myNodePrinter(const evioDOMNodeP pNode) {
  cout << hex << left << "content type:  0x" << setw(6) << pNode->getContentType() << dec << "   tag:  " << setw(6) << pNode->tag 
       << "   num:  " << setw(6) << pNode->num << endl;
  //  cout << pNode->toString() << endl;
}


//--------------------------------------------------------------
//--------------------------------------------------------------


int main(int argc, char **argv) {

  int handle, nevents, status;
  uint32_t buf[MAXBUFLEN];
  uint32_t buf2[MAXBUFLEN];
  int maxev =0;
  vector<int32_t> ivec;
  char *filename;


  for(int i=0; i<10; i++) ivec.push_back(i);

 
  if(argc>1) {
    filename=argv[1];
  } else {
    filename=(char*)"fakeEvents.dat";
  }


  if ((status=evOpen(filename,(char*)"r",&handle))<0) {
    printf("Unable to open file %s status = %d\n",filename,status);
    exit(-1);
  } else {
    printf("Opened %s for reading\n",filename);
  }

  if(argc==3) {
    maxev = atoi(argv[2]);
    printf("Read at most %d events\n",maxev);
  } else {
    maxev = 0;
  }

  nevents=0;
  while ((status=evRead(handle,buf,MAXBUFLEN))==0) {
    nevents++;
    printf("\n  *** event %d len %d type %d ***\n",nevents,buf[0],(buf[1]&0xffff0000)>>16);


    try {

      // stream parse the buffer
      cout << endl << endl << "Stream parsing event:" << endl << endl;
      evioStreamParser p;
      myHandler h;
      p.parse(buf,h,(void*)NULL);



      // DOM parse the event and create event tree
      cout << endl << endl << "DOM parsing event:" << endl << endl;
      evioDOMTree t(buf,"fred");


      // dump tree
      cout << t.toString() << endl;
      

      // fill new evio buffer
      t.toEVIOBuffer(buf2,MAXBUFLEN);


      // compare buffers
      bool ok = true;
      for(unsigned int i=0; i<buf[0]+1; i++) {
        if(buf[i]!=buf2[i]) {
          ok=false;
          cout << dec << i << ":   " << hex << "0x" << buf[i] << "    0x" << buf2[i] << endl;
        }
      }
      if(ok)cout << " *** evio buffers agree ***" << endl << endl << endl;


    } catch (evioException e) {
      cerr << e.toString() << endl;
      exit(EXIT_FAILURE);
    }


    if((nevents >= maxev) && (maxev != 0)) break;
  }

  printf("\n  *** last read status 0x%x ***\n\n",status);
  evClose(handle);
  
  exit(0);
}
