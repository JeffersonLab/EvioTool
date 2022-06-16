//
//  EvioTool.h
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/17/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

// It is entirely possible for this class to NOT inherit from TObject.
// You can still fill and do much of what you want, and even access the classes from
// the ROOT prompt. But since the whole point of these classes is to make EVIO data
// accessible in ROOT, we may as well derive from TObjec and get some of the
// usefull ROOT features, like being able to write these classes to a file.
// Of course, the cost is a litte overhead, but frankly, that is not much to worry
// about here.

#ifndef __EvioTool__
#define __EvioTool__

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
using namespace std;

// %%%%%%%%%%%%
// EVIO headers
// %%%%%%%%%%%%
#ifndef __CINT__
#include "evio.h"
#include "et.h"
#endif

#include <iostream>
#include "TROOT.h"
#include "TObject.h"
#include "TNamed.h"

#include "Leaf.h"
#include "FADCdata.h"
#include "Bank.h"

#ifndef ET_DEFAULT_PORT
#define ET_DEFAULT_PORT 11111
#endif
#ifndef ET_DEFAULT_NAME
#define ET_DEFAULT_NAME "/et/clasprod"
#endif
#ifndef ET_DEFAULT_HOST
#define ET_DEFAULT_HOST "clondaq6"
#endif
#ifndef ET_DEFAULT_STATION
#define ET_DEFAULT_STATION "EvioTool"
#endif

#define TOP_BANK_TYPE 0x10   // Standard container type.

class EvioTool: public Bank {

public:
   enum DebugType_t {
      EvioTool_Debug_Info  = 0x01,
      EvioTool_Debug_Info2 = 0x02,   // Write the bank structure
      EvioTool_Debug_L1    = 0x04,
      EvioTool_Debug_L2    = 0x08
   };

   enum StatusCodeType_t {
      EvioTool_Status_OK  =  0,
      EvioTool_Status_EOF =  1,
      EvioTool_Status_ERROR =  2,
      EvioTool_Status_No_Data = 3
   };
   // None of these control variables need to be written to root files, so all are //!
   //
   // Setting fChop_level = 1 will mean the top most bank, the "event" bank
   // is not copied but instead treated as if it wasn't there.
   // Setting fMax_level = fChop_level+1 means you get only one deep banks.
   // With fChop_level=0, fMax_level at 2 and fAutoAdd=true, you can explore the top level banks
   // of an EVIO file.
   //
   int fDebug = 0;          //!-  Bit wise!!! See DebugType_t
   bool fAutoAdd = false;   //!-  If true, add any bank encountered. If false add only banks already known, i.e. from the dictionary.
   int fChop_level = 1;     //!-  Chop, or collapse, the top N bank levels. Usually you want to collapse the top level (event structure) bank.
   int fMax_level = 9999;      //!-  Prune, or collapse, the bottom (deepest) N bank levels.
   // Setting fChop_level = 1 will mean the top most bank, usually a container bank tag=1 num=0,
   // is not copied but instead treated as if it wasn't there.
   // Setting fMax_level = fChop_level means you get only one deep banks. All banks of
   // higher level will have their contends inserted in the parent bank instead.

   bool fFullErase = false; //!- Useful with fAutoAdd=true and no dictionary, erase fully at new event, so bank structure is gone completely. Otherwise the bank structure is kept.

private:
public:
   string fETStationName = "EvioTool";
   int    fETPort = ET_DEFAULT_PORT;
   string fETHost = ET_DEFAULT_HOST;
   string fETName = ET_DEFAULT_NAME;
   int fETPos=1;
   int fETPpos=1;
   bool fETNoBlock=true;

   bool fIsOpen = false;    //!- True when a file or ET connection has been opened.
   bool fReadFromEt = false;           //!- True of reading from ET.
   unsigned int fEtReadChunkSize = 10; //!- Number of chunks (events) to grab at once from ET.
   int fNumRead = 1;                  //!- Number of chunks actually read from ET
   int fCurrentChunk= -1;               //!- Current chunk being processed.
   int fETWaitMode = ET_SLEEP;         //! - How to wait for an event.
   //  ET_SLEEP, ET_ASYNC, or ET_TIMED. The sleep option waits until an event is available before it returns
   //  and therefore may "hang". The timed option returns after a time set by the last argument.
   //  Finally, the async option returns immediately whether or not it was successful in obtaining a new event for the caller.
   //  For remote users, the mentioned macros may be ORed with ET_MODIFY. This indicates to the ET server that the user
   //  intends to modify the data and so the server must NOT place the event immediately back into the ET system,
   //  but must do so only when et_events_put is called.
   bool fEventValid = false;            //!- Set to true in Next() when the event buffer becomes valid, and false in EndEvent();
   et_event    **fPEventBuffer = nullptr;   //!- ET event header buffer
   et_sys_id   fEventId;
   et_att_id   fEtAttach; //!- Attach handle to ET

