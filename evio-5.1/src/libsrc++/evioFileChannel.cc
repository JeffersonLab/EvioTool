/*
 *  evioFileChannel.cc
 *
 *   Author:  Elliott Wolin, JLab, 18-feb-2010
*/


#include <stdio.h>
#include "evioFileChannel.hxx"


using namespace std;
using namespace evio;



//-----------------------------------------------------------------------
//------------------------- evioFileChannel -----------------------------
//-----------------------------------------------------------------------


/**
 * Constructor opens channel for reading or writing.
 * @param f File name
 * @param m I/O mode, "r", "ra", "w", "a", or "s"
 * @param size Internal buffer size
 */
evioFileChannel::evioFileChannel(const string &f, const string &m, int size) throw(evioException) 
  : evioChannel(), firstEvent(NULL), filename(f), mode(m), handle(0), bufSize(size), noCopyBuf(NULL), randomBuf(NULL),
    fileXMLDictionary(""), createdFileDictionary(false) {

  // lowercase mode
  std::transform(mode.begin(), mode.end(), mode.begin(), (int(*)(int)) tolower);  // magic
  
  // allocate internal event buffer
  buf = new uint32_t[bufSize];
  if(buf==NULL)throw(evioException(0,"?evioFileChannel constructor...unable to allocate buffer",__FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Constructor opens channel for reading or writing, dictionary specified.
 * @param f File name
 * @param dict Dictionary
 * @param m I/O mode, "r", "ra", "w", "a", or "s"
 * @param size Internal buffer size
 */
evioFileChannel::evioFileChannel(const string &f, evioDictionary* dict, const string &m, int size) throw(evioException) 
  : evioChannel(dict), firstEvent(NULL), filename(f), mode(m), handle(0), bufSize(size), noCopyBuf(NULL), randomBuf(NULL),
    fileXMLDictionary(""), createdFileDictionary(false) {

  // lowercase mode
  std::transform(mode.begin(), mode.end(), mode.begin(), (int(*)(int)) tolower);  // magic

  // allocate internal event buffer
  buf = new uint32_t[bufSize];
  if(buf==NULL)throw(evioException(0,"?evioFileChannel constructor...unable to allocate buffer",__FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Constructor opens channel for reading or writing to file with dictionary specified,
 * If writing, the first event (an event which gets written once in each split file)
 * may also be specified.
 * @param f File name
 * @param dict Dictionary
 * @param firstEvent buffer containing first event
 * @param m I/O mode, "r", "ra", "w", "a", or "s"
 * @param size Internal buffer size
 */
evioFileChannel::evioFileChannel(const string &f, evioDictionary* dict, const uint32_t *firstEvent,
                                 const string &m, int size) throw(evioException)
        : evioChannel(dict), firstEvent(firstEvent), filename(f), mode(m), handle(0), bufSize(size),
          noCopyBuf(NULL), randomBuf(NULL), fileXMLDictionary(""), createdFileDictionary(false) {

  // lowercase mode
  std::transform(mode.begin(), mode.end(), mode.begin(), (int(*)(int)) tolower);  // magic

  // allocate internal event buffer
  buf = new uint32_t[bufSize];
  if(buf==NULL)throw(evioException(0,"?evioFileChannel constructor...unable to allocate buffer",__FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Destructor closes file, deletes internal buffer, deletes dictionary one created from file
 */
evioFileChannel::~evioFileChannel(void) {
  if(handle!=0)close();
  if(buf!=NULL)delete[](buf),buf=NULL;
  if(createdFileDictionary)delete(dictionary),dictionary=NULL;
}


//-----------------------------------------------------------------------


/**
 * Opens channel for reading or writing.
 * For read, user-supplied dictionary overrides one found in file.
 */
void evioFileChannel::open(void) throw(evioException) {

  int stat;

  if(buf==NULL)throw(evioException(0,"evioFileChannel::open...null buffer",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evOpen(const_cast<char*>(filename.c_str()),const_cast<char*>(mode.c_str()),&handle))!=S_SUCCESS)
    throw(evioException(stat,"?evioFileChannel::open...unable to open file: " + string(evPerror(stat)),
                        __FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"?evioFileChannel::open...zero handle",__FILE__,__FUNCTION__,__LINE__));


  // on read check if file has dictionary, warn if conflict with user dictionary
  // store file XML just in case
  // set dictionary on write
  if((mode=="r")||(mode=="ra")) {
    char *d;
    uint32_t len;
    stat=evGetDictionary(handle,&d,&len);
    if((stat==S_SUCCESS)&&(d!=NULL)&&(len>0))fileXMLDictionary = string(d);

    if(dictionary==NULL) {
      if(stat!=S_SUCCESS)throw(evioException(stat,"?evioFileChannel::open...bad dictionary in file: " + string(evPerror(stat)),
                                             __FILE__,__FUNCTION__,__LINE__));
      if((d!=NULL)&&(len>0)) {
        dictionary = new evioDictionary(d);
        createdFileDictionary=true;
      }
    } else {
      cout << "evioFileChannel::open...user-supplied dictionary overrides dictionary in file" << endl;
    }
  // else if writing, splitting, or appending ...
  } else {
    // If writing or splitting when writing, dictionary must be written first.
    // If appending, dictionary cannot be written.
    if((dictionary!=NULL) && ((mode=="w") || (mode=="s"))) {
      stat=evWriteDictionary(handle,const_cast<char*>(dictionary->getDictionaryXML().c_str()));
      if(stat!=S_SUCCESS)throw(evioException(stat,"?evioFileChannel::open...error writing dictionary in file: " + string(evPerror(stat)),
                                             __FILE__,__FUNCTION__,__LINE__));
    }

    // If writing, splitting when writing, or appending, write first event now
    if(firstEvent!=NULL) {
      stat=evWriteFirstEvent(handle,firstEvent);
      if(stat!=S_SUCCESS)throw(evioException(stat,"?evioFileChannel::open...error writing first event in file: " + string(evPerror(stat)),
                                             __FILE__,__FUNCTION__,__LINE__));
    }
  }

}


//-----------------------------------------------------------------------


/**
 * Reads from file into internal buffer.
 * @return true if successful, false on EOF or other evRead error condition
 */
bool evioFileChannel::read(void) throw(evioException) {
  noCopyBuf=NULL;
  if(buf==NULL)throw(evioException(0,"evioFileChannel::read...null buffer",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioFileChannel::read...0 handle",__FILE__,__FUNCTION__,__LINE__));
  int stat=evRead(handle,&buf[0], (uint32_t)bufSize);
  if(stat==S_SUCCESS) {
    return(true);
  } else if(stat==EOF) {
    return(false);
  } else {
    throw(evioException(stat,"evioFileChannel::read...read error" + string(evPerror(stat)),__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------


/**
 * Reads from file into user-supplied buffer.
 * @param myBuf User-supplied buffer.
 * @parem length Length of buffer in 4-byte words.
 * @return true if successful, false on EOF or other evRead error condition
 */
bool evioFileChannel::read(uint32_t *myBuf, int length) throw(evioException) {
  noCopyBuf=NULL;
  if(myBuf==NULL)throw(evioException(0,"evioFileChannel::read...null user buffer",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioFileChannel::read...0 handle",__FILE__,__FUNCTION__,__LINE__));
  int stat=evRead(handle,&myBuf[0], (uint32_t)length);
  if(stat==S_SUCCESS) {
    return(true);
  } else if(stat==EOF) {
    return(false);
  } else {
    throw(evioException(stat,"evioFileChannel::read...read error" + string(evPerror(stat)),__FILE__,__FUNCTION__,__LINE__));
  }
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
bool evioFileChannel::readAlloc(uint32_t **buffer, uint32_t *bufLen) throw(evioException) {
  noCopyBuf=NULL;
  if(handle==0)throw(evioException(0,"evioFileChannel::readAlloc...0 handle",__FILE__,__FUNCTION__,__LINE__));

  int stat=evReadAlloc(handle,buffer,bufLen);
  if(stat==EOF) {
    *buffer=NULL;
    *bufLen=0;
    return(false);
  }

  if(stat!=S_SUCCESS) throw(evioException(stat,"evioFileChannel::readAlloc...read error: " + string(evPerror(stat)),
                                          __FILE__,__FUNCTION__,__LINE__));
  return(true);
}


//-----------------------------------------------------------------------


/**
 * Reads from file using no copy mechanism.
 * @return true if successful, false on EOF, throws exception for other error.
 */
bool evioFileChannel::readNoCopy(void) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioFileChannel::readNoCopy...0 handle",__FILE__,__FUNCTION__,__LINE__));

  uint32_t bufLen;
  int stat=evReadNoCopy(handle,&noCopyBuf,&bufLen);
  if(stat==EOF) return(false);
  if(stat!=S_SUCCESS) throw(evioException(stat,"evioFileChannel::readNoCopy...read error: " + string(evPerror(stat)),
                                          __FILE__,__FUNCTION__,__LINE__));
  return(true);
}


//-----------------------------------------------------------------------


/**
 * Reads buffer from file given buffer number
 * @param bufferNumber Buffer to return
 * @return true if successful, false on EOF, throws exception for other error.
 */
bool evioFileChannel::readRandom(uint32_t bufferNumber) throw(evioException) {
  noCopyBuf=NULL;
  if(handle==0)throw(evioException(0,"evioFileChannel::readRandom...0 handle",__FILE__,__FUNCTION__,__LINE__));
  
  uint32_t bufLen;
  int stat=evReadRandom(handle,&randomBuf,&bufLen,bufferNumber);
  if(stat==EOF) return(false);
  if(stat!=S_SUCCESS) throw(evioException(stat,"evioFileChannel::readRandom...read error: " + string(evPerror(stat)),
                                          __FILE__,__FUNCTION__,__LINE__));
  return(true);
}


//-----------------------------------------------------------------------


/**
 * Writes to file from internal buffer.
 */
void evioFileChannel::write(void) throw(evioException) {
  int stat;
  if(buf==NULL)throw(evioException(0,"evioFileChannel::write...null buffer",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioFileChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evWrite(handle,buf))!=0) throw(evioException(stat,"?evioFileChannel::write...unable to write: " + string(evPerror(stat)),
                                                      __FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Writes to file from user-supplied buffer.
 * @param myBuf Buffer containing event
 */
void evioFileChannel::write(const uint32_t *myBuf) throw(evioException) {
  int stat;
  if(myBuf==NULL)throw(evioException(0,"evioFileChannel::write...null myBuf",__FILE__,__FUNCTION__,__LINE__));
  if(handle==0)throw(evioException(0,"evioFileChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evWrite(handle,myBuf))!=0) throw(evioException(stat,"?evioFileChannel::write...unable to write from myBuf: " + string(evPerror(stat)),
                                                        __FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Writes to file from internal buffer of another evioChannel object.
 * @param channel Channel object
 */
void evioFileChannel::write(const evioChannel &channel) throw(evioException) {
  int stat;
  if(handle==0)throw(evioException(0,"evioFileChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  if((stat=evWrite(handle,channel.getBuffer()))!=0) throw(evioException(stat,
       "?evioFileChannel::write...unable to write from channel: " + string(evPerror(stat)),
       __FILE__,__FUNCTION__,__LINE__));
}


//-----------------------------------------------------------------------


/**
 * Writes from internal buffer of another evioChannel object.
 * @param channel Pointer to channel object
 */
void evioFileChannel::write(const evioChannel *channel) throw(evioException) {
  if(channel==NULL)throw(evioException(0,"evioFileChannel::write...null channel",__FILE__,__FUNCTION__,__LINE__));
  evioFileChannel::write(*channel);
}


//-----------------------------------------------------------------------


/**
 * Writes from contents of evioChannelBufferizable object.
 * @param o evioChannelBufferizable object that can serialze to buffer
 */
void evioFileChannel::write(const evioChannelBufferizable &o) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioFileChannel::write...0 handle",__FILE__,__FUNCTION__,__LINE__));
  o.toEVIOBuffer(buf,bufSize);
  evioFileChannel::write();
}


//-----------------------------------------------------------------------


/**
 * Writes from contents of evioChannelBufferizable object.
 * @param o Pointer to evioChannelBufferizable object that can serialize to buffer
 */
void evioFileChannel::write(const evioChannelBufferizable *o) throw(evioException) {
  if(o==NULL)throw(evioException(0,"evioFileChannel::write...null evioChannel Bufferizable pointer",__FILE__,__FUNCTION__,__LINE__));
  evioFileChannel::write(*o);
}


//-----------------------------------------------------------------------


/**
 * For getting and setting evIoctl parameters.
 * @param request String containing evIoctl parameters
 * @param argp Additional evIoctl parameter
 */
int evioFileChannel::ioctl(const string &request, void *argp) throw(evioException) {
  int stat;
  if(handle==0)throw(evioException(0,"evioFileChannel::ioctl...0 handle",__FILE__,__FUNCTION__,__LINE__));
  stat=evIoctl(handle,const_cast<char*>(request.c_str()),argp)!=0;
  if(stat!=S_SUCCESS)throw(evioException(stat,"?evioFileChannel::ioCtl...error return: " + string(evPerror(stat)),
                                         __FILE__,__FUNCTION__,__LINE__));
  return(stat);
}


//-----------------------------------------------------------------------


/**
 * Closes channel.
 */
void evioFileChannel::close(void) throw(evioException) {
  if(handle==0)throw(evioException(0,"evioFileChannel::close...0 handle",__FILE__,__FUNCTION__,__LINE__));
  evClose(handle);
  handle=0;
}


//-----------------------------------------------------------------------


/**
 * Returns channel file name.
 * @return String containing file name
 */
string evioFileChannel::getFileName(void) const {
  return(filename);
}


//-----------------------------------------------------------------------


/**
 * Returns channel I/O mode.
 * @return String containing I/O mode
 */
string evioFileChannel::getMode(void) const {
  return(mode);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to internal channel buffer.
 * @return Pointer to internal buffer
 */
const uint32_t *evioFileChannel::getBuffer(void) const throw(evioException) {
  if(buf==NULL)throw(evioException(0,"evioFileChannel::getbuffer...null buffer",__FILE__,__FUNCTION__,__LINE__));
  return(buf);
}


//-----------------------------------------------------------------------


/**
 * Returns internal channel buffer size.
 * @return Internal buffer size in 4-byte words
 */
int evioFileChannel::getBufSize(void) const {
  return(bufSize);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to no copy buffer.
 * @return Pointer to no copy buffer
 */
const uint32_t *evioFileChannel::getNoCopyBuffer(void) const throw(evioException) {
  return(noCopyBuf);
}


//-----------------------------------------------------------------------


/**
 * Returns pointer to random buffer.
 * @return Pointer to random buffer
 */
const uint32_t *evioFileChannel::getRandomBuffer(void) const throw(evioException) {
  return(randomBuf);
}


//-----------------------------------------------------------------------


/**
 * Returns XML dictionary read in from file
 * @return XML dictionary read in from file
 */
string evioFileChannel::getFileXMLDictionary(void) const {
  return(fileXMLDictionary);
}


//-----------------------------------------------------------------------


/**
 * Returns random access table
 * @param table Pointer to table
 * @param len Length of table
 */
void evioFileChannel::getRandomAccessTable(uint32_t *** const table, uint32_t *len) const throw(evioException) {
  if(handle==0)throw(evioException(0,"evioFileChannel::getRandomAccessTable...0 handle",__FILE__,__FUNCTION__,__LINE__));
  evGetRandomAccessTable(handle,table,len);
}


//-----------------------------------------------------------------------
