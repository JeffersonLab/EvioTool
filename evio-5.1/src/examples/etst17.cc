//  example program reads and dumps EVIO buffers

//  ejw, 20-sep-2011




#include "evioUtil.hxx"
#include "evioDictionary.hxx"
#include "evioFileChannel.hxx"


using namespace evio;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  
  // note:  getTagNumMap is map<string,tagnum>

  try {

    
    string dictXML1 = 
      "<dict>\n"
      "<bank name=\"flintstone\" tag=\"1\" num=\"0\">\n"
      "  <leaf name=\"fred\" tag=\"1\" num=\"1\"/>\n"
      "  <leaf name=\"wilma\" tag=\"1\" num=\"2\"/>\n"
      "</bank>\n"
      "<bank name=\"rubble\" tag=\"2\" num=\"0\">\n"
      "  <leaf name=\"barney\" tag=\"2\" num=\"1\"/>\n"
      "  <leaf name=\"betty\"  tag=\"2\" num=\"2\"/>\n"
      "</bank>\n"
      "</dict>\n";
    evioDictionary dict1(dictXML1);

    int size1 = dict1.getTagNumMap.size();
    cout << "dict1 size is " << size1 << endl;



//     string dictXML2 = R"(
// <bank name="dc" tag="1" num="0">
//     <bank name="wire_endpoints" tag="2" num="0">
//         <bank name="left" tag="3" num="0">
//             <leaf name="x" tag="4" num="0"/>
//             <leaf name="y" tag="5" num="0"/>
//             <leaf name="z" tag="6" num="0"/>
//         </bank>
//         <bank name="right" tag="7" num="0">
//             <leaf name="x" tag="8" num="0"/>
//             <leaf name="y" tag="9" num="0"/>
//             <leaf name="z" tag="10" num="0"/>
//         </bank>
//     </bank>
// </bank>
// )";
    
    string dictXML2 = "<bank name=\"dc\" tag=\"1\" num=\"0\">"
    "<bank name=\"wire_endpoints\" tag=\"2\" num=\"0\">"
     "   <bank name=\"left\" tag=\"3\" num=\"0\">"
      "      <leaf name=\"x\" tag=\"4\" num=\"0\"/>"
       "     <leaf name=\"y\" tag=\"5\" num=\"0\"/>"
        "    <leaf name=\"z\" tag=\"6\" num=\"0\"/>"
        "</bank>"
        "<bank name=\"right\" tag=\"7\" num=\"0\">"
         "   <leaf name=\"x\" tag=\"8\" num=\"0\"/>"
          "  <leaf name=\"y\" tag=\"9\" num=\"0\"/>"
          "  <leaf name=\"z\" tag=\"10\" num=\"0\"/>"
        "</bank>"
    "</bank>"
"</bank>";
    
    evioDictionary dict2(dictXML2);
    
    int size2 = dict2.getTagNumMap.size();
    cout << "dict2 size is " << size2 << endl;
    
    cout << dict2.toString() << endl;


    // map<string,tagNum>::const_iterator pos;
    // for (pos = dict2.getTagNumMap.begin(); pos != dict2.getTagNumMap.end(); pos++) {
    //   cout << "key:   " << pos->first << endl;
    //   cout << "value: " << (int)pos->second.first << "," << (int)pos->second.second << endl << endl;
    // }

    // check if something exists
    tagNum tn = dict2.getTagNum("dc.wire_endpoints.right.y");
    cout << "Key dc.wire_endpoints.right.y has tag,num: " << (int)tn.first << "," << (int)tn.second << endl;


  } catch (evioException e) {
    cerr << e.toString() << endl;

  } catch (...) {
    cerr << "?unknown exception" << endl;
  }
  
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
