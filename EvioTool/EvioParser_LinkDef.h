//
//  EvioTool_Linkdef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 4/20/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//
#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all typedef;

#pragma link C++ class EvioParser+;
#pragma link C++ class Bank+;
#pragma link C++ class vector<Bank>;
#pragma link C++ function Bank::Add_Leaf<int>;
#pragma link C++ function Bank::Add_Leaf<float>;
#pragma link C++ function Bank::Add_Leaf<double>;
#pragma link C++ function Bank::Add_Leaf<string>;
#pragma link C++ function Bank::Push_data<int>;
#pragma link C++ function Bank::Push_data<float>;
#pragma link C++ function Bank::Push_data<double>;
#pragma link C++ function Bank::Push_data<string>;
#pragma link C++ function Bank::Push_data_vector<int>;
#pragma link C++ function Bank::Push_data_vector<float>;
#pragma link C++ function Bank::Push_data_vector<double>;
#pragma link C++ function Bank::Push_data_vector<string>;
#pragma link C++ function Bank::Get_data<int>(int,int);
#pragma link C++ function Bank::Get_data<float>(int,int);
#pragma link C++ function Bank::Get_data<double>(int,int);
#pragma link C++ function Bank::Get_data<string>(int,int);
#pragma link C++ function Bank::Get_data<int>(string,int);
#pragma link C++ function Bank::Get_data<float>(string,int);
#pragma link C++ function Bank::Get_data<double>(string,int);
#pragma link C++ function Bank::Get_data<string>(string,int);
#pragma link C++ function Bank::Get_data_vector<int>;
#pragma link C++ function Bank::Get_data_vector<float>;
#pragma link C++ function Bank::Get_data_vector<double>;
#pragma link C++ function Bank::Get_data_vector<string>;
//

#pragma link C++ class Leaf_base+;
#pragma link C++ enum  LeafDataTypes_t;

#pragma link C++ class Leaf<char>+;
#pragma link C++ class Leaf<unsigned char>+;
#pragma link C++ class Leaf<short>+;
#pragma link C++ class Leaf<unsigned short>+;
#pragma link C++ class Leaf<int>+;
#pragma link C++ class Leaf<unsigned int>+;
#pragma link C++ class Leaf<float>+;
#pragma link C++ class Leaf<double>+;
#pragma link C++ class Leaf<string>+;
#pragma link C++ class Leaf<long long>+;
#pragma link C++ class Leaf<unsigned long long>+;

#pragma link C++ class vector<Leaf<char> >;
#pragma link C++ class vector<Leaf<unsigned char> >;
#pragma link C++ class vector<Leaf<short> >;
#pragma link C++ class vector<Leaf<unsigned short> >;
#pragma link C++ class vector<Leaf<int> >;
#pragma link C++ class vector<Leaf<unsigned int> >;
#pragma link C++ class vector<Leaf<long long> >;
#pragma link C++ class vector<Leaf<unsigned long long> >;
#pragma link C++ class vector<Leaf<float> >;
#pragma link C++ class vector<Leaf<double> >;
#pragma link C++ class vector<Leaf<string> >;


#pragma link C++ class FADCdata+;
#pragma link C++ class Leaf<FADCdata>+;
#pragma link C++ class vector<Leaf<FADCdata> >;

#endif
