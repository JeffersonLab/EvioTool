/*
 *  evioUtil.cc
 *
 *   Author:  Elliott Wolin, JLab, 11-dec-2006
*/


#ifdef vxworks
#include <iostream.h>
#endif
#include "evioUtil.hxx"

using namespace std;
using namespace evio;



//--------------------------------------------------------------
//-------------------- local utilities -------------------------
//--------------------------------------------------------------


namespace evio {

  /**
   * Only used internally, always returns true.
   * @return true Always!
   */
  static bool isTrue(const evioDOMNodeP pNode) {
    return(true);
  }
  
  
//--------------------------------------------------------------

}  // namespace evio




//-----------------------------------------------------------------------
//---------------------- evioUtilities ----------------------------
//-----------------------------------------------------------------------


/**
 * Appends structure to buffer in newly allocated memory.
 * Throws exception if buffer content type and structure container type don't match.
 *   I.e. cannot add a segment structure to a bank of banks.
 *
 * @param buffer Buffer
 * @param bufferType Container type of buffer (BANK, SEGMENT or TAGSEGMENT)
 * @param structure Structure to append to buffer
 * @param structureType Container type of structure
 * @return Pointer to new buffer
 */
uint32_t *evioUtilities::appendToBuffer(const uint32_t *buffer, ContainerType bufferType, const uint32_t *structure, ContainerType structureType)
    throw(evioException) {


  // get buffer content type
  int bufferContentType;
  if(bufferType==BANK) {
    bufferContentType=(buffer[1]>>8)&0x3f;
  } else if(bufferType==SEGMENT) {
    bufferContentType=(buffer[0]>>16)&0x3f;
  } else {
    bufferContentType=(buffer[0]>>16)&0xf;
  }


  // does structure type match buffer content type
  if(bufferContentType!=structureType) throw(evioException(0,"?evioUtilties::appendToBuffer...types do not match",__FILE__,__FUNCTION__,__LINE__));


  // get buffer length
  unsigned int bufferLength;
  if(bufferType==BANK) {
    bufferLength = buffer[0]+1;
  } else {
    bufferLength = buffer[0]&0xffff + 1;
  }


  // get structure length
  unsigned int structureLength;
  if(structureType==BANK) {
    structureLength=structure[0]+1;
  } else {
    structureLength=(buffer[0]&0xffff)+1;
  }


  // check combined length fits in 32 bits for BANK, 16 bits for SEGMENT and TAGSEGMENT
  if(bufferType==BANK) {
    if(((uint64_t)bufferLength+(uint64_t)structureLength)>0xffffffff)
      throw(evioException(0,"?evioUtilties::appendToBuffer...combined length does not fit in 32 bits",__FILE__,__FUNCTION__,__LINE__));
  } else if((bufferLength+structureLength)>0xffff) {
    throw(evioException(0,"?evioUtilties::appendToBuffer...combined length does not fit in 16 bits",__FILE__,__FUNCTION__,__LINE__));
  }


  // allocate new buffer
  uint32_t *newBuffer = (uint32_t*)malloc((bufferLength+structureLength)*sizeof(uint32_t));


  // copy buffer into new buffer
  memcpy((void*)newBuffer,(void*)buffer,bufferLength*sizeof(uint32_t));


  // append structure to new buffer
  memcpy((void*)(&newBuffer[bufferLength]),(void*)structure,structureLength*sizeof(uint32_t));


  // update new buffer length
  newBuffer[0]+=structureLength;


  // return pointer to new buffer
  return(newBuffer);

 }


//-----------------------------------------------------------------------
//---------------------- evioToString Config ----------------------------
//-----------------------------------------------------------------------


/**
 * No-arg constructor with no dictionary.
 */
evioToStringConfig::evioToStringConfig() {
  init();
}


//-----------------------------------------------------------------------


/**
 * Constructor accepts dictionary.
 * @param dictionary evioDictionary
 */
evioToStringConfig::evioToStringConfig(const evioDictionary *dictionary) {
  init();
  toStringDictionary=dictionary;
}


//-----------------------------------------------------------------------


/**
 * Constructor accepts dictionary.
 * @param dictionary evioDictionary
 */
evioToStringConfig::evioToStringConfig(const evioDictionary &dictionary) {
  init();
  toStringDictionary=&dictionary;
}


//-----------------------------------------------------------------------


/**
 * Destructor.
 */
evioToStringConfig::~evioToStringConfig() {
}


//-----------------------------------------------------------------------


/**
 * Initializes config.
 */
void evioToStringConfig::init(void) {
  xtod               = false;
  noData             = false;
  maxDepth           = 0;
  indentSize         = 3;    
  toStringDictionary = NULL;
  verbose            = false;
}


//-----------------------------------------------------------------------


/**
 * Checks if bank tag/num is listed in include/exclude vectors by tag/num OR by name.
 * @param pNode Node under consideration
 * @return true to skip this node
 */
bool evioToStringConfig::skipNode(const evioDOMNodeP pNode) const {

  // if dictionary exists search for bank name given tag/num
  tagNum tn(pNode->tag,pNode->num);
  string name;
  if((toStringDictionary!=NULL)&&(toStringDictionary->getNameMap.find(tn)!=toStringDictionary->getNameMap.end()))
    name=toStringDictionary->getName(tn);
  

  // priority:  name included, tag included, name excluded, tag excluded
  if( (toStringDictionary!=NULL) && (name.size()>0) && (bankNameOk.size()>0) )
    return(find(bankNameOk.begin(),bankNameOk.end(),name)==bankNameOk.end());

  if(bankOk.size()>0)
    return(find(bankOk.begin(),bankOk.end(),pNode->tag)==bankOk.end());

  if( (toStringDictionary!=NULL) && (name.size()>0) && (noBankName.size()>0) ) 
    return(find(noBankName.begin(),noBankName.end(),name)!=bankNameOk.end());

  if(noBank.size()>0) 
    return(find(noBank.begin(),noBank.end(),pNode->tag)!=noBank.end());


  // it's a keeper
  return(false);
}


//-----------------------------------------------------------------------
//------------------------ evioStreamParser -----------------------------
//-----------------------------------------------------------------------


/**
 * Stream parses event in buffer.
 * @param buf Buffer containing event
 * @param handler evioStreamParserHandler object containing callbacks to handle container and leaf nodes
 * @param userArg Passed to handler callbacks
 * @return void* Pointer to root bank of new tree
 */
void *evioStreamParser::parse(const uint32_t *buf, 
                              evioStreamParserHandler &handler, void *userArg) throw(evioException) {
  
  if(buf==NULL)throw(evioException(0,"?evioStreamParser::parse...null buffer",__FILE__,__FUNCTION__,__LINE__));
  return((void*)parseBank(buf,BANK,0,handler,userArg));
}


//--------------------------------------------------------------


/**
 * Used internally to parse banks.
 * @param buf Buffer containing bank
 * @param bankType Bank type
 * @param depth Current depth in depth-first parse of event (zero means children of main bank, 1 means grandchildren, etc)
 * @param handler evioStreamParserHandler object containing callbacks to handle container and leaf nodes
 * @param userArg Passed to handler callbacks
 * @return void* Used internally
 */
