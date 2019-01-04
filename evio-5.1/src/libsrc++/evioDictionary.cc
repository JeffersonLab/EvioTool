/*
 *  evioDictionary.cc
 *
 *   Author:  Elliott Wolin, JLab, 13-Apr-2012
*/


#include "evioDictionary.hxx"
#include <fstream>


using namespace std;
using namespace evio;


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------


/**
 * No-arg constructor contains empty maps.
 */
evioDictionary::evioDictionary() {
}


//-----------------------------------------------------------------------


/**
 * Constructor fills dictionary maps from string.
 * @param dictionaryXML XML string parsed to create dictionary maps
 */
evioDictionary::evioDictionary(const string &dictXML, const string &sep) 
  : dictionaryXML(dictXML), separator(sep), parentIsLeaf(false) {
  parseDictionary(dictionaryXML);
}


//-----------------------------------------------------------------------

/**
 * Constructor fills dictionary maps from ifstream.
 * @param dictionaryXML XML string parsed to create dictionary maps
 */
evioDictionary::evioDictionary(ifstream &dictIFS, const string &sep) : separator(sep), parentIsLeaf(false) {

    if(dictIFS.is_open()) {
      string s;
      while((dictIFS.good())&&(!dictIFS.eof())) {
        getline(dictIFS,s);
        if(s.size()>0)dictionaryXML += s + "\n";
      }
      dictIFS.close();
    } else {
      throw(evioException(0,"?evioDictionary::evioDictionary...unable to read from ifstream",__FILE__,__FUNCTION__,__LINE__));
    }
    
    // now parse XML
    parseDictionary(dictionaryXML);
}


//-----------------------------------------------------------------------

/**
 * Destructor.
 */
evioDictionary::~evioDictionary() {
}


//-----------------------------------------------------------------------


/**
 * Gets dictionary XML
 * @return dictionary XML
 */
string evioDictionary::getDictionaryXML(void) const {
  return(dictionaryXML);
}


//-----------------------------------------------------------------------


/**
 * Uses Expat to parse XML dictionary string and fill maps.
 * @param dictionaryXML XML string
 * @return True if parsing succeeded
 */
bool evioDictionary::parseDictionary(const string &dictionaryXML) {

  // init string parser and start element handler
  XML_Parser xmlParser = XML_ParserCreate(NULL);
  XML_SetElementHandler(xmlParser,startElementHandler,endElementHandler);
  XML_SetUserData(xmlParser,reinterpret_cast<void*>(this));
      

  // parse XML dictionary
  bool ok = XML_Parse(xmlParser,dictionaryXML.c_str(),(int)dictionaryXML.size(),true)!=0;
  if(!ok) {
    cerr << endl << "  ?evioDictionary::parseDictionary...parse error"
         << endl << endl << XML_ErrorString(XML_GetErrorCode(xmlParser));
  }
  XML_ParserFree(xmlParser);

  return(ok);
}


//-----------------------------------------------------------------------


/**
 * Expat start element handler, must be static.
 * @param userData void* pointer to evioDictionary instance
 * @param xmlname Name of current element
 * @param atts Array of attributes for this element
 */
