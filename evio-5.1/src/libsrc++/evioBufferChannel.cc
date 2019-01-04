/*
 *  evioBufferChannel.cc
 *
 *   Author:  Elliott Wolin, JLab, 12-Apr-2012
*/


#include <stdio.h>
#include "evioBufferChannel.hxx"


using namespace std;
using namespace evio;



//-----------------------------------------------------------------------
//------------------------- evioBufferChannel -----------------------------
//-----------------------------------------------------------------------


/**
 * Constructor opens buffer for reading or writing.
 * @param streamBuf Stream buffer specified by the user
 * @param bufLen size of the stream buffer
 * @param m I/O mode, "r", "ra", "w", or "a"
 * @param size Internal event buffer size
 */
evioBufferChannel::evioBufferChannel(uint32_t *streamBuf, int bufLen, const string &m, int size) throw(evioException) 
  : evioChannel(), streamBuf(streamBuf), streamBufSize(bufLen), mode(m), handle(0), bufSize(size), noCopyBuf(NULL), randomBuf(NULL),
    bufferXMLDictionary(""), createdBufferDictionary(false) {
  if(streamBuf==NULL)throw(evioException(0,"?evioBufferChannel constructor...NULL buffer",__FILE__,__FUNCTION__,__LINE__));

  // lowercase mode
  std::transform(mode.begin(), mode.end(), mode.begin(), (int(*)(int)) tolower);  // magic

  // allocate internal event buffer
  buf = new uint32_t[bufSize];
  if(buf==NULL)throw(evioException(0,"?evioBufferChannel constructor...unable to allocate buffer",__FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Constructor opens buffer for reading or writing.
 * @param streamBuf Stream buffer specified by the user
 * @param bufLen size of the stream buffer
 * @param dict Dictionary
 * @param m I/O mode, "r", "ra", "w", or "a"
 * @param size Internal event buffer size
 */
evioBufferChannel::evioBufferChannel(uint32_t *streamBuf, int bufLen, evioDictionary *dict, const string &m, int size) throw(evioException) 
  : evioChannel(dict), streamBuf(streamBuf), streamBufSize(bufLen), mode(m), handle(0), bufSize(size), noCopyBuf(NULL), randomBuf(NULL), 
    bufferXMLDictionary(""), createdBufferDictionary(false) {
  if(streamBuf==NULL)throw(evioException(0,"?evioBufferChannel constructor...NULL buffer",__FILE__,__FUNCTION__,__LINE__));

  // lowercase mode
  std::transform(mode.begin(), mode.end(), mode.begin(), (int(*)(int)) tolower);  // magic

  // allocate internal event buffer
  buf = new uint32_t[bufSize];
  if(buf==NULL)throw(evioException(0,"?evioBufferChannel constructor...unable to allocate buffer",__FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Destructor closes buffer, deletes internal buffer and dictionary.
 */
evioBufferChannel::~evioBufferChannel(void) {
  if(handle!=0)close();
  if(buf!=NULL)delete[](buf),buf=NULL;
  if(createdBufferDictionary)delete(dictionary), dictionary=NULL;
}


//-----------------------------------------------------------------------


/**
 * Opens channel for reading or writing.
 * For read, user-supplied dictionary overrides one found in buffer.
 */
void evioBufferChannel::open(void) throw(evioException) {

  int stat;
  if(buf==NULL)throw(evioException(0,"evioBufferChannel::open...null buffer",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evOpenBuffer((char*)streamBuf,(uint32_t)streamBufSize,const_cast<char*>(mode.c_str()),&handle))!=S_SUCCESS)
    throw(evioException(stat,"?evioBufferChannel::open...unable to open buffer: " + string(evPerror(stat)),
                        __FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"?evioBufferChannel::open...zero handle",__FILE__,__FUNCTION__,__LINE__));


  // on read check if buffer has dictionary, warn if conflict with user dictionary
  // store buffer XML just in case
  // set dictionary on write
  if((mode=="r")||(mode=="ra")) {
    char *d;
    uint32_t len;
    stat=evGetDictionary(handle,&d,&len);
    if((stat==S_SUCCESS)&&(d!=NULL)&&(len>0))bufferXMLDictionary = string(d);

    if(dictionary==NULL) {
      if(stat!=S_SUCCESS)throw(evioException(stat,"?evioBufferChannel::open...bad dictionary in buffer: " + string(evPerror(stat)),
                                             __FILE__,__FUNCTION__,__LINE__));
      if((d!=NULL)&&(len>0)) {
        dictionary = new evioDictionary(d);
        createdBufferDictionary=true;
      }
      
    } else {
      cout << "evioBufferChannel::open...user-supplied dictionary overrides dictionary in buffer" << endl;
    }

  } else if((dictionary!=NULL) && (mode=="w")) {
    evWriteDictionary(handle,const_cast<char*>(dictionary->getDictionaryXML().c_str()));
  }

}


//-----------------------------------------------------------------------


/**
 * Reads from buffer
 * @return true if successful, false on EOF or other evRead error condition
 */
bool evioBufferChannel::read(void) throw(evioException) {
  noCopyBuf=NULL;
  if(buf==NULL)throw(evioException(0,"evioBufferChannel::read...null buffer",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioBufferChannel::read...0 handle",__FILE__,__FUNCTION__,__LINE__));
  return(evRead(handle,&buf[0],(uint32_t)bufSize)==0);
}


//-----------------------------------------------------------------------


/**
 * Reads from buffer into user-supplied event buffer.
 * @param myBuf User-supplied buffer.
 * @parem length Length of buffer in 4-byte words.
 * @return true if successful, false on EOF or other evRead error condition
 */
bool evioBufferChannel::read(uint32_t *myBuf, int length) throw(evioException) {
  noCopyBuf=NULL;
  if(myBuf==NULL)throw(evioException(0,"evioBufferChannel::read...null user buffer",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioBufferChannel::read...0 handle",__FILE__,__FUNCTION__,__LINE__));
  return(evRead(handle,&myBuf[0],(uint32_t)length)==0);
}


//-----------------------------------------------------------------------


/**
 * Reads from file and allocates buffer as needed.
 * @param buffer Pointer to pointer to allocated buffer.
 * @param len Length of allocated buffer in 4-byte words.
 * @return true if successful, false on EOF, throws exception for other error.
 *
 * Note:  user MUST free the allocated buffer!
 */
bool evioBufferChannel::readAlloc(uint32_t **buffer, uint32_t *bufLen) throw(evioException) {
  noCopyBuf=NULL;
  if(handle==0)throw(evioException(0,"evioBufferChannel::readAlloc...0 handle",__FILE__,__FUNCTION__,__LINE__));

  int stat=evReadAlloc(handle,buffer,bufLen);
  if(stat==EOF) {
    *buffer=NULL;
    *bufLen=0;
    return(false);
  }

  if(stat!=S_SUCCESS) throw(evioException(stat,"evioBufferChannel::readAlloc...read error: " + string(evPerror(stat)),
                                          __FILE__,__FUNCTION__,__LINE__));
  return(true);
}


//-----------------------------------------------------------------------


/**
 * Get const pointer to next event in stream buffer.
 * @return true if successful, false on EOF, throws exception for other error.
 */
bool evioBufferChannel::readNoCopy(void) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioBufferChannel::readNoCopy...0 handle",__FILE__,__FUNCTION__,__LINE__));

  uint32_t bufLen;
  int stat=evReadNoCopy(handle,&noCopyBuf,&bufLen);
  if(stat==EOF)return(false);
  if(stat!=S_SUCCESS) throw(evioException(stat,"evioBufferChannel::readNoCopy...read error: " + string(evPerror(stat)),
                                          __FILE__,__FUNCTION__,__LINE__));
  return(true);
}


//-----------------------------------------------------------------------


/**
 * Reads buffer from file given buffer number
 * @param bufferNumber Buffer to return
 * @return true if successful, false on EOF, throws exception for other error.
 */
bool evioBufferChannel::readRandom(uint32_t bufferNumber) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioBufferChannel::readRandom...0 handle",__FILE__,__FUNCTION__,__LINE__));

  uint32_t bufLen;
  int stat=evReadRandom(handle,&randomBuf,&bufLen,bufferNumber);
  if(stat==EOF) return(false);
  if(stat!=S_SUCCESS) throw(evioException(stat,"evioBufferChannel::readRandom...read error: " + string(evPerror(stat)),
                                          __FILE__,__FUNCTION__,__LINE__));
  return(true);
}


//-----------------------------------------------------------------------


/**
 * Writes to buffer from internal buffer.
 */
void evioBufferChannel::write(void) throw(evioException) {
  int stat;
  if(buf==NULL)throw(evioException(0,"evioBufferChannel::write...null buffer",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioBufferChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evWrite(handle,buf))!=0) throw(evioException(stat,"?evioBufferChannel::write...unable to write: " + string(evPerror(stat)),
                                                      __FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Writes to buffer from user-supplied buffer.
 * @param myBuf Buffer containing event
 */
void evioBufferChannel::write(const uint32_t *myBuf) throw(evioException) {
  int stat;
  if(myBuf==NULL)throw(evioException(0,"evioBufferChannel::write...null myBuf",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioBufferChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evWrite(handle,myBuf))!=0) throw(evioException(stat,"?evioBufferChannel::write...unable to write from myBuf: " + string(evPerror(stat)),
                                                        __FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Writes to buffer from internal buffer of another evioChannel object.
 * @param channel Channel object
 */
void evioBufferChannel::write(const evioChannel &channel) throw(evioException) {
  int stat;
  if(handle==0)throw(evioException(0,"evioBufferChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evWrite(handle,channel.getBuffer()))!=0)
    throw(evioException(stat,"?evioBufferChannel::write...unable to write from channel: " + string(evPerror(stat)),
                        __FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Writes from internal buffer of another evioChannel object.
 * @param channel Pointer to channel object
 */
void evioBufferChannel::write(const evioChannel *channel) throw(evioException) {
  if(channel==NULL)throw(evioException(0,"evioBufferChannel::write...null channel",__FILE__,__FUNCTION__,__LINE__));
  evioBufferChannel::write(*channel);
}


//-----------------------------------------------------------------------


/**
 * Writes from contents of evioChannelBufferizable object.
 * @param o evioChannelBufferizable object that can serialze to buffer
 */
void evioBufferChannel::write(const evioChannelBufferizable &o) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioBufferChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  o.toEVIOBuffer(buf,bufSize);
  evioBufferChannel::write();
}


//-----------------------------------------------------------------------


/**
 * Writes from contents of evioChannelBufferizable object.
 * @param o Pointer to evioChannelBufferizable object that can serialize to buffer
 */
void evioBufferChannel::write(const evioChannelBufferizable *o) throw(evioException) {
  if(o==NULL)throw(evioException(0,"evioBufferChannel::write...null evioChannel Bufferizable pointer",__FILE__,__FUNCTION__,__LINE__));
  evioBufferChannel::write(*o);
}


//-----------------------------------------------------------------------


/**
 * For getting and setting evIoctl parameters.
 * @param request String containing evIoctl parameters
 * @param argp Additional evIoctl parameter
 */
int evioBufferChannel::ioctl(const string &request, void *argp) throw(evioException) {
  int stat;
  if(handle==0)throw(evioException(0,"evioBufferChannel::ioctl...0 handle",__FILE__,__FUNCTION__,__LINE__));
  stat=evIoctl(handle,const_cast<char*>(request.c_str()),argp)!=0;
  if(stat!=S_SUCCESS)throw(evioException(stat,"?evioBufferChannel::ioCtl...error return: " + string(evPerror(stat)),
                                         __FILE__,__FUNCTION__,__LINE__));
  return(stat);
}


//-----------------------------------------------------------------------


/**
 * Closes channel.
 */
void evioBufferChannel::close(void) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioBufferChannel::close...0 handle",__FILE__,__FUNCTION__,__LINE__));
  evClose(handle);
  handle=0;
}


//-----------------------------------------------------------------------


/**
 * Returns channel I/O mode.
 * @return String containing I/O mode
 */
string evioBufferChannel::getMode(void) const {
  return(mode);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to internal channel buffer.
 * @return Pointer to internal buffer
 */
const uint32_t *evioBufferChannel::getBuffer(void) const throw(evioException) {
  if(buf==NULL)throw(evioException(0,"evioBufferChannel::getbuffer...null buffer",__FILE__,__FUNCTION__,__LINE__));
  return(buf);
}


//-----------------------------------------------------------------------


/**
 * Returns length of event record in stream buffer in 4-byte words.
 * @return Length of event record in stream buffer 4-byte words
 */
uint32_t evioBufferChannel::getEVIOBufferLength(void) const throw(evioException) {
  if(handle==0)throw(evioException(0,"evioBufferChannel::getEVIOBufferLength...0 handle",__FILE__,__FUNCTION__,__LINE__));
  uint32_t l;
  int stat = evGetBufferLength(handle,&l);   // length is in bytes
  if(stat!=S_SUCCESS)throw(evioException(stat,"evioBufferChannel::getEVIOBufferLength...error return: " + string(evPerror(stat)),
                                         __FILE__,__FUNCTION__,__LINE__));
  return((l+3)/4);  // convert to 4-byte words
}


//-----------------------------------------------------------------------


/**
 * Returns internal channel buffer size.
 * @return Internal buffer size in 4-byte words
 */
int evioBufferChannel::getBufSize(void) const {
  return(bufSize);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to no copy buffer
 * @return Pointer to no copy buffer
 */
const uint32_t *evioBufferChannel::getNoCopyBuffer(void) const throw(evioException) {
  return(noCopyBuf);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to random buffer
 * @return Pointer to random buffer
 */
const uint32_t *evioBufferChannel::getRandomBuffer(void) const throw(evioException) {
  return(randomBuf);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to stream buffer.
 * @return Pointer to stream buffer
 */
const uint32_t *evioBufferChannel::getStreamBuffer(void) const throw(evioException) {
  return(streamBuf);
}


//-----------------------------------------------------------------------



/**
 * Returns internal channel buffer size.
 * @return Internal buffer size in 4-byte words
 */
int evioBufferChannel::getStreamBufSize(void) const {
  return(streamBufSize);
}


//-----------------------------------------------------------------------


/**
 * Returns XML dictionary read in from buffer
 * @return XML dictionary read in from buffer
 */
string evioBufferChannel::getBufferXMLDictionary(void) const {
  return(bufferXMLDictionary);
}


//-----------------------------------------------------------------------


/**
 * Returns random access table
 * @param table Pointer to table
 * @param len Length of table
 */
void evioBufferChannel::getRandomAccessTable(uint32_t *** const table, uint32_t *len) const throw(evioException) {
  if(handle==0)throw(evioException(0,"evioBufferChannel::getRandomAccessTable...0 handle",__FILE__,__FUNCTION__,__LINE__));
  evGetRandomAccessTable(handle,table,len);
}


//-----------------------------------------------------------------------