   int fEvioHandle = 0;    //!- Handle to the evio system
   const unsigned int *fEvioBuf = nullptr; //!- Buf ptr to read event into.
   unsigned int fEvioBufLen     = 0;     //!- length of buffer.

   bool fMultiThread=false;      //!- Set to true if you are multi-threading Next().
   // Notes:
   // For multi-threading, you either need to have the Next() be in the master, or if Next is in multiple
   // threads reading from the same source, we need to orchestrate the reads so another read does not overwrite the
   // memory of the current one.
   // So for

public:
   EvioTool(string infile="");
   virtual ~EvioTool(){
      Close();             // Make sure the file or ET connection are closed properly.
      if(fPEventBuffer) free(fPEventBuffer); // Clean up memory explicitly allocated by ET.
   };
   int Open(const char *filename,const char *dictf=NULL);
   int OpenEt(string station_name, string et_name, string host,
              unsigned short port, int pos = 1, int ppos = 1, bool no_block = true);
   int ReOpenEt();
   int GetWaitMode(){ return fETWaitMode; }
   void SetWaitMode(int mode){ fETWaitMode = mode;}
   bool IsOpen(){ return fIsOpen;}
   bool IsReadingFromEt(){return fReadFromEt;}
   bool IsValid(){return fEventValid;}
   bool IsETAlive();
   void Close();

   void SetETHost(string host) { fETHost = host; }
   string GetETHost() const { return fETHost;}
   void SetETPort(int port){ fETPort = port;}
   int GetETPort() const { return fETPort; }
   void SetETName(string name){ fETName = name;}
   string GetETName() const {return fETName;}

   void SetETStation(string station){ fETStationName = station;}
   string GetETStation(){ return fETStationName;}

   const uint32_t *GetBufPtr(void){  // Useful if you want to write the evio buffer.
      return(fEvioBuf);
   }
   void parseDictionary(const char *dictf);
   virtual int NextNoParse(void);
   virtual int Next(void);
   virtual int EndEvent(void);
   virtual int ParseEvioBuff(const unsigned int *buf);                                  // Top level buffer parser.
   int ParseBank(const unsigned int *buf, int bankType, int depth, Bank *node); // Recursive buffer parser.
   int LeafNodeHandler(const unsigned int *buf,int len, int padding, int contentType,unsigned short tag,unsigned char num,Bank *node);
   Bank *ContainerNodeHandler(const unsigned int *buf, int len, int padding, int contentType,unsigned short tag, unsigned char num, Bank *node, int depth);
   int AddOrFillLeaf_FADCdata(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node); // Non template special case for FADC.
   template<typename T> int AddOrFillLeaf(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node){
      // Add or Fill an int leaf in the bank node.
      // If fAutoAdd is false, find the leaf with tag, num and fill it. If not found do nothing.
      // If fAutoAdd is true, if not found, a new leaf is added and filled.
      int loc = node->FindLeaf(tag,num);
      if( loc == -1){
         if(fAutoAdd){
            char str[100];
            sprintf(str,"Int-%ud-%ud",tag,num);
            if(fDebug & EvioTool_Debug_L2) cout << "Adding a new Leaf node to node: " << node->GetNum() << " with name: " << str << endl;
            node->AddLeaf<T>(str,tag,num,"Auto added int leaf");
            loc= node->leafs->GetEntriesFast()-1;
         }else{
            return 0;
         }
      }

      if(fDebug & EvioTool_Debug_L2) cout << "Adding data to Leaf at idx = " << loc << " templated for type <" << ((Leaf<T> *)node->leafs->At(loc))->Type() << "> \n";
      node->PushDataArray(loc, (T *)buf, len);

      ((Leaf<T> *)node->leafs->At(loc))->CallBack();
      return 1;
   };


   void SetAutoAdd(bool stat=true){
      // Automatically add new data structures found in the EVIO file.
      // This really slows things down, but it does allow you to explore the EVIO data.
      fAutoAdd=stat;
   }

   void SetFullErase(bool stat=false){
      // If FullErase is set to true, then with AutoAdd on, each new event will first have the
      // event structure erased. If false, the event structure of the previous events remains.
      fFullErase=stat;
   }

   void SetChopLevel(int level=1){
      // Set ChopLevel=0 if you want your 0 level bank to be the *event* bank. Usually you would want =1, so that
      // EvioTool contains the *content* of the event, instead of a bank that is the event.
      // Setting higher than 1 collapses the next level(s) into level 0.
      fChop_level=level;
   }

   void SetDebug(int bits=0x0F){ fDebug = bits; }
//  int AddBank(Bank &b);

   void Test(void);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
ClassDef(EvioTool,1);
#pragma clang diagnostic pop

};

#endif /* defined(__EvioTool__) */