void *evioStreamParser::parseBank(const uint32_t *buf, int bankType, int depth, 
                                 evioStreamParserHandler &handler, void *userArg) throw(evioException) {

  int length,dataOffset,p,bankLen;
  int contentType;
  uint16_t tag;
  uint8_t num;
  const uint32_t *data;
  uint32_t mask;
  int padding;

  void *newUserArg = userArg;


  /* get type-dependent info */
  switch(bankType) {

  case 0xe:
  case 0x10:
    length  	= buf[0]+1;
    tag     	= buf[1]>>16;
    contentType	= (buf[1]>>8)&0x3f;
    num     	= buf[1]&0xff;
    padding     = (buf[1]>>14)&0x3;
    dataOffset  = 2;
    break;

  case 0xd:
  case 0x20:
    length  	= (buf[0]&0xffff)+1;
    tag     	= buf[0]>>24;
    contentType = (buf[0]>>16)&0x3f;
    num     	= 0;
    padding     = (buf[0]>>22)&0x3;
    dataOffset  = 1;
    break;
    
  case 0xc:
  case 0x40:
    length  	= (buf[0]&0xffff)+1;
    tag     	= buf[0]>>20;
    contentType	= (buf[0]>>16)&0xf;
    num     	= 0;
    padding     = 0;
    dataOffset  = 1;
    break;

  default:
    ostringstream ss;
    ss << hex << showbase << bankType << noshowbase << dec;
    throw(evioException(0,"?evioStreamParser::parseBank...illegal bank type: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
  }


  /* 
   * if a leaf node, call leaf handler.
   * if container node, call node handler and then parse contained banks.
   * note:  padding refers to the number of extra bytes
   */

  void *newLeaf = NULL;
  switch (contentType) {

  case 0x0:
  case 0x1:
  case 0x2:
  case 0xb:
  case 0xf:
    // four-byte types
    newLeaf=handler.leafNodeHandler(length,bankType,contentType,tag,num,depth,&buf[0],length-dataOffset,&buf[dataOffset],userArg);
    break;

  case 0x3:
  case 0x6:
  case 0x7:
    // one-byte types
    newLeaf=handler.leafNodeHandler(length,bankType,contentType,tag,num,depth,&buf[0],(length-dataOffset)*4-padding,
                                    (int8_t*)(&buf[dataOffset]),userArg);
    break;

  case 0x4:
  case 0x5:
    // two-byte types
    newLeaf=handler.leafNodeHandler(length,bankType,contentType,tag,num,depth,
                                    &buf[0],(length-dataOffset)*2-padding/2,(int16_t*)(&buf[dataOffset]),userArg);
    break;

  case 0x8:
  case 0x9:
  case 0xa:
    // eight-byte types
    newLeaf=handler.leafNodeHandler(length,bankType,contentType,tag,num,depth,
                                    &buf[0],(length-dataOffset)/2,(int64_t*)(&buf[dataOffset]),userArg);
    break;

  case 0xe:
  case 0x10:
  case 0xd:
  case 0x20:
  case 0xc:
  case 0x40:
    // container types
    newUserArg=handler.containerNodeHandler(length,bankType,contentType,tag,num,depth,&buf[0],length-dataOffset,&buf[dataOffset],userArg);


    // parse contained banks
    p       = 0;
    bankLen = length-dataOffset;
    data    = &buf[dataOffset];
    mask    = ((contentType==0xe)||(contentType==0x10))?0xffffffff:0xffff;

    depth++;
    while(p<bankLen) {
      parseBank(&data[p],contentType,depth,handler,newUserArg);
      p+=(data[p]&mask)+1;
    }
    depth--;

    break;


  default:
    ostringstream ss;
    ss << hex << showbase << contentType << noshowbase << dec;
    throw(evioException(0,"?evioStreamParser::parseBank...illegal content type: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
    break;

  }


  // new user arg is pointer to new node
  if(newLeaf!=NULL) {
    return(newLeaf);
  } else {
    return(newUserArg);
  }

}


//-----------------------------------------------------------------------------
//---------------------------- evioDOMNode ------------------------------------
//-----------------------------------------------------------------------------


/** 
 * Container node constructor used internally.
 * @param par Parent node
 * @param tag Node tag
 * @param num Node num
 * @param contentType Container node content type
 */
evioDOMNode::evioDOMNode(evioDOMNodeP par, uint16_t tag, uint8_t num, int contentType) throw(evioException)
  : parent(par), parentTree(NULL), contentType(contentType), tag(tag), num(num)  {
}


//-----------------------------------------------------------------------------


/** 
 * Container node constructor used internally.
 * @param par Parent node
 * @param name Bank name
 * @parem dictionary Dictionary to use
 * @param contentType Container node content type
 */
evioDOMNode::evioDOMNode(evioDOMNodeP par, const string &name, const evioDictionary *dictionary, int contentType) throw(evioException)
  : parent(par), parentTree(NULL), contentType(contentType) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    tag=tn.first;
    num=tn.second;
  } else {
    throw(evioException(0,"?evioDOMNode constructor...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create container node.
 * @param tag Node tag
 * @param num Node num
 * @param cType Container node content type
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(uint16_t tag, uint8_t num, ContainerType cType) throw(evioException) {
  return(new evioDOMContainerNode(NULL,tag,num,cType));
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create container node.
 * @param name Node name
 * @param dictionary Dictionary to use
 * @param cType Container node content type
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(const string &name, const evioDictionary *dictionary, ContainerType cType) throw(evioException) {
  
  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    return(new evioDOMContainerNode(NULL,tn.first,tn.second,cType));
  } else {
    throw(evioException(0,"?evioDOMNode::createEvioDOMNode...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }

}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create container node holding evioSerializable object.
 * @param tag Node tag
 * @param num Node num
 * @param o evioSerializable object
 * @param cType Container node content type
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(uint16_t tag, uint8_t num, const evioSerializable &o, ContainerType cType) 
  throw(evioException) {
  evioDOMContainerNode *c = new evioDOMContainerNode(NULL,tag,num,cType);
  o.serialize(c);
  return(c);
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create container node holding evioSerializable object.
 * @param name Node name
 * @param dictionary Dictionary to use
 * @param o evioSerializable object
 * @param cType Container node content type
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(const string &name, const evioDictionary *dictionary, const evioSerializable &o, ContainerType cType) 
  throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    evioDOMContainerNode *c = new evioDOMContainerNode(NULL,tn.first,tn.second,cType);
    o.serialize(c);
    return(c);
  } else {
    throw(evioException(0,"?evioDOMNode::createEvioDOMNode...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }

}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create container node using C function to fill container.
 * @param tag Node tag
 * @param num Node num
 * @param f C function that fills container node
 * @param userArg User arg passed to C function
 * @param cType Container node content type
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(uint16_t tag, uint8_t num, void (*f)(evioDOMNodeP c, void *userArg), 
                                            void *userArg, ContainerType cType) throw(evioException) {
  evioDOMContainerNode *c = new evioDOMContainerNode(NULL,tag,num,cType);
  f(c,userArg);
  return(c);
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create composite leaf node.
 * @param tag Node tag
 * @param num Node num
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param tVec vector<uint32_t> of data
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(uint16_t tag, uint8_t num, uint16_t formatTag, const string &formatString,
                                            uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &tVec) throw(evioException) {
  return(new evioCompositeDOMLeafNode(NULL,tag,num,formatTag,formatString,dataTag,dataNum,tVec));
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create composite leaf node.
 * @param tag Node tag
 * @param num Node num
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param t Pointer to uint32_t data
 * @param len Length of data array
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(uint16_t tag, uint8_t num, uint16_t formatTag, const string &formatString,
                                            uint16_t dataTag, uint8_t dataNum, const uint32_t* t, int len) throw(evioException) {
  return(new evioCompositeDOMLeafNode(NULL,tag,num,formatTag,formatString,dataTag,dataNum,t,len));
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create composite leaf node.
 * @param name Node name
 * @param dictionary Dictionary to use
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param tVec vector<uint32_t> of data
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(const string &name, const evioDictionary *dictionary, uint16_t formatTag, const string &formatString,
                                            uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &tVec) throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    return(new evioCompositeDOMLeafNode(NULL,tn.first,tn.second,formatTag,formatString,dataTag,dataNum,tVec));
  } else {
    throw(evioException(0,"?evioDOMNode::createEvioDOMNode...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }

}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create composite leaf node.
 * @param name Node name
 * @param dictionary Dictionary to use
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param t Pointer to array of uint32_t data
 * @param len Length of array
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(const string &name, const evioDictionary *dictionary, uint16_t formatTag, const string &formatString,
                                            uint16_t dataTag, uint8_t dataNum, const uint32_t *t, int len) throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    return(new evioCompositeDOMLeafNode(NULL,tn.first,tn.second,formatTag,formatString,dataTag,dataNum,t,len));
  } else {
    throw(evioException(0,"?evioDOMNode::createEvioDOMNode...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }

}


//-----------------------------------------------------------------------------


/** 
 * Unknown type node constructor used internally.
 * @param tag Tag
 * @param num Num
 * @param tVec Vector of uint32_t
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createUnknownEvioDOMNode(uint16_t tag, uint8_t num, const vector<uint32_t> &tVec) throw(evioException) {
  evioDOMNodeP p = evioDOMNode::createEvioDOMNode(tag,num,tVec);
  p->contentType=0x0;
  return(p);
}


//-----------------------------------------------------------------------------


/** 
 * Unknown type node constructor used internally.
 * @param tag Tag
 * @param num Num
 * @param t Pointer to uint32_t array
 * @param len Length of array
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createUnknownEvioDOMNode(uint16_t tag, uint8_t num, const uint32_t *t, int len) throw(evioException) {
  evioDOMNodeP p = evioDOMNode::createEvioDOMNode(tag,num,t,len);
  p->contentType=0x0;
  return(p);
}


//-----------------------------------------------------------------------------


/** 
 * Unknown type node constructor used internally.
 * @param name Name
 * @param dictionary Dictionary
 * @param tVec Vector of uint32_t
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createUnknownEvioDOMNode(const string &name, const evioDictionary *dictionary, const vector<uint32_t> &tVec)
  throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    evioDOMNodeP p = evioDOMNode::createEvioDOMNode(tn.first,tn.second,tVec);
    p->contentType=0x0;
    return(p);
  } else {
    throw(evioException(0,"?evioUnknwonnDOMNode constructor...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------------


/** 
 * Unknown type node constructor used internally.
 * @param name Name
 * @param dictionary Dictionary
 * @param t Pointer to array of uint32_t
 * @param len Length of array
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createUnknownEvioDOMNode(const string &name, const evioDictionary *dictionary, const uint32_t* t, int len)
  throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    evioDOMNodeP p = evioDOMNode::createEvioDOMNode(tn.first,tn.second,t,len);
    p->contentType=0x0;
    return(p);
  } else {
    throw(evioException(0,"?evioUnknwonnDOMNode constructor...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------------


/** 
 * Static factory method to create container node using C function to fill container.
 * @param name Node name
 * @param dictionary Dictionary to use
 * @param f C function that fills container node
 * @param userArg User arg passed to C function
 * @param cType Container node content type
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMNode::createEvioDOMNode(const string &name, const evioDictionary *dictionary, void (*f)(evioDOMNodeP c, void *userArg), 
                                            void *userArg, ContainerType cType) throw(evioException) {
  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    evioDOMContainerNode *c = new evioDOMContainerNode(NULL,tn.first,tn.second,cType);
    f(c,userArg);
    return(c);
  } else {
    throw(evioException(0,"?evioDOMNode constructor...NULL dictionary for bank name: " + name,__FILE__,__FUNCTION__,__LINE__));
  }

}


//-----------------------------------------------------------------------------


/*
 * Destructor
 */
evioDOMNode::~evioDOMNode(void) {
}


//-----------------------------------------------------------------------------


/** 
 * Cleanly removes node from tree or node hierarchy.
 * @return Pointer to now liberated node
 */
evioDOMNodeP evioDOMNode::cut(void) throw(evioException) {
  if(parent!=NULL) {
    evioDOMContainerNode *par = static_cast<evioDOMContainerNode*>(parent);
    par->childList.remove(this);
    parent=NULL;
  } else if (parentTree!=NULL) {
    parentTree->root=NULL;
    parentTree=NULL;
  }
  return(this);
}


//-----------------------------------------------------------------------------


/** 
 * Cleanly removes node from tree or node hierarchy and recursively deletes node and its contents.
 */
void evioDOMNode::cutAndDelete(void) throw(evioException) {
  cut();
  delete(this);
}


//-----------------------------------------------------------------------------


/** 
 * Changes node parent.
 * @param newParent New parent
 * @return Pointer to now node just moved
 */
evioDOMNodeP evioDOMNode::move(evioDOMNodeP newParent) throw(evioException) {

  cut();

  if(!newParent->isContainer())throw(evioException(0,"?evioDOMNode::move...parent node not a container",__FILE__,__FUNCTION__,__LINE__));
  evioDOMContainerNode *par = static_cast<evioDOMContainerNode*>(newParent);
  
  par->childList.push_back(this);
  parent=newParent;
  return(this);
}


//-----------------------------------------------------------------------------


/** 
 * Adds node to container node.
 * @param node Node to be added
 */
void evioDOMNode::addNode(evioDOMNodeP node) throw(evioException) {
  if(node==NULL)return;
  if(!isContainer())throw(evioException(0,"?evioDOMNode::addNode...not a container",__FILE__,__FUNCTION__,__LINE__));
  node->move(this);
}


//-----------------------------------------------------------------------------


/** 
 * Appends string to leaf node.
 * @param s string to append
 */
void evioDOMNode::append(const string &s) throw(evioException) {
  if(contentType!=evioUtil<string>::evioContentType())
    throw(evioException(0,"?evioDOMNode::append...not appropriate node",__FILE__,__FUNCTION__,__LINE__));
  evioDOMLeafNode<string> *l = static_cast<evioDOMLeafNode<string>*>(this);
  l->data.push_back(s);
}


//-----------------------------------------------------------------------------


/** 
 * Appends const char* to leaf node.
 * @param s char* to append
 */
void evioDOMNode::append(const char *s) throw(evioException) {
  if(contentType!=evioUtil<string>::evioContentType())
    throw(evioException(0,"?evioDOMNode::append...not appropriate node",__FILE__,__FUNCTION__,__LINE__));
  evioDOMLeafNode<string> *l = static_cast<evioDOMLeafNode<string>*>(this);
  l->data.push_back(s);
}


//-----------------------------------------------------------------------------


/** 
 * Appends char* to leaf node.
 * @param s char* to append
 */
void evioDOMNode::append(char *s) throw(evioException) {
  append((const char*)s);
}


//-----------------------------------------------------------------------------


/** 
 * Appends array of const char* to leaf node.
 * @param sa array of char* to append
 */
void evioDOMNode::append(const char **ca, int len) throw(evioException) {
  if(contentType!=evioUtil<string>::evioContentType())
    throw(evioException(0,"?evioDOMNode::append...not appropriate node",__FILE__,__FUNCTION__,__LINE__));
  evioDOMLeafNode<string> *l = static_cast<evioDOMLeafNode<string>*>(this);
  for(unsigned int i=0; (int)i<len; i++) {
    l->data.push_back(ca[i]);
  }
}


//-----------------------------------------------------------------------------


/** 
 * Appends array of const char* to leaf node.
 * @param sa array of char* to append
 */
void evioDOMNode::append(char **ca, int len) throw(evioException) {
  append((const char**)ca,len);
}


//-----------------------------------------------------------------------------


/** 
 * Appends single string to leaf node
 * @param sRef String to be added
 * @return Reference to this
 */
evioDOMNode& evioDOMNode::operator<<(const string &s) throw(evioException) {
  append(s);
  return(*this);
}


//-----------------------------------------------------------------------------


/** 
 * Appends single const char* to leaf node
 * @param s char* to be added
 * @return Reference to this
 */
evioDOMNode& evioDOMNode::operator<<(const char *s) throw(evioException) {
  append(s);
  return(*this);
}


//-----------------------------------------------------------------------------


/** 
 * Appends single char* to leaf node
 * @param s char* to be added
 * @return Reference to this
 */
evioDOMNode& evioDOMNode::operator<<(char *s) throw(evioException) {
  append((const char*)s);
  return(*this);
}


//-----------------------------------------------------------------------------


/** 
 * Returns pointer to child list of container node.
 * @return Pointer to internal child list
 */
evioDOMNodeList *evioDOMNode::getChildList(void) throw(evioException) {
  if(!isContainer())return(NULL);
  evioDOMContainerNode *c = static_cast<evioDOMContainerNode*>(this);
  return(&c->childList);
}


//-----------------------------------------------------------------------------


/** 
 * Returns copy of child list of container node.
 * List must be deleted by user
 * @return Copy of child list
 */

//evioDOMNodeList *evioDOMNode::getChildren(void) throw(evioException) {
evioDOMNodeListP evioDOMNode::getChildren(void) throw(evioException) {
  evioDOMNodeList *l1 = getChildList();
  if(l1==NULL)return(evioDOMNodeListP(NULL));
  evioDOMNodeList *l2 = new evioDOMNodeList(l1->size());
  copy(l1->begin(), l1->end(), l2->begin());
  return(evioDOMNodeListP(l2));
}


//-----------------------------------------------------------------------------


/** 
 * Adds node to container node.
 * @param node Node to be added
 * @return Reference to node to be added
 */
evioDOMNode& evioDOMNode::operator<<(evioDOMNodeP node) throw(evioException) {
  addNode(node);
  return(*this);
}


//-----------------------------------------------------------------------------


/** 
 * True if node tag equals value.
 * @param tg Value to compare to
 * @return true if tags agree
 */
bool evioDOMNode::operator==(uint16_t tg) const {
  return(this->tag==tg);
}


//-----------------------------------------------------------------------------


/** 
 * True if node tag does not equal value
 * @param tg Value to compare to
 * @return true if tags disagree
 */
bool evioDOMNode::operator!=(uint16_t tg) const {
  return(this->tag!=tg);
}


//-----------------------------------------------------------------------------


/** 
 * True if node tag and num same as in tagNum pair.
 * @param tnPair tagNum pair to compare to
 * @return true if tag and num agree
 */
bool evioDOMNode::operator==(tagNum tnPair) const {
  return(
         (this->tag==tnPair.first) &&
         (this->num==tnPair.second)
         );
}


//-----------------------------------------------------------------------------


/** 
 * True if node tag and num NOT the same as in tagNum pair.
 * @param tnPair tagNum pair to compare to
 * @return true if tag and num disagree
 */
bool evioDOMNode::operator!=(tagNum tnPair) const {
  return(
         (this->tag!=tnPair.first) ||
         (this->num!=tnPair.second)
         );
}


//-----------------------------------------------------------------------------


/** 
 * Returns parent evioDOMNode.
 * @return Pointer to parent evioDOMNode
 */
const evioDOMNodeP evioDOMNode::getParent(void) const {
  return(parent);
}


//-----------------------------------------------------------------------------


/** 
 * Returns parent evioDOMTree.
 * @return Pointer to parent evioDOMTree
 */
const evioDOMTree *evioDOMNode::getParentTree(void) const {
  return(parentTree);
}


//-----------------------------------------------------------------------------


/** 
 * Returns content type.
 * @return Content type
 */
int evioDOMNode::getContentType(void) const {
  return(contentType);
}


//-----------------------------------------------------------------------------


/** 
 * True if node is a container node.
 * @return true if node is a container
 */
bool evioDOMNode::isContainer(void) const {
  return(::evIsContainer(contentType)==1);
}


//-----------------------------------------------------------------------------


/** 
 * True if node is a leaf node.
 * @return true if node is a leaf
 */
bool evioDOMNode::isLeaf(void) const {
  return(::evIsContainer(contentType)==0);
}


//--------------------------------------------------------------


/** 
 * Returns indent for pretty-printing, used internally
 * @param depth Depth level
 * @param size Size of indent per level
 * @return String containing proper number of indent spaces for this depth
 */
string evioDOMNode::getIndent(int depth, int size) {
  return(string(depth*size,' '));
}


//-----------------------------------------------------------------------------

                                   
/**
 * Returns XML string listing container node contents.
 * @return XML string listing contents
 */
string evioDOMNode::toString(void) const {
  ostringstream os;
  if(isLeaf()) {
    os << getHeader(0) << "   <!-- leaf node contains data of size " << getSize() << " -->" << endl << getFooter(0);
  } else {
    os << getHeader(0) << "   <!-- container node has " << getSize() << " children -->" << endl << getFooter(0);
  }
  return(os.str());
}



//-----------------------------------------------------------------------------
//----------------------- evioDOMContainerNode --------------------------------
//-----------------------------------------------------------------------------


/** 
 * Container node constructor used internally.
 * @param par Parent node
 * @param tg Node tag
 * @param num Node num
 * @param cType Container node content type
 */
evioDOMContainerNode::evioDOMContainerNode(evioDOMNodeP par, uint16_t tg, uint8_t num, ContainerType cType) throw(evioException)
  : evioDOMNode(par,tg,num,cType) {
}


//-----------------------------------------------------------------------------


/** 
 * Destructor recursively deletes children.
 */
evioDOMContainerNode::~evioDOMContainerNode(void) {
  evioDOMNodeList::iterator iter;
  for(iter=childList.begin(); iter!=childList.end(); iter++) {
    delete(*iter);
  }
}


//-----------------------------------------------------------------------------


/**
 * Returns XML string containing header needed for toString() and related methods
 * @param depth Current depth
 * @return XML string
 */
string evioDOMContainerNode::getHeader(int depth, const evioToStringConfig *config) const {
  ostringstream os;

  // get node name
  string name;
  if((config!=NULL)&&(config->toStringDictionary!=NULL)) {
    map<tagNum,string>::const_iterator iter = config->toStringDictionary->getNameMap.find(tagNum(tag,num));
    if(iter!=config->toStringDictionary->getNameMap.end()) {
      //tagNum t=(*iter).first;
      name=(*iter).second;
    }
  }
  if(name.size()<=0) name = evGetTypename(parent==NULL?BANK:parent->getContentType());


  os << ((config==NULL)?getIndent(depth):getIndent(depth,config->indentSize))
     <<  "<" << name << " content=\"" << evGetTypename(contentType)
     << "\" data_type=\"" << hex << showbase << getContentType() << noshowbase << dec
     << dec << "\" tag=\""  << tag;

  if((parent==NULL)||((parent->getContentType()==0xe)||(parent->getContentType()==0x10)))
    os << dec << "\" num=\"" << (int)num;

  if((config!=NULL)&&(config->verbose)) {
    os << dec << "\" nchildren=\"" << getSize();
  }

  os << "\">" << endl;
  return(os.str());
}


//-----------------------------------------------------------------------------

                                   
/**
 * Returns empty body for container node
 * @param depth Current depth
 * @return Empty string
 */
string evioDOMContainerNode::getBody(int depth, const evioToStringConfig *config) const {
  return("");
}


//-----------------------------------------------------------------------------

                                   
/**
 * Returns XML string containing footer needed by toString
 * @param depth Current depth
 * @return XML string
 */
string evioDOMContainerNode::getFooter(int depth, const evioToStringConfig *config) const {
  ostringstream os;

  // get node name
  string name;
  if((config!=NULL)&&(config->toStringDictionary!=NULL)) {
    map<tagNum,string>::const_iterator iter = config->toStringDictionary->getNameMap.find(tagNum(tag,num));
    if(iter!=config->toStringDictionary->getNameMap.end()) name=(*iter).second;
  }
  if(name.size()<=0) name = evGetTypename(parent==NULL?BANK:parent->getContentType());

  os << ((config==NULL)?getIndent(depth):getIndent(depth,config->indentSize)) << "</" << name << ">" << endl;
  return(os.str());
}


//-----------------------------------------------------------------------------


/**
 * Returns number of children of this container node
 * @return Number of children of this container node
 */
int evioDOMContainerNode::getSize(void) const {
  return(childList.size());
}



//-----------------------------------------------------------------------------
//----------------------- evioCompositeDOMLeafNode --------------------------------
//-----------------------------------------------------------------------------


/** 
 * Composite leaf node constructor used internally.
 * @param par Parent node
 * @param tg Node tag
 * @param num Node num
 */
evioCompositeDOMLeafNode::evioCompositeDOMLeafNode(evioDOMNodeP par, uint16_t tg, uint8_t num,
                                                   uint16_t formatTag, const string &formatString, 
                                                   uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &v) throw(evioException)
  : formatTag(formatTag), formatString(formatString), dataTag(dataTag), dataNum(dataNum), evioDOMLeafNode<uint32_t>(par,tg,num,v) {
  contentType=0xf;
}


//-----------------------------------------------------------------------------


/** 
 * Composite leaf node constructor used internally.
 * @param par Parent node
 * @param tg Node tag
 * @param num Node num
 */
evioCompositeDOMLeafNode::evioCompositeDOMLeafNode(evioDOMNodeP par, uint16_t tg, uint8_t num,
                                                   uint16_t formatTag, const string &formatString, 
                                                   uint16_t dataTag, uint8_t dataNum, const uint32_t *p, int ndata) throw(evioException)
  : formatTag(formatTag), formatString(formatString), dataTag(dataTag), dataNum(dataNum), evioDOMLeafNode<uint32_t>(par,tg,num,p,ndata) {
  contentType=0xf;
}


//-----------------------------------------------------------------------------


/*
 * Destructor
 */
evioCompositeDOMLeafNode::~evioCompositeDOMLeafNode(void) {
}


//-----------------------------------------------------------------------------


/**
 * Returns XML string containing body needed by toString
 * @param depth Current depth
 * @return XML string
 */
string evioCompositeDOMLeafNode::getBody(int depth, const evioToStringConfig *config) const {

  ostringstream os;
  string indent = ((config==NULL)?getIndent(depth):getIndent(depth,config->indentSize));
  string indent2 = indent + "       ";
  string indent3 = indent2 + "       ";
  string spaces = "     ";

  int wid=5,swid=10;


  // dump format string
  os << indent2 << "<formatString tag=\"" << formatTag << "\">" << endl << indent3 << formatString << endl << indent2 << "</formatString>" << endl;


  // dump data as uint32_t
  os << indent2 << "<data tag=\"" << dataTag << "\" num=\"" << (int)dataNum << "\"> " << endl;
  vector<uint32_t>::const_iterator iter;
  for(iter=data.begin(); iter!=data.end();) {

    os << indent3;
    for(int j=0; (j<wid)&&(iter!=data.end()); j++) {
      os.width(swid);
      os << hex << showbase << *iter << spaces;
      iter++;
    }
    os << endl;
  }
  os << indent2 << "</data>" << dec << noshowbase << endl;


  return(os.str());
}


//-----------------------------------------------------------------------------


/**
 * Returns numnber of data elements
 * @return number of data elements
 */
int evioCompositeDOMLeafNode::getSize(void) const {
  return(2 + ((formatString.size()+3)/4) + data.size());
}



//-----------------------------------------------------------------------------
//------------------------------- evioDOMTree ---------------------------------
//-----------------------------------------------------------------------------


/**
 * No-arg constructor creates empty tree name="evio", root node is bank with tag=0, num=0.
 */
evioDOMTree::evioDOMTree(void) throw(evioException)
  : root(NULL), name("evio"), dictionary(NULL) {
  root=evioDOMNode::createEvioDOMNode(0,0,BANK);
  root->parentTree=this;
}


//-----------------------------------------------------------------------------


/**
 * Constructor creates empty tree name="evio" with dictionary, root node is bank with tag=0, num=0.
 * @param dict dictionary
 */
evioDOMTree::evioDOMTree(evioDictionary *dict) throw(evioException)
  : root(NULL), name("evio"), dictionary(dict) {
  root=evioDOMNode::createEvioDOMNode(0,0,BANK);
  root->parentTree=this;
}


//-----------------------------------------------------------------------------


/**
 * Constructor fills tree from contents of evioChannel object, gets dictionary from channel.
 * @param channel evioChannel object
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(const evioChannel &channel, const string &name) throw(evioException) : root(NULL), name(name) {

  // check for random and no copy read
  const uint32_t *buf;

  buf = channel.getRandomBuffer();
  if(buf==NULL) buf = channel.getNoCopyBuffer();
  if(buf==NULL) buf = channel.getBuffer();
  if(buf==NULL)throw(evioException(0,"?evioDOMTree constructor...channel delivered null buffer",__FILE__,__FUNCTION__,__LINE__));

  root=parse(buf);
  root->parentTree=this;
  dictionary=channel.getDictionary();
}


//-----------------------------------------------------------------------------


/**
 * Constructor fills tree from contents of evioChannel object, gets dictionary from channel.
 * @param channel Pointer to evioChannel object
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(const evioChannel *channel, const string &name) throw(evioException)
  : root(NULL), name(name), dictionary(NULL) {

  if(channel==NULL)throw(evioException(0,"?evioDOMTree constructor...null channel",__FILE__,__FUNCTION__,__LINE__));

  // check for no copy read first
  const uint32_t *buf;
  buf = channel->getRandomBuffer();
  if(buf==NULL) buf = channel->getNoCopyBuffer();
  if(buf==NULL) buf = channel->getBuffer();
  if(buf==NULL)throw(evioException(0,"?evioDOMTree constructor...channel delivered null buffer",__FILE__,__FUNCTION__,__LINE__));

  root=parse(buf);
  root->parentTree=this;
  dictionary=channel->getDictionary();
}


//-----------------------------------------------------------------------------

/**
 * Constructor fills tree from contents of buffer
 * @param buf Buffer containing event
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(const uint32_t *buf, const string &name) throw(evioException)
  : root(NULL), name(name), dictionary(NULL) {
  if(buf==NULL)throw(evioException(0,"?evioDOMTree constructor...null buffer",__FILE__,__FUNCTION__,__LINE__));
  root=parse(buf);
  root->parentTree=this;
}


//-----------------------------------------------------------------------------


/**
 * Constructor creates tree using node as the root node.
 * @param node Pointer to node that becomes the root node
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(evioDOMNodeP node, const string &name) throw(evioException)
  : root(NULL), name(name), dictionary(NULL) {
  if(node==NULL)throw(evioException(0,"?evioDOMTree constructor...null evioDOMNode",__FILE__,__FUNCTION__,__LINE__));
  root=node;
  root->parentTree=this;
}


//-----------------------------------------------------------------------------


/**
 * Constructor creates new container node as root node.
 * @param tag Root node tag
 * @param num Root node num
 * @param cType Root node content type
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(uint16_t tag, uint8_t num, ContainerType cType, const string &name) throw(evioException)
  : root(NULL), name(name), dictionary(NULL) {
  root=evioDOMNode::createEvioDOMNode(tag,num,cType);
  root->parentTree=this;
}


//-----------------------------------------------------------------------------


/**
 * Constructor creates new container node as root node.
 * @param tag Root node name
 * @param cType Root node content type
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(const string& bankName, ContainerType cType, const string &name) throw(evioException)
  : root(NULL), name(name), dictionary(NULL) {
  
  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(bankName);
    root=evioDOMNode::createEvioDOMNode(tn.first,tn.second,cType);
    root->parentTree=this;
  } else {
    throw(evioException(0,"?evioDOMTree constructor...no dictionary to lookup bank name: " + bankName,__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------------


/**
 * Constructor creates new container node as root node.
 * @param name Root node tagNum
 * @param cType Root node content type
 * @param name Name of tree
 */
evioDOMTree::evioDOMTree(tagNum tn, ContainerType cType, const string &name) throw(evioException)
  : root(NULL), name(name), dictionary(NULL) {
  root=evioDOMNode::createEvioDOMNode(tn.first,tn.second,cType);
  root->parentTree=this;
}


//-----------------------------------------------------------------------------


/**
 * Destructor deletes root node and contents.
 */
evioDOMTree::~evioDOMTree(void) {
  root->cutAndDelete();
}


//-----------------------------------------------------------------------------


/**
 * Parses contents of buffer and creates node hierarchry.
 * @param buf Buffer containing event data
 * @return Pointer to highest node resulting from parsing of buffer
 */
evioDOMNodeP evioDOMTree::parse(const uint32_t *buf) throw(evioException) {
  evioStreamParser p;
  return((evioDOMNodeP)p.parse(buf,*this,NULL));
}


//-----------------------------------------------------------------------------


/**
 * Creates container node, used internally when parsing buffer.
 * @param bankLength Length of bank in 32-bit units
 * @param containerType Container type
 * @param contentType New node content type
 * @param tag New node tag
 * @param num New node num
 * @param depth Current depth
 * @param bankPointer Pointer to head of bank
 * @param payloadLength Length of contained payload in 32-bit units
 * @param payload Pointer to bank payload
 * @param userArg Used internally
 * @return void* used internally
 */
void *evioDOMTree::containerNodeHandler(int bankLength, int containerType, int contentType, uint16_t tag, uint8_t num, int depth, 
                                        const uint32_t *bankPointer, int payloadLength, const uint32_t *payload, void *userArg) {
  
    
  // get parent pointer
  evioDOMContainerNode *parent = (evioDOMContainerNode*)userArg;
    

  // create new node
  evioDOMNodeP newNode = evioDOMNode::createEvioDOMNode(tag,num,(ContainerType)contentType);


  // set parent pointers
  if(parent!=NULL) {
    parent->childList.push_back(newNode);
    newNode->parent=parent;
  }
  

  // return pointer to new node
  return((void*)newNode);
}
  
  
//-----------------------------------------------------------------------------


/**
 * Creates leaf node, used internally when parsing buffer.
 * @param bankLength Length of bank in 32-bit units
 * @param containerType Container type
 * @param contentType New node content type
 * @param tag New node tag
 * @param num New node num
 * @param depth Current depth
 * @param bankPointer Pointer to head of bank
 * @param dataLength Length of data in appropriate units
 * @param data Pointer to data in buffer
 * @param userArg Used internally
 * @return void* Pointer to new leaf node
 */
void *evioDOMTree::leafNodeHandler(int bankLength, int containerType, int contentType, uint16_t tag, uint8_t num, int depth, 
                                   const uint32_t *bankPointer, int dataLength, const void *data, void *userArg) {
  

  // create and fill new leaf
  evioDOMNodeP newLeaf;
  string s;


  switch (contentType) {

  case 0x0:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(uint32_t*)data,dataLength);
    newLeaf->contentType=0x0;
    break;

  case 0x1:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(uint32_t*)data,dataLength);
    break;
      
  case 0x2:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(float*)data,dataLength);
    break;
      
  case 0x3:
    {
      newLeaf = evioDOMNode::createEvioDOMNode<string>(tag,num);
      char *start = (char*)data;
      char *c = start;
      while((c[0]!=0x4)&&((c-start)<dataLength)) {
        s=string(c);
        newLeaf->append(s);
        c+=s.size()+1;
      }
      break;
    }

  case 0x4:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(int16_t*)data,dataLength);
    break;

  case 0x5:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(uint16_t*)data,dataLength);
    break;

  case 0x6:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(int8_t*)data,dataLength);
    break;

  case 0x7:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(uint8_t*)data,dataLength);
    break;

  case 0x8:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(double*)data,dataLength);
    break;

  case 0x9:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(int64_t*)data,dataLength);
    break;

  case 0xa:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(uint64_t*)data,dataLength);
    break;

  case 0xb:
    newLeaf = evioDOMNode::createEvioDOMNode(tag,num,(int32_t*)data,dataLength);
    break;

  case 0xf:
    {
      const uint32_t *d   = (const uint32_t*)data;
      int formatTag       = (d[0]>>20)&0xfff;
      int formatLen       = d[0]&0xffff;
      string formatString = string((const char *) &(((uint32_t*)data)[1]));
      int dataLen         = d[1+formatLen]-1;
      int dataTag         = (d[1+formatLen+1]>>16)&0xffff;
      int dataNum         = d[1+formatLen+1]&0xff;
      newLeaf = evioDOMNode::createEvioDOMNode(tag,num,formatTag,formatString,dataTag,dataNum,(uint32_t*)&d[3+formatLen],dataLen);
    }
    break;

  default:
    {
      ostringstream ss;
      ss << hex << showbase << contentType << noshowbase << dec;
      throw(evioException(0,"?evioDOMTree::leafNodeHandler...illegal content type: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
      break;
    }
  }


  // add new leaf to parent
  evioDOMContainerNode *parent = (evioDOMContainerNode*)userArg;
  if(parent!=NULL) {
    parent->childList.push_back(newLeaf);
    newLeaf->parent=parent;
  }


  // return pointer to newly created leaf node
  return((void*)newLeaf);
}


//-----------------------------------------------------------------------------


/** 
 * Removes and deletes tree root node and all its contents.
 */
void evioDOMTree::clear(void) throw(evioException) {
  if(root!=NULL) {
    root->cutAndDelete();
    root=NULL;
  }
}


//-----------------------------------------------------------------------------


/** 
 * Makes node root if tree is empty, or adds node to root if a container.
 * @param node Node to add to tree
 */
void evioDOMTree::addBank(evioDOMNodeP node) throw(evioException) {

  node->cut();

  if(root==NULL) {
    root=node;
    root->parentTree=this;

  } else {
    if(!root->isContainer())throw(evioException(0,"?evioDOMTree::addBank...root is not container",__FILE__,__FUNCTION__,__LINE__));
    evioDOMContainerNode *c = static_cast<evioDOMContainerNode*>(root);
    c->childList.push_back(node);
    node->parent=root;
  }
}


//-----------------------------------------------------------------------------


/**
 * Creates leaf node and adds it to tree root node.
 * @param tag Node tag
 * @param num Node num
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param dataVec vector<T> of data
 */
void evioDOMTree::addBank(uint16_t tag, uint8_t num, uint16_t formatTag, const string &formatString,
                          uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &dataVec) throw(evioException) {
  
  if(root==NULL) {
    root = evioDOMNode::createEvioDOMNode(tag,num,formatTag,formatString,dataTag,dataNum,dataVec);
    root->parentTree=this;

  } else {
    if(!root->isContainer())throw(evioException(0,"?evioDOMTree::addBank...root is not container",__FILE__,__FUNCTION__,__LINE__));
    evioDOMContainerNode* c = static_cast<evioDOMContainerNode*>(root);
    evioDOMNodeP node = evioDOMNode::createEvioDOMNode(tag,num,formatTag,formatString,dataTag,dataNum,dataVec);
    c->childList.push_back(node);
    node->parent=root;
  }

  return;
}


//-----------------------------------------------------------------------------


/**
 * Creates leaf node and adds it to tree root node.
 * @param name Leaf node name
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param dataVec vector<T> of data
 */
void evioDOMTree::addBank(const string &name, uint16_t formatTag, const string &formatString, 
                          uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &dataVec) throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    addBank(tn.first, tn.second, formatTag, formatString, dataTag, dataNum, dataVec);
  } else {
    throw(evioException(0,"?evioDOMTree::addBank...no dictionary",__FILE__,__FUNCTION__,__LINE__));
  }

  return;
}


//-----------------------------------------------------------------------------


/**
 * Creates leaf node and adds it to tree root node.
 * @param tn Leaf tagNum
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param dataVec vector<T> of data
 */
void evioDOMTree::addBank(tagNum tn, uint16_t formatTag, const string &formatString, 
                          uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &dataVec) throw(evioException) {
  addBank(tn.first, tn.second, formatTag, formatString, dataTag, dataNum, dataVec);
  return;
}


//-----------------------------------------------------------------------------


/**
 * Creates leaf node and adds it to tree root node.
 * @param tn Leaf tagNum
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param t array of uint32_t data
 * @parem len Length of array
 */
void evioDOMTree::addBank(tagNum tn, uint16_t formatTag, const string &formatString, 
                          uint16_t dataTag, uint8_t dataNum, const uint32_t *t, int len) throw(evioException) {

  addBank(tn.first, tn.second, formatTag, formatString, dataTag, dataNum, t, len);
  return;
}


//-----------------------------------------------------------------------------


/**
 * Creates leaf node and adds it to tree root node.
 * @param tag Node tag
 * @param num Node num
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param t array of uint32_t data
 * @parem len Length of array
 */
void evioDOMTree::addBank(uint16_t tag, uint8_t num, uint16_t formatTag, const string &formatString,
                          uint16_t dataTag, uint8_t dataNum, const uint32_t *t, int len) throw(evioException) {
  
  if(root==NULL) {
    root = evioDOMNode::createEvioDOMNode(tag,num,formatTag,formatString,dataTag,dataNum,t,len);
    root->parentTree=this;

  } else {
    if(!root->isContainer())throw(evioException(0,"?evioDOMTree::addBank...root not a container node",__FILE__,__FUNCTION__,__LINE__));
    evioDOMContainerNode* c = static_cast<evioDOMContainerNode*>(root);
    evioDOMNodeP node = evioDOMNode::createEvioDOMNode(tag,num,formatTag,formatString,dataTag,dataNum,t,len);
    c->childList.push_back(node);
    node->parent=root;
  }

  return;
}


//-----------------------------------------------------------------------------


/**
 * Creates leaf node and adds it to tree root node.
 * @param name Leaf node name
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param t array of uint32_t data
 * @parem len Length of array
 */
void evioDOMTree::addBank(const string &name, uint16_t formatTag, const string &formatString, 
                          uint16_t dataTag, uint8_t dataNum, const uint32_t *t, int len) throw(evioException) {

  if(dictionary!=NULL) {
    tagNum tn = dictionary->getTagNum(name);
    addBank(tn.first, tn.second, formatTag, formatString, dataTag, dataNum, t, len);
  } else {
    throw(evioException(0,"?evioDOMTree::addBank...no dictionary",__FILE__,__FUNCTION__,__LINE__));
  }

  return;
}


//-----------------------------------------------------------------------------


/** 
 * Creates new container node.
 * @param nName Node name
 * @param ContainerType Type of container node
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMTree::createNode(const string &nName, ContainerType cType) const throw(evioException) {
  return(evioDOMNode::createEvioDOMNode(nName,dictionary,cType));
}


//-----------------------------------------------------------------------------


/** 
 * Creates new container node.
 * @param nName Node name
 * @param ContainerType Type of container node
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMTree::createNode(const string &nName, const evioSerializable &o, ContainerType cType) const throw(evioException) {
  return(evioDOMNode::createEvioDOMNode(nName,dictionary,o,cType));
}


//-----------------------------------------------------------------------------


/** 
 * Creates new container node.
 * @param nName Node name
 * @param ContainerType Type of container node
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMTree::createNode(const string &nName, void (*f)(evioDOMNodeP c, void *userArg), void *userArg, ContainerType cType) const 
  throw(evioException) {
  return(evioDOMNode::createEvioDOMNode(nName,dictionary,f,userArg,cType));
}


//-----------------------------------------------------------------------------


/** 
 * Creates new composite leaf node.
 * @param nName Node name
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param dataVec Vector of uint32_t data
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMTree::createNode(const string &nName, uint16_t formatTag, const string &formatString, 
                        uint16_t dataTag, uint8_t dataNum, const vector<uint32_t> &dataVec) const throw(evioException) {
  return(evioDOMNode::createEvioDOMNode(nName,dictionary,formatTag,formatString,dataTag,dataNum,dataVec));
}


//-----------------------------------------------------------------------------


/** 
 * Creates new composite leaf node.
 * @param nName Node name
 * @param formatTag Format tag
 * @param formatString Format string
 * @param dataTag Data tag
 * @param dataNum Data num
 * @param dataVec Pointer to arry of uint32_t data
 * @param len Length of array
 * @return Pointer to new node
 */
evioDOMNodeP evioDOMTree::createNode(const string &nName, uint16_t formatTag, const string &formatString, 
                        uint16_t dataTag, uint8_t dataNum, const uint32_t *t, int len) const throw(evioException) {
  return(evioDOMNode::createEvioDOMNode(nName,dictionary,formatTag,formatString,dataTag,dataNum,t,len));
}


//-----------------------------------------------------------------------------


/** 
 * Makes node root if tree is empty, or adds node to root if a container.
 * @param node Node to add to root
 * @return Pointer to this
 */
evioDOMTree& evioDOMTree::operator<<(evioDOMNodeP node) throw(evioException) {
  addBank(node);
  return(*this);
}


//-----------------------------------------------------------------------------


/** 
 * Gets serialized length of tree.
 * @return Size of serialized tree in 4-byte words
 */
int evioDOMTree::getSerializedLength(void) const throw(evioException) {
  return(getSerializedLength(root));
}


//-----------------------------------------------------------------------------


/** 
 * Serializes tree to buffer.
 * @param buf Buffer that receives serialized tree
 * @param size Size of buffer
 * @return Size of serialized buffer in 4-byte words
 */
int evioDOMTree::toEVIOBuffer(uint32_t *buf, int size) const throw(evioException) {
  return(toEVIOBuffer(buf,root,size));
}


//-----------------------------------------------------------------------------


/** 
 * Gets serialized length of node, used internally.
 * Must include padding in byte count.
 * @param pNode Node to serialize
 * @return Size of serialized node in 4-byte words
 */
int evioDOMTree::getSerializedLength(const evioDOMNodeP pNode) const throw(evioException) {

  int bankLen,bankType,dataOffset;

  if(pNode->parent==NULL) {
    bankType=BANK;
  } else {
    bankType=pNode->parent->contentType;
  }


  // add bank header word(s)
  switch (bankType) {

  case 0xe:
  case 0x10:
    dataOffset=2;
    break;
  case 0xc:
  case 0xd:
  case 0x20:
  case 0x40:
    dataOffset=1;
    break;
  default:
    ostringstream ss;
    ss << hex << showbase << bankType << noshowbase << dec;
    throw(evioException(0,"evioDOMTree::getSerializedLength...illegal bank type: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
    break;
  }


  // set starting length
  bankLen=dataOffset;


  // loop over contained nodes if container node
  if(pNode->isContainer()) {
    const evioDOMContainerNode *c = static_cast<const evioDOMContainerNode*>(pNode);
    evioDOMNodeList::const_iterator iter;
    for(iter=c->childList.begin(); iter!=c->childList.end(); iter++) {
      bankLen+=getSerializedLength(*iter);
    }


  // leaf node...count data including padding
  } else {

    int nword,ndata;
    switch (pNode->contentType) {

    case 0x0:
    case 0x1:
    case 0x2:
    case 0xb:
      {
        const evioDOMLeafNode<uint32_t> *leaf = static_cast<const evioDOMLeafNode<uint32_t>*>(pNode);
        ndata = leaf->data.size();
        nword = ndata;
      }
      break;
      
    case 0x3:
      {
        const evioDOMLeafNode<string> *leaf = static_cast<const evioDOMLeafNode<string>*>(pNode);
        int nstring = leaf->data.size();
        ndata=0;
        for(unsigned int i=0; (int)i<nstring; i++) ndata+=leaf->data[i].size();
        ndata+=nstring;
        nword = (ndata+3)/4;
      }
      break;

    case 0x4:
    case 0x5:
      {
        const evioDOMLeafNode<uint16_t> *leaf = static_cast<const evioDOMLeafNode<uint16_t>*>(pNode);
        ndata = leaf->data.size();
        nword = (ndata+1)/2;
      }
      break;

    case 0x6:
    case 0x7:
      {
        const evioDOMLeafNode<uint8_t> *leaf = static_cast<const evioDOMLeafNode<uint8_t>*>(pNode);
        ndata = leaf->data.size();
        nword = (ndata+3)/4;
      }
      break;

    case 0x8:
    case 0x9:
    case 0xa:
      {
        const evioDOMLeafNode<uint64_t> *leaf = static_cast<const evioDOMLeafNode<uint64_t>*>(pNode);
        ndata = leaf->data.size();
        nword = ndata*2;
      }
      break;

    default:
      ostringstream ss;
      ss << pNode->contentType;
      throw(evioException(0,"?evioDOMTree::getSerializedLength...illegal leaf type: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
      break;
    }

    // increment data count
    bankLen+=nword;
  }


  // return length of this bank
  return(bankLen);
}


//-----------------------------------------------------------------------------


/** 
 * Serializes node into buffer, used internally.
 * @param buf Buffer that receives serialized tree
 * @param pNode Node to serialize
 * @param size Size of buffer in 4-byte words
 * @return Size of serialized node in 4-byte words
 */
int evioDOMTree::toEVIOBuffer(uint32_t *buf, const evioDOMNodeP pNode, int size) const throw(evioException) {

  int bankLen,bankType,dataOffset,padding=0;

  if(size<=0)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...illegal buffer size",__FILE__,__FUNCTION__,__LINE__));


  if(pNode->parent==NULL) {
    bankType=BANK;
  } else {
    bankType=pNode->parent->contentType;
  }


  // add bank header word(s)
  switch (bankType) {


  case 0xe:
  case 0x10:
    buf[0]=0;
    buf[1] = (pNode->tag<<16) | (pNode->contentType<<8) | pNode->num;
    dataOffset=2;
    break;
  case 0xd:
  case 0x20:
    if(pNode->num!=0)cerr << "?warning...num ignored in segment: " << pNode->num << endl;
    buf[0] = (pNode->tag<<24) | ( pNode->contentType<<16);
    dataOffset=1;
    break;
  case 0xc:
  case 0x40:
    if(pNode->tag>0xfff)cerr << "?warning...tag truncated to 12 bits in tagsegment: " << pNode->tag << endl;
    if(pNode->num!=0)cerr << "?warning...num ignored in tagsegment: " << pNode->num<< endl;
    buf[0] = (pNode->tag<<20) | ( pNode->contentType<<16);
    dataOffset=1;
    break;
  default:
    ostringstream ss;
    ss << hex << showbase << bankType << noshowbase << dec;
    throw(evioException(0,"evioDOMTree::toEVIOBuffer...illegal bank type in boilerplate: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
    break;
  }


  // set starting length
  bankLen=dataOffset;


  // loop over contained nodes if container node
  if(pNode->isContainer()) {
    const evioDOMContainerNode *c = static_cast<const evioDOMContainerNode*>(pNode);
    evioDOMNodeList::const_iterator iter;
    for(iter=c->childList.begin(); iter!=c->childList.end(); iter++) {
      bankLen+=toEVIOBuffer(&buf[bankLen],*iter,size-bankLen-1);
      if(bankLen>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));
    }
    

  // leaf node...copy data, and don't forget to pad!
  } else {

    int nword,ndata,i;
    switch (pNode->contentType) {

    case 0x0:
    case 0x1:
    case 0x2:
    case 0xb:
      {
        const evioDOMLeafNode<uint32_t> *leaf = static_cast<const evioDOMLeafNode<uint32_t>*>(pNode);
        ndata = leaf->data.size();
        nword = ndata;
        if(bankLen+nword>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));
        for(i=0; i<ndata; i++) buf[dataOffset+i]=(uint32_t)(leaf->data[i]);
      }
      break;
      
    case 0x3:
      {
        const evioDOMLeafNode<string> *leaf = static_cast<const evioDOMLeafNode<string>*>(pNode);
        int nstring =  leaf->data.size();
        int nbytes,nbytesTotal=0;
        uint8_t *c = (uint8_t*)&buf[dataOffset];
        for(unsigned int j=0; (int)j<nstring; j++) {
          string s = leaf->data[j];
          nbytes = s.size();
          nbytesTotal += nbytes + 1;
          if(bankLen+((nbytesTotal+3)/4)>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));
          for(i=0; i<nbytes; i++) c[i]=(s.c_str())[i]; 
          c[nbytes]='\0';
          c+=nbytes+1;
        }
        for(i=0; i<(4-nbytesTotal%4)%4; i++) c[i]=0x04;   // pad with EOT if needed
        nword = (nbytesTotal+3)/4;
      }
      break;

    case 0x4:
    case 0x5:
      {
        const evioDOMLeafNode<uint16_t> *leaf = static_cast<const evioDOMLeafNode<uint16_t>*>(pNode);
        ndata = leaf->data.size();
        nword = (ndata+1)/2;
        if(bankLen+nword>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));
        uint16_t *s = (uint16_t *)&buf[dataOffset];
        for(i=0; i<ndata; i++) s[i]=static_cast<uint16_t>(leaf->data[i]);
        if((ndata%2)!=0)s[ndata]=0,padding=2;
      }
      break;

    case 0x6:
    case 0x7:
      {
        const evioDOMLeafNode<uint8_t> *leaf = static_cast<const evioDOMLeafNode<uint8_t>*>(pNode);
        ndata = leaf->data.size();
        nword = (ndata+3)/4;
        if(bankLen+nword>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));
        uint8_t *c = (uint8_t*)&buf[dataOffset];
        for(i=0; i<ndata; i++) c[i]=static_cast<uint8_t>(leaf->data[i]);
        for(i=ndata; i<ndata+(4-ndata%4)%4; i++) c[i]=0;
        padding=(4-ndata%4)%4;
      }
      break;

    case 0x8:
    case 0x9:
    case 0xa:
      {
        const evioDOMLeafNode<uint64_t> *leaf = static_cast<const evioDOMLeafNode<uint64_t>*>(pNode);
        ndata = leaf->data.size();
        nword = ndata*2;
        if(bankLen+nword>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));
        uint64_t *ll = (uint64_t*)&buf[dataOffset];
        for(i=0; i<ndata; i++) ll[i]=static_cast<uint64_t>(leaf->data[i]);
      }
      break;

    case 0xf:
      {
        const evioCompositeDOMLeafNode *leaf = static_cast<const evioCompositeDOMLeafNode*>(pNode);
        int nfmtchar  = leaf->formatString.size()+1;
        uint16_t nfmtword  = (nfmtchar+3)/4;
        ndata = leaf->data.size();
        nword = 3 + nfmtword + ndata;
        if(bankLen+nword>size)throw(evioException(0,"?evioDOMTree::toEVOIBuffer...buffer too small",__FILE__,__FUNCTION__,__LINE__));

        // format string stored in tagsegment
        if(leaf->formatTag>0xfff)cerr << "?warning...formatTag truncated to 12 bits in composite leaf node tagsegment: " 
                                      << leaf->formatTag << endl;
        buf[dataOffset] = (leaf->formatTag<<20) | (0x3<<16) | nfmtword;
        uint8_t *c = (uint8_t*)&buf[dataOffset+1];
        for(i=0; i<nfmtchar; i++) c[i]=(leaf->formatString.c_str())[i]; 
        c[nfmtchar]='\0';
        for(i=0; i<(4-nfmtchar%4)%4; i++) c[nfmtchar+i]=0x04;   // pad with EOT if needed
        
        // data stored as uint32_t in bank
        buf[dataOffset+nfmtword+1] = ndata+1;
        buf[dataOffset+nfmtword+2] = (leaf->dataTag<<16) | (0x1<<8) | leaf->dataNum;
        uint32_t *d = &buf[dataOffset+3+nfmtword];
        for(i=0; i<ndata; i++) d[i]=leaf->data[i];
      }
      break;


    default:
      ostringstream ss;
      ss << pNode->contentType;
      throw(evioException(0,"?evioDOMTree::toEVOIBuffer...illegal leaf type: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
      break;
    }

    // increment data count
    bankLen+=nword;
  }


  // finaly set node length and padding size in buffer
  switch (bankType) {

  case 0xe:
  case 0x10:
    buf[0]=bankLen-1;
    buf[1]|=(padding<<14);
    break;
  case 0xd:
  case 0x20:
    buf[0]|=(padding<<22);
  case 0xc:
  case 0x40:
    if((bankLen-1)>0xffff)throw(evioException(0,"?evioDOMTree::toEVIOBuffer...length too big for segment type",__FILE__,__FUNCTION__,__LINE__));
    buf[0]|=(bankLen-1);
    break;
  default: 
    ostringstream ss;
    ss << bankType;
    throw(evioException(0,"?evioDOMTree::toEVIOBuffer...illegal bank type setting length: " + ss.str(),__FILE__,__FUNCTION__,__LINE__));
    break;
  }


  // return length of this bank
  return(bankLen);
}


//-----------------------------------------------------------------------------


/**
 * Returns list of all nodes in tree.
 * @return Pointer to list of nodes in tree (actually auto_ptr<>)
 */
evioDOMNodeListP evioDOMTree::getNodeList(void) throw(evioException) {
  return(evioDOMNodeListP(addToNodeList(root,new evioDOMNodeList,isTrue)));
}


//-----------------------------------------------------------------------------


/**
 * Returns list of all nodes in tree with particular name, tagNum from dictionary
 * @param name Name of banks to find
 * @return Pointer to list of nodes in tree (actually auto_ptr<>)
 */
evioDOMNodeListP evioDOMTree::getNodeList(const string &nName) throw(evioException) {

  if(dictionary!=NULL) {
    map<string,tagNum>::const_iterator iter = dictionary->getTagNumMap.find(nName);
    if(iter!=dictionary->getTagNumMap.end())
      return(evioDOMNodeListP(addToNodeList(root,new evioDOMNodeList,tagNumEquals((*iter).second))));
  }

  // return empty list if no dictionary or name not found
  return(*(new evioDOMNodeListP));
}


//-----------------------------------------------------------------------------


/**
 * Returns XML string listing tree contents.
 * @return XML string listing contents
 */
string evioDOMTree::toString(void) const {

  if(root==NULL)return("<!-- empty tree -->");

  ostringstream os;
  if(dictionary==NULL) {
    toOstream(os,root,0,&defaultToStringConfig);
  } else {
    evioToStringConfig config(dictionary);
    toOstream(os,root,0,&config);
  }
  os << endl << endl;
  return(os.str());

}


//-----------------------------------------------------------------------------


/**
 * Returns XML string listing tree contents.
 * @param config Pointer to evioToStringConfig contains options that control string creation
 * @return XML string listing contents
 */
string evioDOMTree::toString(const evioToStringConfig *config) const {

  if(root==NULL)return("<!-- empty tree -->");


  // use tree dictionary if none in config, otherwise config dictionary has priority
  ostringstream os;
  if((config->toStringDictionary==NULL)&&(dictionary!=NULL)) {
    evioToStringConfig c(*config);
    c.setDictionary(dictionary);
    toOstream(os,root,0,&c);
    os << endl << endl;
  } else {
    toOstream(os,root,0,config);
    os << endl << endl;
  }
  
  return(os.str());

}


//-----------------------------------------------------------------------------


/**
 * @param config Pointer to config
 * Returns XML string listing tree contents.
 * @return XML string listing contents
 */
string evioDOMTree::toString(const evioToStringConfig &config) const {
  return(toString(&config));
}


//-----------------------------------------------------------------------------


/**
 * Used internally by toString method.
 * @param os ostream to append XML fragments
 * @param pNode Node to get XML representation
 * @param depth Current depth
 * @param config Pointer to toStringConfig
 */
void evioDOMTree::toOstream(ostream &os, const evioDOMNodeP pNode, int depth, const evioToStringConfig *config) const
  throw(evioException) {

  if(pNode==NULL)return;


  // check if bank name or tag is in one of toStringConfig include/exclude vectors
  if((config!=NULL)&&config->skipNode(pNode))return;


  // get node header
  os << pNode->getHeader(depth,config);


  // dump data if leaf node, dump contained banks if container
  if(pNode->isLeaf()) {
    if((config!=NULL)&&(config->noData)) {
      os << pNode->getIndent(depth,config->indentSize) 
         << "   <!-- leaf node contains vector of size "<< pNode->getSize() << " -->" << endl;
    } else {
      os << pNode->getBody(depth,config);
    }

  } else {

    if((config!=NULL)&&(config->maxDepth>0)&&((depth+1)>=config->maxDepth)) {
      os << pNode->getIndent(depth,config->indentSize) << "   <!-- container node has "<< pNode->getSize() << " children -->" << endl;
    } else {
      if(pNode->isContainer()) {
        const evioDOMContainerNode *c = static_cast<const evioDOMContainerNode*>(pNode);
        evioDOMNodeList::const_iterator iter;
        for(iter=c->childList.begin(); iter!=c->childList.end(); iter++) toOstream(os,*iter,depth+1,config);
      }
    }
  }
  

  // get footer
  os << pNode->getFooter(depth,config);
}


//-----------------------------------------------------------------------------


/**
 * Sets dictionary to use by this tree.
 * @return Const pointer to dictionary
 */
const evioDictionary *evioDOMTree::getDictionary(void) const {
  return(dictionary);
}


//-----------------------------------------------------------------------------


/**
 * Sets dictionary to use by this tree.
 * @param dict Pointer to dictionary
 */
void evioDOMTree::setDictionary(const evioDictionary *dict) {
  dictionary=dict;
}


//-----------------------------------------------------------------------------


/**
 * Sets dictionary to use by this tree.
 * @param dict Ref to dictionary
 */
void evioDOMTree::setDictionary(const evioDictionary &dict) {
  dictionary=&dict;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