void evioDictionary::startElementHandler(void *userData, const char *xmlname, const char **atts) {
  

  // userData points to dictionary 
  if(userData==NULL) {
    cerr << "?evioDictionary::startElementHandler...NULL userData" << endl;
    return;
  }


  // get dictionary from user data
  evioDictionary *d = reinterpret_cast<evioDictionary*>(userData);


  // only process dictionary entries
  string xmlnameLC = xmlname;
  std::transform(xmlnameLC.begin(), xmlnameLC.end(), xmlnameLC.begin(), (int(*)(int)) tolower);  // magic
  if((xmlnameLC!="bank")&&(xmlnameLC!="leaf")&&(xmlnameLC!=dictEntryTag))return;
  if(d->parentIsLeaf) {
    throw(evioException(0,"?evioDictionary::startElementHandler...parent bank is leaf!",__FILE__,__FUNCTION__,__LINE__));
  }
  

  // only look at name,tag,num attributes
  string name = "";
  int tag = 0;
  int num = 0;
  for (int i=0; atts[i]; i+=2) {
    if(strcasecmp(atts[i],"name")==0) {
      name = string(atts[i+1]);
    } else if(strcasecmp(atts[i],"tag")==0) {
      tag = atoi(atts[i+1]);
    } else if(strcasecmp(atts[i],"num")==0) {
      num = atoi(atts[i+1]);
    }
  }


  // create full name using parent prefix for hierarchical dictionary tags
  string fullName = name;
  if(xmlnameLC!=dictEntryTag) {
    if(d->parentPrefix.empty()) {
      d->parentPrefix = name;
    } else {
      fullName = d->parentPrefix + d->separator + name;
      d->parentPrefix += d->separator + name;
    }
    d->parentIsLeaf = xmlnameLC=="leaf";
  }
  

  // add tag/num pair and full (hierarchical) name to both maps, first check for duplicates
  tagNum tn = tagNum((uint16_t)tag, (uint8_t)num);
  if((d->getNameMap.find(tn)==d->getNameMap.end())&&(d->getTagNumMap.find(fullName)==d->getTagNumMap.end())) {
    d->getNameMap[tn]         = fullName;
    d->getTagNumMap[fullName] = tn;
  } else {
    throw(evioException(0,"?evioDictionary::startElementHandler...duplicate entry in dictionary!",__FILE__,__FUNCTION__,__LINE__));
  }
}
    

//-----------------------------------------------------------------------


/**
 * Expat end element handler, must be static.
 * @param userData void* pointer to evioDictionary instance
 * @param xmlname Name of current element
 */
void evioDictionary::endElementHandler(void *userData, const char *xmlname) {
  if((strcasecmp(xmlname,"bank")==0)||(strcasecmp(xmlname,"leaf")==0)) {
    evioDictionary *d = reinterpret_cast<evioDictionary*>(userData);
    d->parentIsLeaf = false;
    int spos = d->parentPrefix.rfind(d->separator);
    if(spos==d->parentPrefix.npos) {
      d->parentPrefix.clear();
    } else {
      d->parentPrefix.erase(spos);
    }
  }
}
    

//-----------------------------------------------------------------------


/**
 * Gets tagNum given name, throws exception if not found.
 * @param name Name of bank
 * @return tagNum
 */
tagNum evioDictionary::getTagNum(const string &name) const throw(evioException) {
  map<string,tagNum>::const_iterator iter = getTagNumMap.find(name);
  if(iter!=getTagNumMap.end()) {
    return((*iter).second);
  } else {
    throw(evioException(0,"?evioDictionary::getTagNum...no entry named "+name,__FILE__,__FUNCTION__,__LINE__));
  }
}



//-----------------------------------------------------------------------


/**
 * Gets name given tagNum, throws exception if not found.
 * @param tn tagNum of bank
 * @return name
 */
string evioDictionary::getName(tagNum tn) const throw(evioException) {
  map<tagNum,string>::const_iterator iter = getNameMap.find(tn);
  if(iter!=getNameMap.end()) {
    return((*iter).second);
  } else {
    ostringstream ss;
    ss << "?evioDictionary::getName...no entry with tagNum "<<  tn.first << "," << tn.second << ends;
    throw(evioException(0,ss.str(),__FILE__,__FUNCTION__,__LINE__));
  }
}


//-----------------------------------------------------------------------


/**
 * Sets separator character.
 * @param sep Separator character
 */
void evioDictionary::setSeparator(const string &sep) {
  separator=sep;
}


//-----------------------------------------------------------------------


/**
 * Gets separator character.
 * @return Separator character
 */
string evioDictionary::getSeparator(void) const {
  return(separator);
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------


/**
 * Converts dictionary into string.
 * @return String containing dictionary keys and values
 */
string evioDictionary::toString(void) const throw(evioException) {

  stringstream ss;

  map<string,tagNum>::const_iterator pos;
  ss << "key                                    " << "value" << endl;
  ss << "---                                    " << "-----" << endl;
  for (pos = getTagNumMap.begin(); pos != getTagNumMap.end(); pos++) {
    ss << left << setw(35) << pos->first << "    " << (int)pos->second.first << "," << (int)pos->second.second << endl;
  }

  return(ss.str());
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

