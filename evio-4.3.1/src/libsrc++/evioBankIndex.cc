/*
 *  evioBankIndex.cc
 *
 *   Author:  Elliott Wolin, JLab, 1-May-2012
*/


#include "evioBankIndex.hxx"
#include "evioUtil.hxx"

using namespace evio;
using namespace std;



//--------------------------------------------------------------
//--------------------------------------------------------------


// used internally to stream-parse the event and fill the tagNum map
class myHandler : public evioStreamParserHandler {
  
public:
  myHandler(int maxDepth) : maxDepth(maxDepth) {}

  void *containerNodeHandler(int bankLength, int containerType, int contentType, uint16_t tag, uint8_t num, 
                             int depth, const uint32_t *bankPointer, int dataLength, const uint32_t *data, void *userArg) {

    // don't index beyond specified depth
    // Note...maxDepth 0 means index all levels, 1 means just include children level, etc.
    // Note...current depth 1 means you are at children level, 2 means grandchildren level, etc.
    if((maxDepth>0)&&(depth>maxDepth))return(userArg);


    // adds bank index to map
    evioBankIndex *bi = static_cast<evioBankIndex*>(userArg);
  
    bankIndex b;
    b.containerType=containerType;
    b.contentType=contentType;
    b.depth=depth;
    b.bankPointer=bankPointer;
    b.bankLength=bankLength;
    b.data=data;
    b.dataLength=dataLength;
    bi->tagNumMap.insert(bankIndexMap::value_type(tagNum(tag,num),b));
    
    return(userArg);
  }
  
  
  //--------------------------------------------------------------
  
  
  void *leafNodeHandler(int bankLength, int containerType, int contentType, uint16_t tag, uint8_t num, 
                        int depth, const uint32_t *bankPointer, int dataLength, const void *data, void *userArg) {
    
    // don't index beyond specified depth
    // Note...maxDepth 0 means index all levels, 1 means just include children level, etc.
    // Note...current depth 1 means you are at children level, 2 means grandchildren level, etc.
    if((maxDepth>0)&&(depth>maxDepth))return(userArg);


    // adds bank index to map
    evioBankIndex *bi = static_cast<evioBankIndex*>(userArg);

    bankIndex b;
    b.containerType=containerType;
    b.contentType=contentType;
    b.depth=depth;
    b.bankPointer=bankPointer;
    b.bankLength=bankLength;
    b.data=data;
    b.dataLength=dataLength;
    bi->tagNumMap.insert(bankIndexMap::value_type(tagNum(tag,num),b));

    return(userArg);
  }

private:
   int maxDepth;

};


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------


/**
 * Constructor.
 */
evioBankIndex::evioBankIndex(int maxDepth) : maxDepth(maxDepth) {
}


//-----------------------------------------------------------------------


/**
 * Constructor from buffer.
 *
 * @param buffer Event buffer to index
 */
evioBankIndex::evioBankIndex(const uint32_t *buffer, int maxDepth) : maxDepth(maxDepth) {
  parseBuffer(buffer,maxDepth);
}


//-----------------------------------------------------------------------


/**
 * Destructor.
 */
evioBankIndex::~evioBankIndex() {
}


//-----------------------------------------------------------------------


/**
 * Indexes buffer and fills map.
 * @param buffer Buffer containing serialized event
 * @return true if indexing succeeded
 */
bool evioBankIndex::parseBuffer(const uint32_t *buffer, int maxDepth) {

  // create parser and handler
  evioStreamParser p;
  myHandler h(maxDepth);
  p.parse(buffer,h,((void*)this));

  return(true);
}


//-----------------------------------------------------------------------


/**
 * True if tagNum is in map at least once.
 * @param tn tagNum
 * @return true if tagNum is in map
 */
bool evioBankIndex::tagNumExists(const tagNum& tn) const {
  bankIndexMap::const_iterator iter = tagNumMap.find(tn);
  return(iter!=tagNumMap.end());
}


//-----------------------------------------------------------------------


/**
 * Returns count of tagNum in map.
 * @param tn tagNum
 * @return Count of tagNum in map.
 */
int evioBankIndex::tagNumCount(const tagNum& tn) const {
  return((int)tagNumMap.count(tn));
}


//-----------------------------------------------------------------------


/**
 * Returns pair of iterators defining range of equal keys in tagNumMap.
 * @param tn tagNum
 * @return Pair of iterators defining range
 */
bankIndexRange evioBankIndex::getRange(const tagNum& tn) const {
  return(tagNumMap.equal_range(tn));
}


//-----------------------------------------------------------------------


/**
 * Returns bankIndex given tagNum, throws exception if no entry found
 * @param tn tagNum
 * @return bankIndex for tagNum
 */
bankIndex evioBankIndex::getBankIndex(const tagNum &tn) const throw(evioException) {

  bankIndexMap::const_iterator iter = tagNumMap.find(tn);
  if(iter!=tagNumMap.end()) {
    return((*iter).second);
  } else {
    throw(evioException(0,"?evioBankIndex::getBankIndex...tagNum not found",__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
